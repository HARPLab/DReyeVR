import argparse
import datetime
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

import numpy as np
from config_manager import ConfigManager
from DReyeVR_utils import DReyeVRSensor, find_ego_vehicle, save_sensor_data_to_csv
from HapticSharedControl.haptic_algo import HapticSharedControl
from HapticSharedControl.utils import dist
from HapticSharedControl.wheel_control import WheelController

import carla


class DReyeVRController:
    """Main controller class for DReyeVR haptic control system"""
    
    def __init__(self, config_manager: ConfigManager, path_idx: Optional[str] = None):
        """
        Initialize the controller
        
        Args:
            config_manager: Configuration manager
            path_idx: Path index to use (overrides default from config)
        """
        # Configuration
        self.config = config_manager
        
        # Setup timestamp and paths
        self.timestamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        self.log_dir = self.config.get_logging_dir()
        
        # State variables
        self.ready = True
        self.take_control = False
        self.backward_btn_pressed_cnt = 0
        self.delta_t = self.config.get_config("simulation.delta_t", 1.0/60)
        self.path_idx = path_idx or self.config.get_default_path_idx()
        self.current_path_param = None
        
        # Get vehicle
        self.vehicle = self.config.get_vehicle()
        
        # Calibration data
        self.init_sa_swa = [[], []]  # [steering_angles, steering_wheel_angles]
        self.coef = [[], []]        # Calibration coefficients
        
        # Initialize wheel controller
        self.controller = WheelController()
        
        # Create log file paths
        self.haptic_log_path = self.log_dir / f"haptic_shared_control_log_{self.timestamp}.csv"
        self.wheel_log_path = self.log_dir / f"steering_wheel_log_{self.timestamp}.csv"
        self.sensor_log_path = self.log_dir / f"carla_sensor_log_{self.timestamp}.csv"
        
        print(f"Using path index: {self.path_idx}")
        print(f"Logs will be saved to: {self.log_dir}")
    
    def initialize_steering_calibration(self, measured_carla_data: Dict) -> bool:
        """
        Calibrate steering wheel angle and steering angle relationship
        
        Args:
            measured_carla_data: Sensor data from CARLA
            
        Returns:
            bool: True if calibration is complete
        """
        # Calculate offset for spring force
        desired_offset = len(self.init_sa_swa[0]) - 100
        
        # Apply spring force for calibration
        self.controller.play_spring_force(
            offset_percentage=desired_offset,
            saturation_percentage=100,
            coefficient_percentage=100,
        )
        
        # Get current angles
        steering_wheel_angle = self.controller.get_angle() * 450.0  # in degrees
        steering_angles = [
            measured_carla_data["FL_Wheel_Angle"],
            measured_carla_data["FR_Wheel_Angle"],
        ]
        
        # Calculate vehicle steering angle
        vehicle_steering_angle = self.vehicle.calc_turning_radius(steering_angles)["Delta"]
        
        # Store calibration data
        self.init_sa_swa[0].append(vehicle_steering_angle)
        self.init_sa_swa[1].append(steering_wheel_angle)
        
        # Check if calibration is complete
        max_samples = self.config.get_config("simulation.calibration_samples", 201)
        if len(self.init_sa_swa[0]) > max_samples:
            x = np.array(self.init_sa_swa[0])
            y = np.array(self.init_sa_swa[1])
            
            # Calculate linear relationships
            self.coef[0] = np.polyfit(x, y, 1)  # steering angle to wheel angle
            self.coef[1] = np.polyfit(y, x, 1)  # wheel angle to steering angle
            
            print("Calibration completed")
            print(f"Steering Wheel Angle = {self.coef[0][0]} * Steering Angle + {self.coef[0][1]}")
            print(f"Steering Angle = {self.coef[1][0]} * Steering Wheel Angle + {self.coef[1][1]}")
            
            return True
        
        return False
    
    def log_wheel_data(self, desired_steering_angle_deg: float) -> None:
        """
        Log steering wheel data to CSV
        
        Args:
            desired_steering_angle_deg: Desired steering angle in degrees
        """
        # Create file with header if it doesn't exist
        if not self.wheel_log_path.exists():
            with open(self.wheel_log_path, "w") as f:
                f.write("Desired Steering Angle, Acceleration Pedal, Steering Wheel Angle\n")
        
        # Append data
        with open(self.wheel_log_path, "a") as f:
            f.write(f"{desired_steering_angle_deg},{self.controller.get_acceleration_pedal()},{self.controller.get_angle()}\n")
    
    def handle_driving_direction(self, buttons: Dict) -> bool:
        """
        Handle driving direction changes based on button presses
        
        Args:
            buttons: Button states from wheel controller
            
        Returns:
            bool: True if driving backward, False if forward
        """
        if any([btn_value for btn_value in list(buttons.values())[1:]]):
            self.backward_btn_pressed_cnt += 1
            print("Backward button pressed")
        
        return self.backward_btn_pressed_cnt % 2 == 1
    
    def check_path_control_triggers(self, position_to_world: List[float]) -> bool:
        """
        Check if vehicle should begin path control
        
        Args:
            position_to_world: Current vehicle position [x, y]
            
        Returns:
            bool: True if control should be triggered
        """
        # Get path data for current index
        path_data = self.config.get_path(self.path_idx)
        
        # If vehicle is near destination control point
        if "P_d" in path_data and len(path_data["P_d"]) > 0:
            if dist(position_to_world, path_data["P_d"]) < 5 and not self.take_control:
                # Set backward path
                self.current_path_param = path_data["backward paths"]
                self.take_control = True
                print("Control triggered - starting haptic guidance")
                return True
        
        return False
    
    def execute_haptic_control(
        self, 
        position_to_world: List[float],
        vehicle_yaw: float,
        steering_angles: List[float],
        steering_wheel_angle: float,
        speed: float
    ) -> float:
        """
        Execute haptic shared control
        
        Args:
            position_to_world: Current vehicle position [x, y]
            vehicle_yaw: Current vehicle yaw angle in degrees
            steering_angles: Wheel steering angles [FL, FR] in degrees
            steering_wheel_angle: Steering wheel angle in degrees
            speed: Vehicle speed in m/s
            
        Returns:
            float: Desired steering angle in degrees
        """
        # Get haptic control settings
        haptic_settings = self.config.get_haptic_control_settings()
        
        # Create haptic control object
        haptic_control = HapticSharedControl(
            Cs=haptic_settings["Cs"],
            Kc=haptic_settings["Kc"],
            tp=haptic_settings["tp"],
            speed=speed,
            desired_trajectory_params=self.current_path_param,
            vehicle_config=self.config.vehicle_config,
            simulation=False,
            log_save_path=str(self.haptic_log_path)
        )
        haptic_control.debug = self.config.get_config("logging.enable_debug", True)
        
        # Calculate torque and desired steering angle
        _, _, desired_steering_angle_deg = haptic_control.calculate_torque(
            current_position=position_to_world,
            current_yaw_angle_deg=vehicle_yaw,
            steering_angles_deg=steering_angles,
            steering_wheel_angle_deg=steering_wheel_angle,
        )
        
        # Clip to valid range
        desired_steering_angle_deg = np.clip(desired_steering_angle_deg, -450, 450)
        
        # Calculate desired offset for force feedback
        try:
            desired_offset = int(desired_steering_angle_deg * 100 / 450.0)
        except Exception:
            # Fallback calculation if normal calculation fails
            try:
                desired_offset_deg = np.degrees(
                    np.arctan(
                        (self.current_path_param["end_y"] - position_to_world[1]) / 
                        (self.current_path_param["end_x"] - position_to_world[0])
                    )
                )
                desired_offset = int(desired_offset_deg * 100 / 450.0)
            except Exception:
                # Safe fallback
                desired_offset = 0
        
        # Get force feedback settings
        saturation, coefficient = self.config.get_force_feedback_settings()
        
        # Apply force feedback
        print(f"--> Desired Offset: {desired_offset}")
        if coefficient > 0:
            self.controller.play_spring_force(
                offset_percentage=desired_offset,
                saturation_percentage=saturation,
                coefficient_percentage=coefficient,
            )
        
        # Log wheel data
        self.log_wheel_data(desired_steering_angle_deg)
        
        return desired_steering_angle_deg
    
    def check_destination_reached(self, position_to_world: List[float]) -> bool:
        """
        Check if vehicle has reached the destination
        
        Args:
            position_to_world: Current vehicle position [x, y]
            
        Returns:
            bool: True if destination reached
        """
        path_data = self.config.get_path(self.path_idx)
        
        if "P_f" in path_data and len(path_data["P_f"]) > 0:
            if dist(position_to_world, path_data["P_f"]) < 1:
                self.controller.stop_spring_force()
                print("Simulation Completed - reached destination")
                return True
        
        return False
    
    def print_distance_info(self, position_to_world: List[float]) -> None:
        """
        Print distance information
        
        Args:
            position_to_world: Current vehicle position [x, y]
        """
        path_data = self.config.get_path(self.path_idx)
        
        distance_to_SP = float('inf')
        distance_to_DCP = float('inf')
        
        if "P_0" in path_data and len(path_data["P_0"]) > 0:
            distance_to_SP = dist(position_to_world, path_data["P_0"])
            
        if "P_d" in path_data and len(path_data["P_d"]) > 0:
            distance_to_DCP = dist(position_to_world, path_data["P_d"])
        
        print(
            f"Distance to SP: {distance_to_SP:.3f}m",
            f" | Distance to DCP: {distance_to_DCP:.3f}m",
        )
    
    def control_loop(self, data: Dict) -> None:
        """
        Main control loop executed for each sensor update
        
        Args:
            data: Sensor data from CARLA
        """
        # Update sensor data
        sensor.update(data)
        measured_carla_data = sensor.data
        
        # Handle calibration if not ready
        if not self.ready:
            calibration_complete = self.initialize_steering_calibration(measured_carla_data)
            
            if calibration_complete:
                self.ready = True
                print("Ready to start the simulation")
                sys.exit(0)
                
            time.sleep(0.5)
            return

        print("=====================================")
        
        
        # 1. Get vehicle state
        position_to_world = measured_carla_data["Location"][0:2]
        vehicle_yaw = measured_carla_data["Rotation"][1]
        velocity = measured_carla_data["Velocity"][0:2]
        speed = np.linalg.norm(velocity)
        
        # 2. Handle driving direction
        buttons = self.controller.get_buttons_pressed()
        backward = self.handle_driving_direction(buttons)
        
        # Adjust speed for direction
        speed *= -1 if backward else 1
        print("Driving Direction:", "Backward" if backward else "Forward")
        
        # 3. Get steering information
        steering_wheel_angle = self.controller.get_angle() * 450.0
        steering_angles = [
            measured_carla_data["FL_Wheel_Angle"],
            measured_carla_data["FR_Wheel_Angle"],
        ]
        
        # 4. Check if control should be triggered
        self.check_path_control_triggers(position_to_world)
        
        # 5. Execute control or show distance info
        if self.take_control:
            self.execute_haptic_control(
                position_to_world,
                vehicle_yaw,
                steering_angles,
                steering_wheel_angle,
                speed
            )
            
            # Save sensor data
            save_sensor_data_to_csv(measured_carla_data, file_path=str(self.sensor_log_path))
        else:
            self.print_distance_info(position_to_world)
        
        # 6. Check if destination reached
        if self.check_destination_reached(position_to_world):
            sys.exit(0)
        
        # Wait for next frame
        time.sleep(self.delta_t)


def main():
    """Main function to set up and run the DReyeVR system"""
    # Parse command line arguments
    argparser = argparse.ArgumentParser(
        description="DReyeVR Haptic Shared Control System"
    )
    argparser.add_argument(
        "--host",
        metavar="H",
        default="127.0.0.1",
        help="IP of the host server (default: 127.0.0.1)",
    )
    argparser.add_argument(
        "-p",
        "--port",
        metavar="P",
        default=2000,
        type=int,
        help="TCP port to listen to (default: 2000)",
    )
    argparser.add_argument(
        "--config",
        default=None,
        help="Path to config file",
    )
    argparser.add_argument(
        "--paths",
        default=None,
        help="Path to paths definition file",
    )
    argparser.add_argument(
        "--path-idx",
        default=None,
        help="Path index to use (overrides config)",
    )

    args = argparser.parse_args()

    # Initialize configuration manager
    config_manager = ConfigManager(
        config_path=args.config,
        paths_path=args.paths
    )

    # Connect to CARLA
    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    sync_mode = config_manager.get_config("simulation.sync_mode", True)
    np.random.seed(int(time.time()))

    world = client.get_world()
    
    # Initialize sensor and find ego vehicle
    global sensor
    sensor = DReyeVRSensor(world)
    find_ego_vehicle(world)
    
    # Create controller
    controller = DReyeVRController(
        config_manager=config_manager,
        path_idx=args.path_idx
    )
    
    # Subscribe sensor to control loop
    sensor.ego_sensor.listen(controller.control_loop)
    
    try:
        # Main loop
        while True:
            if sync_mode:
                world.tick()
            else:
                world.wait_for_tick()
    finally:
        # Clean up
        if sync_mode:
            settings = world.get_settings()
            settings.synchronous_mode = False
            settings.fixed_delta_seconds = None
            world.apply_settings(settings)
        
        controller.controller.exit()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        print("\ndone.")