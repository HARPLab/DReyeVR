import numpy as np
from typing import Any, Dict, List, Optional

import sys
import glob
import os

try:
    sys.path.append(
        glob.glob(
            "../carla/dist/carla-*%d.%d-%s.egg"
            % (
                sys.version_info.major,
                sys.version_info.minor,
                "win-amd64" if os.name == "nt" else "linux-x86_64",
            )
        )[0]
    )
except IndexError:
    pass

import carla


def find_ego_vehicle(world: carla.libcarla.World) -> Optional[carla.libcarla.Vehicle]:
    DReyeVR_vehicle = None
    ego_vehicles = list(world.get_actors().filter("harplab.dreyevr_vehicle.*"))
    if len(ego_vehicles) >= 1:
        DReyeVR_vehicle = ego_vehicles[0]  # TODO: support for multiple ego vehicles?
    else:
        model: str = "harplab.dreyevr_vehicle.model3"
        print(f'No EgoVehicle found, spawning one: "{model}"')
        bp = world.get_blueprint_library().find(model)
        transform = world.get_map().get_spawn_points()[0]
        DReyeVR_vehicle = world.spawn_actor(bp, transform)
    return DReyeVR_vehicle


def find_ego_sensor(world: carla.libcarla.World) -> Optional[carla.libcarla.Sensor]:
    sensor = None
    ego_sensors = list(world.get_actors().filter("harplab.dreyevr_sensor.*"))
    if len(ego_sensors) >= 1:
        sensor = ego_sensors[0]  # TODO: support for multiple ego sensors?
    elif find_ego_vehicle(world) is None:
        raise Exception(
            "No EgoVehicle (nor EgoSensor) found in the world! EgoSensor needs EgoVehicle as parent"
        )
    return sensor


class DReyeVRSensor:
    def __init__(self, world: carla.libcarla.World):
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

    def update(self, data) -> None:
        # update local variables
        elements: List[str] = [key for key in dir(data) if "__" not in key]
        for key in elements:
            self.data[key] = self.preprocess(getattr(data, key))

    @classmethod
    def spawn(cls, world: carla.libcarla.World):
        # TODO: check if dreyevr sensor already exsists, then use it
        # spawn a DReyeVR sensor and begin listening
        if find_ego_sensor(world) is None:
            bp = [x for x in world.get_blueprint_library().filter("sensor.dreyevr*")]
            try:
                bp = bp[0]
            except IndexError:
                print("no eye tracker in blueprint library?!")
                return None
            ego_vehicle = find_ego_vehicle()
            ego_sensor = world.spawn_actor(
                bp, ego_vehicle.get_transform(), attach_to=ego_vehicle
            )
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
