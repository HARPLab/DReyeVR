"""
Utility module for vehicle dynamics and geometry calculations.

This module provides utility functions and classes for:
- Vehicle dynamics and steering geometry
- Path processing and trajectory calculations
- General mathematical and geometric operations
"""

import math
from typing import Any, Callable, Dict, List, Optional, Tuple, Union

import numpy as np
from scipy.spatial import KDTree

# =====================
# Mathematical Utilities
# =====================

def cot(x: float) -> float:
    """
    Calculate cotangent of angle.
    
    Args:
        x: Angle in radians
        
    Returns:
        Cotangent value
    """
    return 1 / np.tan(x)


def arccot(x: float) -> float:
    """
    Calculate inverse cotangent (arccot) of value.
    
    Args:
        x: Value to find arccot for
        
    Returns:
        Angle in radians
    """
    return np.arctan(1 / x)


def sigmoid(x: float) -> float:
    """
    Sigmoid activation function.
    
    Args:
        x: Input value
        
    Returns:
        Sigmoid result in range (0, 1)
    """
    return 1 / (1 + np.exp(-x))


def dist(p1: Union[List[float], np.ndarray], 
        p2: Union[List[float], np.ndarray]) -> float:
    """
    Calculate Euclidean distance between two points.
    
    Args:
        p1: First point as [x, y]
        p2: Second point as [x, y]
        
    Returns:
        Euclidean distance
    """
    return np.linalg.norm(np.array(p1) - np.array(p2))


def sign(x: float) -> float:
    """
    Get sign of a value (+1, -1, or 0).
    
    Args:
        x: Input value
        
    Returns:
        Sign of input: +1, -1, or 0
    """
    return x / np.abs(x) if x != 0 else 0


def linear_fn(slope: float, intercept: float) -> Callable[[float], float]:
    """
    Create a linear function (y = mx + b).
    
    Args:
        slope: Slope (m)
        intercept: Y-intercept (b)
        
    Returns:
        Function that computes y = slope * x + intercept
    """
    return lambda x: slope * x + intercept


# =====================
# Geometric Utilities
# =====================

def getAngle(a: List[float], b: List[float], c: List[float], degrees: bool = False) -> float:
    """
    Calculate the angle between three points (angle at point b).
    
    Args:
        a: First point [x, y]
        b: Middle point [x, y] (vertex of angle)
        c: Third point [x, y]
        degrees: If True, return angle in degrees, otherwise in radians
        
    Returns:
        Angle in degrees or radians
    """
    angle_rad = math.atan2(c[1] - b[1], c[0] - b[0]) - math.atan2(a[1] - b[1], a[0] - b[0])
    return math.degrees(angle_rad) if degrees else angle_rad


def rotation_matrix_cw(angle_rad: float) -> np.ndarray:
    """
    Create a 2D clockwise rotation matrix.
    
    Args:
        angle_rad: Rotation angle in radians
        
    Returns:
        2x2 rotation matrix
    """
    return np.array([
        [np.cos(angle_rad), -np.sin(angle_rad)], 
        [np.sin(angle_rad), np.cos(angle_rad)]
    ])


def rotation_matrix_ccw(angle_rad: float) -> np.ndarray:
    """
    Create a 2D counter-clockwise rotation matrix.
    
    Args:
        angle_rad: Rotation angle in radians
        
    Returns:
        2x2 rotation matrix
    """
    return np.array([
        [np.cos(angle_rad), np.sin(angle_rad)], 
        [-np.sin(angle_rad), np.cos(angle_rad)]
    ])


def find_closest_point(given_point: Union[List[float], np.ndarray], 
                       list_of_points: Union[List[List[float]], np.ndarray], 
                       method: str = "Brute") -> Tuple[np.ndarray, int]:
    """
    Find the closest point in a list to a given point.
    
    Args:
        given_point: The reference point [x, y]
        list_of_points: List of points to search
        method: Search method ('KDTree' or 'linear')
        
    Returns:
        Tuple containing (closest_point, index)
    """
    # Convert inputs to numpy arrays
    given_point_np = np.array(given_point)
    points_np = np.array(list_of_points)
    
    if method == "KDTree":
        # Use KD-Tree for efficient nearest neighbor search
        tree = KDTree(points_np)
        _, nnidx = tree.query(given_point_np, k=1)
    else:
        # Use linear search (slower but no dependencies)
        distances = np.linalg.norm(points_np - given_point_np, axis=1)
        nnidx = np.argmin(distances)
    
    return list_of_points[nnidx], nnidx


# =====================
# Path Processing
# =====================

def process_exist_path(path_points: np.ndarray) -> Dict[str, Any]:
    """
    Process a sequence of path points to extract path properties.
    
    Args:
        path_points: Array of path coordinates [[x1, y1], [x2, y2], ...]
        
    Returns:
        Dictionary containing processed path data:
        - 'path': Array of points
        - 'tangent': Array of tangent vectors
        - 'end_x', 'end_y': End point coordinates
    """
    if len(path_points) < 3:
        raise ValueError("Path must have at least 3 points to process")
    
    # Extract path points
    path_x = path_points[:, 0]
    path_y = path_points[:, 1]
    
    # Calculate tangent vectors (p1, p2, p3) for each point
    tangent_vectors = []
    for i in range(len(path_points)):
        if i == 0:  # First point
            p1 = path_points[i]
            p2 = path_points[i]
            p3 = path_points[i+1]
        elif i == len(path_points) - 1:  # Last point
            p1 = path_points[i-1]
            p2 = path_points[i]
            p3 = path_points[i]
        else:  # Middle points
            p1 = path_points[i-1]
            p2 = path_points[i]
            p3 = path_points[i+1]
        
        tangent_vectors.append((p1, p3))
    
    # Return processed path data
    return {
        "path": path_points,
        "tangent": np.array(tangent_vectors),
        "end_x": path_x[-1],
        "end_y": path_y[-1]
    }


# =====================
# Vehicle Class
# =====================

class Vehicle:
    """
    Vehicle model for dynamics and steering calculations.
    
    This class encapsulates vehicle parameters and provides methods
    to calculate turning radius, steering angles, and other dynamics.
    """
    
    def __init__(self, vehicle_config: Dict[str, Any], model: str = "simple") -> None:
        """
        Initialize vehicle model.
        
        Args:
            vehicle_config: Dictionary with vehicle configuration
            model: Vehicle model type ('simple' or 'ackermann')
        """
        self.vehicle_config = vehicle_config
        self.vehicle_model = model.lower()
        
        # Default parameters
        self.wheelbase = 2.7  # meters
        self.tire_width = 1.8  # meters
        self.center_of_mass_ratio = 0.45  # ratio of wheelbase
        self.center_of_mass = 0.0  # calculated from wheelbase
        self.minimum_turning_radius = 5.0  # meters
        
        # Maximum steering angles
        self.max_steering_angle_inner_deg = 69.99999237060547
        self.max_steering_angle_outer_deg = 47.425662994384766
        
        # Initialize model from config
        self._init_vehicle_model()
    
    def _init_vehicle_model(self) -> None:
        """
        Initialize vehicle parameters from configuration.
        
        Extracts wheelbase, tire width, and center of mass
        from the vehicle configuration.
        """
        # Check if we have a valid configuration
        if not self.vehicle_config or "wheels" not in self.vehicle_config:
            return
        
        try:
            # Calculate wheelbase (distance between front and rear axles)
            self.wheelbase = (
                abs(
                    self.vehicle_config["wheels"][0]["position"]["y"] -
                    self.vehicle_config["wheels"][2]["position"]["y"]
                ) / 100.0  # Convert from cm to meters
            )
            
            # Calculate tire width (track width)
            self.tire_width = (
                abs(
                    self.vehicle_config["wheels"][1]["position"]["x"] -
                    self.vehicle_config["wheels"][0]["position"]["x"]
                ) / 100.0  # Convert from cm to meters
            )
            
            # Get center of mass position relative to wheelbase
            self.center_of_mass_ratio = self.vehicle_config.get("center_of_mass", {}).get("x", 0.45)
            
            # Calculate center of mass position in meters
            self.center_of_mass = self.wheelbase * self.center_of_mass_ratio
            
            # Calculate minimum turning radius using maximum steering angles
            self.minimum_turning_radius = self.calc_turning_radius(
                [self.max_steering_angle_inner_deg, self.max_steering_angle_outer_deg]
            )["R"]
            
        except (KeyError, IndexError, TypeError) as e:
            print(f"Error initializing vehicle model: {e}")
    
    def calc_turning_radius(self, steering_angles: List[float]) -> Dict[str, Any]:
        """
        Calculate vehicle turning radius and steering parameters.
        
        This function implements two steering models:
        1. Simple: Basic bicycle model using outer wheel angle
        2. Ackermann: Uses both inner and outer wheel angles for more accuracy
        
        Args:
            steering_angles: List of [left_wheel_angle, right_wheel_angle] in degrees
            
        Returns:
            Dictionary with calculated parameters:
            - R: Turning radius (meters)
            - Delta: Vehicle steering angle (degrees)
            - CoR_v: Center of rotation in vehicle coordinates [x, y]
        """
        # Determine inner and outer wheel based on steering direction
        if steering_angles[0] > steering_angles[1]:  # Turning right
            inner_wheel_angle_deg = steering_angles[1]
            outer_wheel_angle_deg = steering_angles[0]
        else:  # Turning left
            inner_wheel_angle_deg = steering_angles[0]
            outer_wheel_angle_deg = steering_angles[1]
        
        # Convert to radians
        inner_wheel_angle_rad = np.radians(inner_wheel_angle_deg)
        outer_wheel_angle_rad = np.radians(outer_wheel_angle_deg)
        
        # Initialize variables
        turning_radius = 0.0
        vehicle_steering_angle_rad = 0.0
        
        # Calculate using selected model
        if self.vehicle_model == "simple":
            # Use the simple bicycle model
            # Based on outer wheel angle for better stability
            steering_angle_rad = outer_wheel_angle_rad
            
            # Avoid division by zero at exactly 0 or 180 degrees
            if abs(steering_angle_rad % np.pi) < 1e-6:
                steering_angle_rad += 1e-6
            
            # Calculate turning radius using bicycle model formula
            turning_radius = np.sqrt(
                self.center_of_mass**2 + 
                (self.wheelbase**2) / np.tan(steering_angle_rad)**2
            )
            
            # Calculate effective vehicle steering angle
            vehicle_steering_angle_rad = np.arctan(
                self.center_of_mass_ratio * np.tan(steering_angle_rad)
            )
            
        else:
            # Use the Ackermann steering model for more accuracy
            # Avoid division by zero
            if abs(inner_wheel_angle_rad % np.pi) < 1e-6:
                inner_wheel_angle_rad += 1e-6
            if abs(outer_wheel_angle_rad % np.pi) < 1e-6:
                outer_wheel_angle_rad += 1e-6
            
            # Calculate effective steering angle using Ackermann formula
            steering_angle_rad = arccot(
                (cot(inner_wheel_angle_rad) + cot(outer_wheel_angle_rad)) / 2
            )
            
            # Calculate turning radius
            turning_radius = np.sqrt(
                self.center_of_mass**2 + 
                (self.wheelbase**2) * (cot(steering_angle_rad)**2)
            )
            
            # Calculate effective vehicle steering angle
            vehicle_steering_angle_rad = steering_angle_rad
        
        # Calculate center of rotation in vehicle coordinates
        # Avoid division by zero
        if abs(vehicle_steering_angle_rad) < 1e-6:
            cor_y = float('inf')  # Infinite turning radius (straight line)
        else:
            cor_y = self.center_of_mass / np.tan(vehicle_steering_angle_rad)
            
        center_of_rotation = np.array([-self.center_of_mass, cor_y])
        
        # Return calculated parameters
        return {
            "R": abs(turning_radius),
            "Delta": np.degrees(vehicle_steering_angle_rad),
            "CoR_v": center_of_rotation
        }


# Testing the utilities
if __name__ == "__main__":
    # Test angle calculation
    a = (5, 0)
    b = (0, 0)
    c = (0, -5)
    angle_deg = getAngle(a, b, c, degrees=True)
    print(f"Angle between points: {angle_deg} degrees")
    
    # Test vehicle model
    test_config = {
        "wheels": [
            {"position": {"x": -75, "y": 130}},  # Front left
            {"position": {"x": 75, "y": 130}},   # Front right
            {"position": {"x": -75, "y": -130}}, # Rear left
            {"position": {"x": 75, "y": -130}}   # Rear right
        ],
        "center_of_mass": {"x": 0.45}
    }
    
    vehicle = Vehicle(test_config)
    print(f"Vehicle wheelbase: {vehicle.wheelbase} m")
    print(f"Vehicle track width: {vehicle.tire_width} m")
    
    # Test turning radius calculation
    steering_angles = [20.0, 15.0]  # Left and right wheel angles
    turning_data = vehicle.calc_turning_radius(steering_angles)
    print(f"Turning radius: {turning_data['R']:.2f} m")
    print(f"Vehicle steering angle: {turning_data['Delta']:.2f} degrees")
    print(f"Center of rotation: {turning_data['CoR_v']}")