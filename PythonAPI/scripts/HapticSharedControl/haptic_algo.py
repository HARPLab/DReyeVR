import csv
import datetime
import os
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np
from scipy.spatial import KDTree

from .utils import (
    Vehicle,
    dist,
    find_closest_point,
    getAngle,
    linear_fn,
    rotation_matrix_cw,
    sigmoid,
    sign,
)

# Constants
DEFAULT_LOG_DIR = Path(__file__).parent.parent / "logs"
os.makedirs(DEFAULT_LOG_DIR, exist_ok=True)
CURRENT_TIME = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")


def get_sign_of_error(
    given_position: List[float], 
    desired_trajectory: np.ndarray, 
    tangent_vectors: np.ndarray, 
    velocity: float
) -> float:
    """
    Calculate the sign of the error between current position and desired trajectory.
    
    Args:
        given_position: [x, y] coordinate of the current position
        desired_trajectory: Array of trajectory points [[x1, y1], [x2, y2], ...]
        tangent_vectors: Tangent vectors of the trajectory
        velocity: Vehicle speed
        
    Returns:
        Sign of the error (-1 or 1)
    """
    p1 = given_position
    _, pt_index = find_closest_point(p1, desired_trajectory)

    p2, p3 = tangent_vectors[pt_index]
    angle_rad = getAngle(p1, p2, p3, degrees=False)

    # Return direction based on velocity and angle
    return sign(velocity) * (np.sin(angle_rad) / abs(np.sin(angle_rad)))


def distance_to_trajectory(
    given_position: List[float], 
    desired_trajectory: np.ndarray
) -> Tuple[float, np.ndarray, int]:
    """
    Calculate the distance from a position to the desired trajectory.
    
    Args:
        given_position: The (x, y) coordinates of the position
        desired_trajectory: Array of trajectory points [[x1, y1], [x2, y2], ...]
        
    Returns:
        Tuple containing:
          - distance: Distance to the closest point on trajectory
          - closest_point: Coordinates of the closest point
          - index: Index of the closest point in the trajectory array
    """
    closest_point, idx = find_closest_point(given_position, desired_trajectory)
    return dist(given_position, closest_point), closest_point, idx

def look_ahead_distance(vel, tp, cur_pos=None, end_pos=None, method="fix"):
    """
    Calculate the look-ahead distance based on velocity and time preview.
    
    Args:
        vel: Vehicle speed
        tp: Time preview in seconds
    
    Returns:
        Look-ahead distance in meters
    """
    if method == 'fix':
        return sign(vel) * 4.8 # Fixed distance of 4.8m
    elif method == 'dynamic':
        return sign(vel) * 1.0 *  dist(cur_pos, end_pos) # Dynamic distance based on trajectory  
    else:
        return vel * tp    
    

def predict_position(
    current_position: List[float],
    current_yaw_angle_deg: float,
    vehicle_steering_angle_deg: float,
    center_of_rotation_to_vehicle: List[float],
    turning_radius: float,
    look_ahead_distance: float,
) -> Dict[str, Any]:
    """
    Predict the vehicle's position after a time preview based on current state.
    
    Args:
        current_position: Current [x, y] position
        current_yaw_angle_deg: Current yaw angle in degrees
        vehicle_steering_angle_deg: Current steering angle in degrees
        center_of_rotation_to_vehicle: Center of rotation relative to vehicle
        turning_radius: Vehicle turning radius
        look_ahead_distance: Look-ahead distance in meters
        
    Returns:
        Dictionary containing:
          - predicted_position: Predicted [x, y] position
          - rotating_direction: Direction of rotation (-1: CW, 1: CCW)
          - delta_phi_rad: Change in yaw angle in radians
          - predict_yaw_angle_rad: Predicted yaw angle in radians
          - center_of_rotation_to_world: Center of rotation in world coordinates
    """
    # Convert angles to radians
    current_yaw_angle_rad = np.radians(current_yaw_angle_deg)
    vehicle_steering_angle_rad = np.radians(vehicle_steering_angle_deg)
    
    # Adjust coordinate system
    current_yaw_angle_rad = np.pi / 2 - current_yaw_angle_rad
    current_position = np.array(current_position)[::-1]
    center_of_rotation_to_vehicle = np.array(center_of_rotation_to_vehicle)[::-1]
    
    # Define rotation direction for the vehicle
    rotating_direction = sign(np.sin(vehicle_steering_angle_rad))
    
    # Transform vehicle coordinates to global coordinates
    center_of_rotation_to_world = (
        rotation_matrix_cw(current_yaw_angle_rad - np.pi / 2).dot(center_of_rotation_to_vehicle) + 
        np.array(current_position)
    )
    
    # Calculate predicted position
    # delta_phi_rad = rotating_direction * velocity * tp / turning_radius
    delta_phi_rad = rotating_direction * look_ahead_distance / turning_radius
    predict_yaw_angle_rad = current_yaw_angle_rad - delta_phi_rad

    # Calculate the predicted position based on circular motion
    angle_for_prediction = (
        predict_yaw_angle_rad - 
        vehicle_steering_angle_rad + 
        rotating_direction * np.pi / 2
    )
    
    predicted_position_in_world = (
        center_of_rotation_to_world + 
        turning_radius * np.array([
            np.cos(angle_for_prediction),
            np.sin(angle_for_prediction)
        ])
    )
    
    # Convert back to original coordinate system
    predict_yaw_angle_rad = np.pi / 2 - predict_yaw_angle_rad 
    predicted_position_in_world = predicted_position_in_world[::-1]
    center_of_rotation_to_world = center_of_rotation_to_world[::-1]

    return {
        "predicted_position": predicted_position_in_world,
        "rotating_direction": rotating_direction,
        "delta_phi_rad": delta_phi_rad,
        "predict_yaw_angle_rad": predict_yaw_angle_rad,
        "center_of_rotation_to_world": center_of_rotation_to_world,
    }


# Log data schema with typed dictionary structure
class LoggingManager:
    """Manages logging and data storage for the haptic control system"""
    
    def __init__(self, log_path: Optional[str] = None, debug: bool = False):
        """
        Initialize logging manager
        
        Args:
            log_path: Path to save log file
            debug: Whether to print debug information
        """
        self.log_path = log_path
        self.debug = debug
        self.log_data = self._create_log_dict()
        
    def _create_log_dict(self) -> Dict:
        """Create empty log dictionary with proper structure"""
        return {
            "Time (t)": [],
            "[Measured] Speed ~ V(t) (m/s)": [],
            "[Measured] Current Position ~ X(t) (m)": [],
            "[Measured] Current Position ~ Y(t) (m)": [],
            "[Measured] Steering Angle FL ~ delta_fl(t) (deg)": [],
            "[Measured] Steering Angle FR ~ delta_fr(t) (deg)": [],
            "[Measured] Current Yaw Angle ~ Phi(t) (deg)": [],
            "[Measured] Steering Wheel Angle SWA ~ Theta(t) (deg)": [],
            "[Measured] VelocityX ~ Vx(t) (m/s)": [],
            "[Measured] VelocityY ~ Vy(t) (m/s)": [],
            "[Measured] AccelerationX ~ Ax(t) (m/s^2)": [],
            "[Measured] AccelerationY ~ Ay(t) (m/s^2)": [],
            "[Measured] Angular VelocityZ ~ Phi_dot(t) (rad/s^2)": [],
            "[Computed] Rotation direction ~ rotate(t) (CW/CCW)": [],
            "[Computed] Turning Radius ~ R(delta) (m)": [],
            "[Computed] Vehicle Steering Angle ~ Delta_v(t) (deg)": [],
            "[Computed] Center of Rotation to WORLD ~ X_c(t) (m)": [],
            "[Computed] Center of Rotation to WORLD ~ Y_c(t) (m)": [],
            "[Computed] Current Error to Trajectory ~ e(t) (m)": [],
            "[Computed] Predicted Position ~ X_tp(t) (m)": [],
            "[Computed] Predicted Position ~ Y_tp(t) (m)": [],
            "[Computed] Change of Yaw Angle ~ Delta_Phi(t) (deg)": [],
            "[Computed] Predicted Yaw Angle ~ Phi_tp(t) (deg)": [],
            "[Computed] Predicted Error to Trajectory ~ eps_tp(t) (m)": [],
            "[Computed] Desired Steering Wheel Angle ~ Theta_d(t) (deg)": [],
            "[Computed] Torque applied ~ Tau_das (N.m)": [],
        }
    
    def log(self, data_dict: Dict[str, Any]) -> None:
        """
        Log data to internal storage and optionally to file
        
        Args:
            data_dict: Dictionary containing values to log
        """
        # Record timestamp
        self.log_data["Time (t)"].append(time.time())
        
        # Log all provided values
        for key, value in data_dict.items():
            if key in self.log_data:
                self.log_data[key].append(value)
                
        # Debug print if enabled
        if self.debug:
            print("----------------------")
            print("Logging data...")
            for key in self.log_data:
                if len(self.log_data[key]) > 0:
                    print(f"{key}: {self.log_data[key][-1]}")
        
        # Save to file if path is provided
        if self.log_path:
            self.save_log()
    
    def save_log(self) -> None:
        """
        Save the log data to a CSV file in an efficient manner
        """
        file_path = self.log_path
        file_exists = os.path.isfile(file_path)
        
        # Prepare headers and row data
        headers = list(self.log_data.keys())
        row_data = {}
        
        # Get the latest value for each field
        for key in headers:
            values = self.log_data[key]
            row_data[key] = values[-1] if values else None
        
        # Write to file (append mode)
        with open(file_path, 'a', newline='') as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=headers)
            
            # Write header only if file is new
            if not file_exists:
                writer.writeheader()
            
            # Write the latest data point
            writer.writerow(row_data)
        
        # Debug message
        if self.debug:
            print(f"Log data saved to {file_path}")


class HapticSharedControl:
    """
    Implements haptic shared control algorithm for vehicle steering
    
    Calculates torque to be applied to the steering wheel based on:
    - Current position and orientation
    - Desired trajectory
    - Steering angles and wheel angle
    - Vehicle dynamics
    """
    
    def __init__(
        self,
        Cs: float = 0.5,
        Kc: float = 0.5,
        T: float = 1.0,
        tp: float = 1.0,
        speed: float = 1.0,
        desired_trajectory_params: Dict = None,
        vehicle_config: Dict = None,
        log_save_path: Optional[str] = None,
        simulation: bool = True,
        debug: bool = False
    ):
        """
        Initialize haptic shared control system
        
        Args:
            Cs: Scaling factor for torque calculation
            Kc: Preview gain for driver model
            T: Time constant for complex preview model
            tp: Preview time in seconds
            speed: Vehicle speed
            desired_trajectory_params: Parameters of desired trajectory
            vehicle_config: Vehicle configuration
            log_save_path: Path to save log file
            simulation: Whether running in simulation mode
            debug: Enable debug output
        """
        # Control parameters
        self.Cs = Cs
        self.Kc = Kc
        self.T = T
        self.tp = tp
        
        # Vehicle state
        self.speed = speed
        self.steering_ratio = 10
        self.vehicle_config = vehicle_config or {}
        self.vehicle = Vehicle(vehicle_config or {})
        
        # Path planning
        self.desired_trajectory_params = desired_trajectory_params or {"path": [], "tangent": []}
        
        # System state
        self.simulation = simulation
        self.debug = debug
        
        # Set up logging
        self.logger = LoggingManager(log_save_path, debug)
        
        # Initialize state variables that will be populated during calculations
        self._init_state_variables()
    
    def _init_state_variables(self) -> None:
        """Initialize state variables to default values"""
        # Core measurements
        self.r = [0, 0]  # Current position
        self.phi = 0.0  # Current yaw angle (deg)
        self.deltas = [0.0, 0.0]  # Steering angles [FL, FR]
        self.steering_wheel_angle_deg = 0.0
        self.steering_wheel_angle_rad = 0.0
        
        # Derived measurements
        self.turning_radius = 0.0
        self.vehicle_steering_angle_deg = 0.0
        self.vehicle_steering_angle_rad = 0.0
        self.center_of_rotation_to_vehicle = [0, 0]
        
        # Predictions and calculations
        self.e_t = 0.0  # Current error
        self.epsilon_tp_t = 0.0  # Predicted error
        self.rtp = [0, 0]  # Predicted position
        self.center_of_rotation_to_world = [0, 0]
        self.rotating_direction = 0
        self.delta_phi_rad = 0.0
        self.delta_phi_deg = 0.0
        self.predict_yaw_angle_rad = 0.0
        self.predict_yaw_angle_deg = 0.0
        
        # Closest points
        self.closest_pt_rt = [0, 0]
        self.closest_pt_rt_idx = 0
        self.closest_pt_rtp = [0, 0]
        self.closest_pt_rtp_idx = 0
        
        # Control outputs
        self.theta_d_rad = 0.0
        self.theta_d_deg = 0.0
        self.tau_das = 0.0
    
    def calculate_torque(
        self,
        current_position: List[float],
        steering_angles_deg: List[float],
        current_yaw_angle_deg: float,
        steering_wheel_angle_deg: Optional[float] = None,
    ) -> Tuple[float, float, float]:
        """
        Calculate torque for haptic shared control
        
        Args:
            current_position: Current vehicle position [x, y]
            steering_angles_deg: Wheel steering angles [FL, FR] in degrees
            current_yaw_angle_deg: Current yaw angle in degrees
            steering_wheel_angle_deg: Current steering wheel angle in degrees
            
        Returns:
            Tuple of (torque, coefficient, desired_steering_wheel_angle)
        """
        # Store input state
        self.r = current_position
        self.phi = current_yaw_angle_deg
        self.deltas = steering_angles_deg
        
        # 1. Calculate vehicle steering geometry
        turning_data = self.vehicle.calc_turning_radius(steering_angles=self.deltas)
        self.turning_radius = turning_data["R"]
        self.vehicle_steering_angle_deg = turning_data["Delta"]
        self.center_of_rotation_to_vehicle = turning_data["CoR_v"]
        self.vehicle_steering_angle_rad = np.radians(self.vehicle_steering_angle_deg)

        # Use provided steering wheel angle or derive from vehicle angle
        if steering_wheel_angle_deg is None:
            self.steering_wheel_angle_deg = self.translate_sa_to_swa(
                self.vehicle_steering_angle_deg
            )
        else:
            self.steering_wheel_angle_deg = steering_wheel_angle_deg
        self.steering_wheel_angle_rad = np.radians(self.steering_wheel_angle_deg)

        # 2. Calculate error to trajectory
        if len(self.desired_trajectory_params.get("path", [])) > 0:
            self.e_t, self.closest_pt_rt, self.closest_pt_rt_idx = distance_to_trajectory(
                given_position=current_position,
                desired_trajectory=self.desired_trajectory_params["path"]
            )
        else:
            self.e_t = 0.0

        # 3. Calculate preview driver model
        self.theta_d_rad = self._preview_driver_model()
        self.theta_d_deg = np.degrees(self.theta_d_rad)

        # 4. Calculate torque
        self.tau_das = -(self.Cs * self.e_t) * (self.steering_wheel_angle_rad - self.theta_d_rad)
        
        # Calculate coefficient based on sigmoid function
        coef = sigmoid(self.Cs * self.e_t)
        
        # Log all data
        self._log_data()

        return self.tau_das, coef, self.theta_d_deg

    def _preview_driver_model(self, method: str = "simple") -> float:
        """
        Predict desired steering angle using preview driver model
        
        Args:
            method: Method to use ("simple" or "complex")
            
        Returns:
            Desired steering angle in radians
        """
        # Skip if no trajectory is available
        if not self.desired_trajectory_params or "path" not in self.desired_trajectory_params:
            return self.steering_wheel_angle_rad
            
        # 1. Predict position at preview time
        prediction = predict_position(
            current_position=self.r,
            current_yaw_angle_deg=self.phi,
            vehicle_steering_angle_deg=self.vehicle_steering_angle_deg,
            center_of_rotation_to_vehicle=self.center_of_rotation_to_vehicle,
            turning_radius=self.turning_radius,
            look_ahead_distance=look_ahead_distance(
                vel=self.speed, 
                tp=self.tp, 
                cur_pos=self.r,
                end_pos=[self.desired_trajectory_params["end_x"], 
                         self.desired_trajectory_params["end_y"]],
                method="fix"
            ),
        )
        
        # Store prediction results
        self.rtp = prediction["predicted_position"]
        self.rotating_direction = prediction["rotating_direction"]
        self.delta_phi_rad = prediction["delta_phi_rad"]
        self.predict_yaw_angle_rad = prediction["predict_yaw_angle_rad"]
        self.center_of_rotation_to_world = prediction["center_of_rotation_to_world"]
        
        self.delta_phi_deg = np.degrees(self.delta_phi_rad)
        self.predict_yaw_angle_deg = np.degrees(self.predict_yaw_angle_rad)
        
        # 2. Calculate error between predicted position and trajectory
        has_trajectory = len(self.desired_trajectory_params.get("path", [])) > 0
        
        if has_trajectory:
            # Get distance to trajectory
            epsilon_tp_t, self.closest_pt_rtp, self.closest_pt_rtp_idx = distance_to_trajectory(
                given_position=self.rtp,
                desired_trajectory=self.desired_trajectory_params["path"]
            )
            
            # Determine sign of error
            sign_factor = get_sign_of_error(
                given_position=self.rtp,
                desired_trajectory=self.desired_trajectory_params["path"],
                tangent_vectors=self.desired_trajectory_params["tangent"],
                velocity=self.speed,
            )
            
            self.epsilon_tp_t = sign_factor * epsilon_tp_t
        else:
            self.epsilon_tp_t = 0.0

        # 3. Calculate desired steering angle based on method
        if method == "simple":
            # Simple proportional control
            theta_d_rad = self.Kc * self.epsilon_tp_t + self.steering_wheel_angle_rad
        else:
            # More complex model with time constant
            theta_d_rad_curr = self.steering_wheel_angle_rad
            update_frequency = 60  # 60 Hz
            d_t = 1 / update_frequency
            
            # Iteratively calculate the desired steering angle
            for _ in range(update_frequency):
                theta_d_rad_next = (
                    self.Kc * self.epsilon_tp_t + ((self.T / d_t) - 1) * theta_d_rad_curr
                ) * (d_t / self.T)
                theta_d_rad_curr = theta_d_rad_next
                
            theta_d_rad = theta_d_rad_curr

        return theta_d_rad

    def translate_sa_to_swa(self, sa_deg: float) -> float:
        """
        Translate steering angle to steering wheel angle
        
        Args:
            sa_deg: Steering angle in degrees
            
        Returns:
            Steering wheel angle in degrees
        """
        if not self.simulation:
            return linear_fn(
                slope=self.steering_ratio,
                intercept=0.0
            )(sa_deg)
        return linear_fn(1, 0)(sa_deg)

    def translate_swa_to_sa(self, swa_deg: float) -> float:
        """
        Translate steering wheel angle to steering angle
        
        Args:
            swa_deg: Steering wheel angle in degrees
            
        Returns:
            Steering angle in degrees
        """
        if not self.simulation:
            return linear_fn(
                slope=1/self.steering_ratio, 
                intercept=1.8142561990902186e-05
            )(swa_deg)
        return linear_fn(1, 0)(swa_deg)

    def _log_data(self) -> None:
        """Log all calculated data to the logging manager"""
        # Prepare data for logging
        log_data = {
            "[Measured] Speed ~ V(t) (m/s)": self.speed,
            "[Measured] Current Position ~ X(t) (m)": self.r[0],
            "[Measured] Current Position ~ Y(t) (m)": self.r[1],
            "[Measured] Steering Angle FL ~ delta_fl(t) (deg)": self.deltas[0],
            "[Measured] Steering Angle FR ~ delta_fr(t) (deg)": self.deltas[1],
            "[Measured] Current Yaw Angle ~ Phi(t) (deg)": self.phi,
            "[Measured] Steering Wheel Angle SWA ~ Theta(t) (deg)": self.steering_wheel_angle_deg,
            "[Computed] Rotation direction ~ rotate(t) (CW/CCW)": self.rotating_direction,
            "[Computed] Turning Radius ~ R(delta) (m)": self.turning_radius,
            "[Computed] Vehicle Steering Angle ~ Delta_v(t) (deg)": self.vehicle_steering_angle_deg,
            "[Computed] Current Error to Trajectory ~ e(t) (m)": self.e_t,
            "[Computed] Predicted Position ~ X_tp(t) (m)": self.rtp[0],
            "[Computed] Predicted Position ~ Y_tp(t) (m)": self.rtp[1],
            "[Computed] Change of Yaw Angle ~ Delta_Phi(t) (deg)": self.delta_phi_deg,
            "[Computed] Center of Rotation to WORLD ~ X_c(t) (m)": self.center_of_rotation_to_world[0],
            "[Computed] Center of Rotation to WORLD ~ Y_c(t) (m)": self.center_of_rotation_to_world[1],
            "[Computed] Predicted Yaw Angle ~ Phi_tp(t) (deg)": self.predict_yaw_angle_deg,
            "[Computed] Predicted Error to Trajectory ~ eps_tp(t) (m)": self.epsilon_tp_t,
            "[Computed] Desired Steering Wheel Angle ~ Theta_d(t) (deg)": self.theta_d_deg,
            "[Computed] Torque applied ~ Tau_das (N.m)": self.tau_das,
        }
        
        # Pass to logger
        self.logger.log(log_data)


if __name__ == "__main__":
    pass