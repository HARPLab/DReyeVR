import json
import os
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
from HapticSharedControl.path_planning import calculate_bezier_trajectory
from HapticSharedControl.utils import Vehicle, process_exist_path


class ConfigManager:
    """Manages configuration and path data for the DReyeVR Haptic Control system"""
    
    def __init__(self, config_path: Optional[str] = None, paths_path: Optional[str] = None):
        """
        Initialize the configuration manager
        
        Args:
            config_path: Path to the config JSON file
            paths_path: Path to the paths JSON file
        """
        self.base_dir = Path(__file__).resolve().parent
        
        # Set default paths if not provided
        config_path = config_path or str(self.base_dir / "HapticSharedControl" / "config.json")
        paths_path = paths_path or str(self.base_dir / "HapticSharedControl" / "paths.json")
        
        # Load configurations
        self.config = self._load_json(config_path)
        self.paths_config = self._load_json(paths_path)
        
        # Load vehicle config
        self.vehicle_config = self._load_json(
            str(self.base_dir / "HapticSharedControl" / "wheel_setting.json")
        )
        self.vehicle = Vehicle(vehicle_config=self.vehicle_config)
        
        # Process paths to create usable trajectory data
        self.predefined_path = self._process_paths()
    
    def _load_json(self, file_path: str) -> Dict[str, Any]:
        """
        Load JSON configuration from file
        
        Args:
            file_path: Path to JSON file
            
        Returns:
            Dictionary containing configuration data
        """
        try:
            with open(file_path, "r") as f:
                return json.load(f)
        except Exception as e:
            print(f"Error loading config from {file_path}: {e}")
            return {}
    
    def _load_path_data(self, file_path: str) -> np.ndarray:
        """
        Load path data from text file
        
        Args:
            file_path: Path to the text file containing path data
            
        Returns:
            NumPy array of path coordinates
        """
        try:
            path = Path(file_path)
            # Resolve relative paths
            if not path.is_absolute():
                path = self.base_dir / path
                
            with open(path, "r") as f:
                data = f.readlines()
                data = [line.strip().split(",") for line in data]
                data = [[float(val) for val in line] for line in data]
                return np.array(data)
        except Exception as e:
            print(f"Error loading path data from {file_path}: {e}")
            return np.array([])
    
    def _extract_path_range(self, 
                          data: np.ndarray, 
                          range_spec: List[int]) -> np.ndarray:
        """
        Extract a range of points from path data
        
        Args:
            data: Array of path data
            range_spec: [start_idx, end_idx] where end_idx can be -1 for end
            
        Returns:
            Subset of path data
        """
        start, end = range_spec
        if end == -1 or end is None:
            return data[start:]
        return data[start:end]
    
    def _process_paths(self) -> Dict[str, Dict[str, Any]]:
        """
        Process all paths from configuration
        
        Returns:
            Dictionary of processed path data
        """
        processed_paths = {}
        
        for path_idx, path_config in self.paths_config.items():
            processed_path = {
                "P_0": path_config.get("P_0", []),
                "P_d": path_config.get("P_d", []),
                "P_f": path_config.get("P_f", []),
                "yaw_0": path_config.get("yaw_0"),
                "yaw_d": path_config.get("yaw_d"),
                "yaw_f": path_config.get("yaw_f"),
                "forward paths": None,
                "backward paths": None,
            }
            
            # Process forward path if specified
            if "forward_path_file" in path_config:
                data = self._load_path_data(path_config["forward_path_file"])
                if len(data) > 0 and "forward_path_range" in path_config:
                    range_data = self._extract_path_range(data, path_config["forward_path_range"])
                    processed_path["forward paths"] = process_exist_path(range_data)
            
            # Process backward path if specified
            if "backward_path_file" in path_config:
                data = self._load_path_data(path_config["backward_path_file"])
                if len(data) > 0 and "backward_path_range" in path_config:
                    range_data = self._extract_path_range(data, path_config["backward_path_range"])
                    processed_path["backward paths"] = process_exist_path(range_data)
            
            # Generate Bezier curve if specified
            if path_config.get("use_bezier", False):
                self._generate_bezier_curves(processed_path)
                
            processed_paths[path_idx] = processed_path
            
        return processed_paths
    
    def _generate_bezier_curves(self, path_data: Dict[str, Any]) -> None:
        """
        Generate Bezier curves for a path
        
        Args:
            path_data: Path data dictionary to be updated with Bezier curves
        """
        n_points = 60
        min_radius = self.vehicle.minimum_turning_radius
        
        # Check for required points
        if (not path_data["P_0"] or not path_data["P_d"] or 
            path_data["yaw_0"] is None or path_data["yaw_d"] is None):
            return
        
        # Generate forward path
        _, _, param_fw = calculate_bezier_trajectory(
            start_pos=path_data["P_0"][::-1],
            end_pos=path_data["P_d"][::-1],
            start_yaw=path_data["yaw_0"],
            end_yaw=path_data["yaw_d"],
            n_points=n_points,
            turning_radius=min_radius,
            show_animation=False,
        )
        
        # Generate backward path
        _, _, param_bw = calculate_bezier_trajectory(
            start_pos=path_data["P_d"][::-1],
            end_pos=path_data["P_f"][::-1],
            start_yaw=path_data["yaw_d"] + 180,  # reverse yaw angle
            end_yaw=path_data["yaw_f"] + 180,    # reverse yaw angle
            n_points=n_points,
            turning_radius=min_radius,
            show_animation=False,
        )
        
        path_data["forward paths"] = param_fw
        path_data["backward paths"] = param_bw
    
    def get_config(self, key: str, default: Any = None) -> Any:
        """
        Get a value from the main configuration
        
        Args:
            key: Configuration key (can use dot notation for nested keys)
            default: Default value if key not found
            
        Returns:
            Configuration value
        """
        if "." in key:
            parts = key.split(".")
            config = self.config
            for part in parts:
                if part in config:
                    config = config[part]
                else:
                    return default
            return config
        return self.config.get(key, default)
    
    def get_path(self, path_idx: str) -> Dict[str, Any]:
        """
        Get processed path data by index
        
        Args:
            path_idx: Path index
            
        Returns:
            Processed path data dictionary
        """
        return self.predefined_path.get(path_idx, {})
    
    def get_default_path_idx(self) -> str:
        """
        Get the default path index from configuration
        
        Returns:
            Default path index
        """
        return self.get_config("default_path_idx", "3")
    
    def get_vehicle(self) -> Vehicle:
        """
        Get the vehicle instance
        
        Returns:
            Vehicle instance
        """
        return self.vehicle
    
    def get_force_feedback_settings(self) -> Tuple[int, int]:
        """
        Get force feedback settings
        
        Returns:
            Tuple of (saturation_percentage, coefficient_percentage)
        """
        saturation = self.get_config("force_feedback.saturation_percentage", 100)
        coefficient = self.get_config("force_feedback.coefficient_percentage", 30)
        return saturation, coefficient
    
    def get_haptic_control_settings(self) -> Dict[str, float]:
        """
        Get haptic control settings
        
        Returns:
            Dictionary of haptic control parameters
        """
        return {
            "Cs": self.get_config("haptic_control.Cs", 0.5),
            "Kc": self.get_config("haptic_control.Kc", 0.5),
            "tp": self.get_config("haptic_control.tp", 12.0)
        }
    
    def get_logging_dir(self) -> Path:
        """
        Get logging directory
        
        Returns:
            Path object for logging directory
        """
        log_dir = self.get_config("logging.log_dir", "./logs")
        path = Path(log_dir)
        if not path.is_absolute():
            path = self.base_dir / path
        os.makedirs(path, exist_ok=True)
        return path