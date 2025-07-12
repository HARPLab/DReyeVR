import csv
import os
import sys
import time
from typing import Any, Dict, List, Optional

import carla
import numpy as np

sys.path.append(os.path.join(os.getenv("CARLA_ROOT"), "PythonAPI"))
import examples  # calls ./__init__.py to add all the necessary things to path

# cspell: ignore dreyevr dreyevrsensor libcarla harplab vergence numer linalg

def find_ego_vehicle(world: carla.libcarla.World) -> Optional[carla.libcarla.Vehicle]:
    DReyeVR_vehicles: str = "harplab.dreyevr_vehicle.*"
    ego_vehicles_in_world = list(world.get_actors().filter(DReyeVR_vehicles))
    if len(ego_vehicles_in_world) >= 1:
        print(f"Found a DReyeVR EgoVehicle in the world ({ego_vehicles_in_world[0].id})")
        return ego_vehicles_in_world[0]

    DReyeVR_vehicle: Optional[carla.libcarla.Vehicle] = None
    available_ego_vehicles = world.get_blueprint_library().filter(DReyeVR_vehicles)
    if len(available_ego_vehicles) == 1:
        bp = available_ego_vehicles[0]
        print(f'Spawning only available EgoVehicle: "{bp.id}"')
    else:
        print(f"Found {len(available_ego_vehicles)} available EgoVehicles. Which one to use?")
        for i, ego in enumerate(available_ego_vehicles):
            print(f"\t[{i}] - {ego.id}")
        print()
        ego_choice = f"Pick EgoVehicle to spawn [0-{len(available_ego_vehicles) - 1}]: "
        i: int = int(input(ego_choice))
        assert 0 <= i < len(available_ego_vehicles)
        bp = available_ego_vehicles[i]
    i: int = 0
    spawn_pts = world.get_map().get_spawn_points()
    while DReyeVR_vehicle is None:
        print(f'Spawning DReyeVR EgoVehicle: "{bp.id}" at {spawn_pts[i]}')
        DReyeVR_vehicle = world.spawn_actor(bp, transform=spawn_pts[i])
        i = (i + 1) % len(spawn_pts)
    return DReyeVR_vehicle


def find_ego_sensor(world: carla.libcarla.World) -> Optional[carla.libcarla.Sensor]:
    def get_world_sensors() -> list:
        return list(world.get_actors().filter("harplab.dreyevr_sensor.*"))

    ego_sensors: list = get_world_sensors()
    if len(ego_sensors) == 0:
        # no EgoSensors found in world, trying to spawn EgoVehicle (which spawns an EgoSensor)
        if find_ego_vehicle(world) is None:  # tries to spawn an EgoVehicle
            raise Exception(
                "No EgoVehicle (nor EgoSensor) found in the world! EgoSensor needs EgoVehicle as parent"
            )
    # in case we had to spawn the EgoVehicle, this effect is not instant and might take some time
    # to account for this, we allow some time (max_wait_sec) to continuously ping the server for
    # an updated actor list with the EgoSensor in it.

    start_t: float = time.time()
    # maximum time to keep checking for EgoSensor spawning after EgoVehicle
    maximum_wait_sec: float = 10.0  # might take a while to spawn EgoVehicle (esp in VR)
    while len(ego_sensors) == 0 and time.time() - start_t < maximum_wait_sec:
        # EgoVehicle should now be present, so we can try again
        ego_sensors = get_world_sensors()
        time.sleep(0.1)  # tick to allow the server some time to breathe
    if len(ego_sensors) == 0:
        raise Exception("Unable to find EgoSensor in the world!")
    assert len(ego_sensors) > 0  # should have spawned with EgoVehicle at least
    if len(ego_sensors) > 1:
        print("[WARN] There are >1 EgoSensors in the world! Defaulting to first")
    return ego_sensors[0]  # always return the first one?


class DReyeVRSensor:
    def __init__(self, world: carla.libcarla.World):
        self.ego_vehicle: carla.libcarla.Vehicle = find_ego_vehicle(world)
        self.ego_sensor: carla.sensor.dreyevrsensor = find_ego_sensor(world)
        self.data: Dict[str, Any] = {}
        print("initialized DReyeVRSensor PythonAPI client")

    def preprocess(self, obj: Any) -> Any:
        if isinstance(obj, carla.libcarla.Vector3D):
            return np.array([obj.x, obj.y, obj.z])
        if isinstance(obj, carla.libcarla.Vector2D):
            return np.array([obj.x, obj.y])
        if isinstance(obj, carla.libcarla.Transform):
            return [
                np.array([obj.location.x, obj.location.y, obj.location.z]),
                np.array([obj.rotation.pitch, obj.rotation.yaw, obj.rotation.roll]),
            ]
        return obj

    def update(self, data: dict) -> None:
        # update local variables
        elements: List[str] = [key for key in dir(data) if "__" not in key]
        for key in elements:
            self.data[key] = self.preprocess(getattr(data, key))
        # update location and rotation
        location = self.ego_vehicle.get_transform().location
        rotation = self.ego_vehicle.get_transform().rotation
        self.data["Location"] = np.array([location.x, location.y, location.z])
        self.data["Rotation"] = np.array([rotation.pitch, rotation.yaw, rotation.roll])
        # velocity
        velocity = self.ego_vehicle.get_velocity()
        self.data["Velocity"] = np.array([velocity.x, velocity.y, velocity.z])
        # acceleration
        acceleration = self.ego_vehicle.get_acceleration()
        self.data["Acceleration"] = np.array(
            [acceleration.x, acceleration.y, acceleration.z]
        )
        # angular velocity
        angular_velocity = self.ego_vehicle.get_angular_velocity()
        self.data["AngularVelocity"] = np.array(
            [angular_velocity.x, angular_velocity.y, angular_velocity.z]
        )
        # wheel angles
        wheel_locations = {
            "FL_Wheel": carla.VehicleWheelLocation.FL_Wheel,
            "FR_Wheel": carla.VehicleWheelLocation.FR_Wheel,
            "BL_Wheel": carla.VehicleWheelLocation.BL_Wheel,
            "BR_Wheel": carla.VehicleWheelLocation.BR_Wheel,
        }
        for key, wheel_name in wheel_locations.items():
            self.data[f"{key}_Angle"] = float(self.ego_vehicle.get_wheel_steer_angle(wheel_name))

    @classmethod
    def spawn(cls, world: carla.libcarla.World):
        # TODO: check if dreyevr sensor already exsists, then use it
        # spawn a DReyeVR sensor and begin listening
        if find_ego_sensor(world) is None:
            bp = list(world.get_blueprint_library().filter("harplab.dreyevr_sensor.*"))
            try:
                bp = bp[0]
            except IndexError:
                print("no eye tracker in blueprint library?!")
                return None
            ego_vehicle = find_ego_vehicle()
            ego_sensor = world.spawn_actor(bp, ego_vehicle.get_transform(), attach_to=ego_vehicle)
            print("Spawned DReyeVR sensor: " + ego_sensor.type_id)
        return cls(world)

    def calc_vergence_from_dir(self, L0, R0, LDir, RDir):
        # Calculating shortest line segment intersecting both lines
        # Implementation sourced from http://paulbourke.net/geometry/Ptlineplane/

        L0R0 = L0 - R0  # segment between L origin and R origin
        epsilon = 0.00000001  # small positive real number

        # Calculating dot-product equation to find perpendicular shortest-line-segment
        d1343 = L0R0[0] * RDir[0] + L0R0[1] * RDir[1] + L0R0[2] * RDir[2]
        d4321 = RDir[0] * LDir[0] + RDir[1] * LDir[1] + RDir[2] * LDir[2]
        d1321 = L0R0[0] * LDir[0] + L0R0[1] * LDir[1] + L0R0[2] * LDir[2]
        d4343 = RDir[0] * RDir[0] + RDir[1] * RDir[1] + RDir[2] * RDir[2]
        d2121 = LDir[0] * LDir[0] + LDir[1] * LDir[1] + LDir[2] * LDir[2]
        denom = d2121 * d4343 - d4321 * d4321
        if abs(denom) < epsilon:
            return 1.0  # no intersection, would cause div by 0 err (potentially)
        numer = d1343 * d4321 - d1321 * d4343

        # calculate scalars (mu) that scale the unit direction XDir to reach the desired points
        # variable scale of direction vector for LEFT ray
        muL = numer / denom
        # variable scale of direction vector for RIGHT ray
        muR = (d1343 + d4321 * (muL)) / d4343

        # calculate the points on the respective rays that create the intersecting line
        ptL = L0 + muL * LDir  # the point on the Left ray
        ptR = R0 + muR * RDir  # the point on the Right ray

        # calculate the vector between the middle of the two endpoints and return its magnitude
        # middle point between two endpoints of shortest-line-segment
        ptM = (ptL + ptR) / 2.0
        oM = (L0 + R0) / 2.0  # midpoint between two (L & R) origins
        FinalRay = ptM - oM  # Combined ray between midpoints of endpoints
        # returns the magnitude of the vector (length)
        return np.linalg.norm(FinalRay) / 100.0
    
def save_sensor_data_to_csv(data, file_path="dreyevr_sensor_data.csv"):
    """
    Save DReyeVR sensor data to a CSV file in tabular format.
    
    Args:
        data (dict): The sensor data dictionary from DReyeVR
        file_path (str): Path to save the CSV file
    """

    
    file_exists = os.path.isfile(file_path)
    
    # Define headers - flatten nested arrays
    headers = []
    row_data = {}
    
    for key, value in data.items():
        # Handle special case for transform (list of arrays)
        if key == 'transform':
            for i, arr in enumerate(value):
                for j, val in enumerate(arr):
                    column_name = f"{key}_{i}_{j}"
                    headers.append(column_name)
                    row_data[column_name] = val
        # Handle numpy arrays
        elif isinstance(value, np.ndarray):
            for i, val in enumerate(value):
                column_name = f"{key}_{i}"
                headers.append(column_name)
                row_data[column_name] = val
        # Handle standard values
        else:
            headers.append(key)
            row_data[key] = value
    
    # Write to CSV file
    with open(file_path, 'a', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=headers)
        
        # Write header only if the file is newly created
        if not file_exists:
            writer.writeheader()
        
        # Write the data row
        writer.writerow(row_data)
        
"""sensor data example:
{'AngularVelocity': array([ 6.43164603e-05,  3.80409183e-05, -1.56078613e-06]),
 'BL_Wheel_Angle': 0.0,
 'BR_Wheel_Angle': 0.0,
 'FL_Wheel_Angle': 0.0,
 'FR_Wheel_Angle': 0.0,
 'Location': array([  1.03617442, -18.3298645 ,   0.16717896]),
 'Rotation': array([-4.49221507e-02,  4.99724770e+01, -3.72619666e-02]),
 'Velocity': array([4.93484549e-06, 2.47542033e-07, 4.61575430e-04]),
 'brake_input': 0.0,
 'camera_location': array([0., 0., 0.]),
 'camera_rotation': array([0., 0., 0.]),
 'current_gear_input': False,
 'focus_actor_dist': 1164.83837890625,
 'focus_actor_name': 'Road_Grass_Town05_123',
 'focus_actor_pt': array([ -111.9954071 , -1904.90673828,    16.88562012]),
 'frame': 13614,
 'frame_number': 13614,
 'framesequence': 13612,
 'gaze_dir': array([ 0.98058057,  0.10336377, -0.16666579]),
 'gaze_origin': array([0., 0., 0.]),
 'gaze_valid': True,
 'gaze_vergence': 0.0,
 'handbrake_input': False,
 'left_eye_openness': 0.0,
 'left_eye_openness_valid': False,
 'left_gaze_dir': array([ 0.98058057,  0.10336377, -0.16666579]),
 'left_gaze_origin': array([ 0., -5.,  0.]),
 'left_gaze_valid': False,
 'left_pupil_diam': 0.0,
 'left_pupil_posn': array([0., 0.]),
 'left_pupil_posn_valid': False,
 'right_eye_openness': 0.0,
 'right_eye_openness_valid': False,
 'right_gaze_dir': array([ 0.98058057,  0.10336377, -0.16666579]),
 'right_gaze_origin': array([0., 5., 0.]),
 'right_gaze_valid': False,
 'right_pupil_diam': 0.0,
 'right_pupil_posn': array([0., 0.]),
 'right_pupil_posn_valid': False,
 'steering_input': 0.0002929776965174824,
 'throttle_input': 0.0,
 'timestamp': 55.89655466005206,
 'timestamp_carla': 55896,
 'timestamp_device': 55533,
 'timestamp_stream': 55.89655466005206,
 'transform': [array([ 0.00987167, -0.00354858,  0.1648705 ]),
               array([-0.0574214 , -0.02758213,  0.01046066])]}

"""