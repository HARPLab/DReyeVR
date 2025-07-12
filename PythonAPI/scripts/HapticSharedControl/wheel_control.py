"""
Logitech G920 Driving Force Racing Wheel Control Module

This module provides a robust interface to the Logitech G920 steering wheel controller,
handling force feedback, button mapping, and pedal input with comprehensive error handling.
"""

import ctypes
import gc
import logging
import sys
import time
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple, Union

import numpy as np
import pandas as pd

# Add logidrivepy to path
sys.path.append(str(Path(__file__).parent.parent / "logidrivepy"))
from logidrivepy import LogitechController

# Constants
MAX_ANGLE_RANGE = 900  # Maximum rotation range in degrees
DEFAULT_SATURATION = 50
DEFAULT_COEFFICIENT = 100
AXIS_MAX_VALUE = 32767.0  # Maximum raw axis value

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("WheelController")


class WheelController:
    """
    Controller interface for Logitech G920 Driving Force Racing Wheel.
    
    This class provides methods to interact with the G920 wheel, including reading wheel position,
    pedal states, button presses, and applying force feedback effects.
    """
    
    def __init__(self, controller_index: int = 0, log_level: int = logging.INFO):
        """
        Initialize the wheel controller.
        
        Args:
            controller_index: Index of the controller (default: 0)
            log_level: Logging level (default: INFO)
        
        Raises:
            RuntimeError: If controller initialization fails
        """
        # Set up logging
        logger.setLevel(log_level)
        
        # Initialize controller
        self.controller_index = controller_index
        self.controller = LogitechController()
        
        # Initialize wheel
        init_success = self.controller.steering_initialize()
        if not init_success:
            raise RuntimeError("Failed to initialize Logitech controller. Check connection and drivers.")
            
        # Set wheel operating range
        self.controller.set_operating_range(index=self.controller_index, range=MAX_ANGLE_RANGE)
        
        # Cache for reducing state queries
        self._state_cache = None
        self._last_update_time = 0
        self._cache_validity_ms = 10  # Cache valid for 10ms
        
        # Log initialization details
        self._log_controller_info()
    
    def _log_controller_info(self) -> None:
        """Log controller initialization details"""
        logger.info("Logitech Controller Initialized")
        logger.info(f"Connected: {self.controller.is_connected(index=self.controller_index)}")
        logger.info(f"Force feedback available: {self.controller.has_force_feedback(index=self.controller_index)}")
        
        operating_range = self.controller.get_operating_range(
            index=self.controller_index, 
            range=ctypes.c_int()
        )
        logger.info(f"Operating range: {operating_range} degrees")
    
    def _update_state(self, force: bool = False) -> None:
        """
        Update the controller state.
        
        Args:
            force: Force update even if cache is valid
        """
        current_time = time.time() * 1000  # Convert to milliseconds
        
        # Update if cache is invalid or forced update
        if force or (current_time - self._last_update_time > self._cache_validity_ms):
            self.controller.logi_update()
            self._state_cache = self.controller.get_state_engines(self.controller_index).contents
            self._last_update_time = current_time
    
    def play_spring_force(
        self, 
        offset_percentage: int, 
        saturation_percentage: int = DEFAULT_SATURATION, 
        coefficient_percentage: int = DEFAULT_COEFFICIENT
    ) -> bool:
        """
        Apply spring force effect to the steering wheel.
        
        Args:
            offset_percentage: Center point offset (-100 to 100)
            saturation_percentage: Force saturation (0 to 100)
            coefficient_percentage: Spring stiffness (0 to 100)
        
        Returns:
            bool: Success status
        
        Notes:
            - offset_percentage: Controls the center position of the spring
              -100 = full left, 0 = center, 100 = full right
            - saturation_percentage: Controls maximum force applied
              0 = no force, 100 = maximum force
            - coefficient_percentage: Controls spring stiffness
              0 = no resistance, 100 = maximum resistance
        """
        try:
            self._update_state(force=True)
            
            # Ensure values are within valid ranges
            offset = max(-100, min(100, int(offset_percentage)))
            saturation = max(0, min(100, int(saturation_percentage)))
            coefficient = max(-100, min(100, int(coefficient_percentage)))
            
            if coefficient < 0:
                logger.warning(f"Coefficient {coefficient} will make the steering wheel rotate reversly")

            success = self.controller.play_spring_force(
                index=self.controller_index,
                offset_percentage=offset,
                saturation_percentage=saturation,
                coefficient_percentage=coefficient,
            )
            
            if not success:
                logger.warning(f"Failed to apply spring force: {offset}, {saturation}, {coefficient}")
                
            return success
            
        except Exception as e:
            logger.error(f"Error applying spring force: {e}")
            return False
    
    def stop_spring_force(self) -> bool:
        """
        Stop spring force feedback effect.
        
        Returns:
            bool: Success status
        """
        try:
            self._update_state(force=True)
            success = self.controller.stop_spring_force(index=self.controller_index)
            
            if not success:
                logger.warning("Failed to stop spring force")
                
            return success
            
        except Exception as e:
            logger.error(f"Error stopping spring force: {e}")
            return False
    
    def get_state_engines(self) -> Dict[str, Any]:
        """
        Get complete controller state including all axes, buttons, and forces.
        
        Returns:
            Dict containing all controller state parameters
        """
        try:
            self._update_state()
            
            # Create dictionary of all state values
            return {
                # Position axes
                "lX": self._state_cache.lX,
                "lY": self._state_cache.lY,
                "lZ": self._state_cache.lZ,
                "lRx": self._state_cache.lRx,
                "lRy": self._state_cache.lRy,
                "lRz": self._state_cache.lRz,
                "rglSlider": list(self._state_cache.rglSlider),
                
                # D-pad and buttons
                "rgdwPOV": list(self._state_cache.rgdwPOV),
                "rgbButtons": list(self._state_cache.rgbButtons),
                
                # Velocity
                "lVX": self._state_cache.lVX,
                "lVY": self._state_cache.lVY,
                "lVZ": self._state_cache.lVZ,
                "lVRx": self._state_cache.lVRx,
                "lVRy": self._state_cache.lVRy,
                "lVRz": self._state_cache.lVRz,
                "rglVSlider": list(self._state_cache.rglVSlider),
                
                # Acceleration
                "lAX": self._state_cache.lAX,
                "lAY": self._state_cache.lAY,
                "lAZ": self._state_cache.lAZ,
                "lARx": self._state_cache.lARx,
                "lARy": self._state_cache.lARy,
                "lARz": self._state_cache.lARz,
                "rglASlider": list(self._state_cache.rglASlider),
                
                # Force
                "lFX": self._state_cache.lFX,
                "lFY": self._state_cache.lFY,
                "lFZ": self._state_cache.lFZ,
                "lFRx": self._state_cache.lFRx,
                "lFRy": self._state_cache.lFRy,
                "lFRz": self._state_cache.lFRz,
                "rglFSlider": list(self._state_cache.rglFSlider),
            }
            
        except Exception as e:
            logger.error(f"Error getting state: {e}")
            # Return empty state on error
            return {}
    
    def get_angle(self, translate: bool = True) -> float:
        """
        Get steering wheel angle.
        
        Args:
            translate: If True, normalize to [-1.0, 1.0] range
        
        Returns:
            float: Wheel angle (normalized or raw)
        """
        try:
            self._update_state()
            
            raw_value = self._state_cache.lX
            
            if not translate:
                return raw_value
            else:
                return np.clip(raw_value / AXIS_MAX_VALUE, -1.0, 1.0)
                
        except Exception as e:
            logger.error(f"Error getting wheel angle: {e}")
            return 0.0
    
    def get_acceleration_pedal(self, translate: bool = True) -> float:
        """
        Get acceleration pedal position.
        
        Args:
            translate: If True, normalize to [0, 1] range
        
        Returns:
            float: Pedal position (normalized or raw)
        """
        try:
            self._update_state()
            
            raw_value = self._state_cache.lY
            
            if not translate:
                return raw_value
            else:
                # Map from [-32767, 32767] to [0, 1]
                # Note: For acceleration, higher raw value = more pressed
                return np.clip((raw_value + AXIS_MAX_VALUE) / (2 * AXIS_MAX_VALUE), 0.0, 1.0)
                
        except Exception as e:
            logger.error(f"Error getting acceleration pedal: {e}")
            return 0.0
    
    def get_brake_pedal(self, translate: bool = True) -> float:
        """
        Get brake pedal position.
        
        Args:
            translate: If True, normalize to [0, 1] range
        
        Returns:
            float: Pedal position (normalized or raw)
        """
        try:
            self._update_state()
            
            raw_value = self._state_cache.lRz
            
            if not translate:
                return raw_value
            else:
                # Brake is fully released at 32767, fully pressed at -32768
                # Map to [0, 1] where 1 is fully pressed
                return np.clip(abs(raw_value - AXIS_MAX_VALUE) / (2 * AXIS_MAX_VALUE), 0.0, 1.0)
                
        except Exception as e:
            logger.error(f"Error getting brake pedal: {e}")
            return 0.0
    
    def get_buttons_pressed(self) -> Dict[str, Union[str, bool]]:
        """
        Get status of wheel buttons and D-pad.
        
        Returns:
            Dict containing button states:
                - "D-pad": Direction ("up", "right", "down", "left", "center")
                - "A", "B", "X", "Y": Boolean button states
        """
        try:
            self._update_state()
            
            # Extract D-pad and button values
            rgdwPOV = list(self._state_cache.rgdwPOV)
            rgbButtons = list(self._state_cache.rgbButtons)
            
            # Process D-pad values
            dpad_direction = self._process_dpad(rgdwPOV[0])
            
            # Process buttons (first 4 buttons: A, B, X, Y)
            button_states = {
                "D-pad": dpad_direction,
                "A": self._process_button(rgbButtons[0]),
                "B": self._process_button(rgbButtons[1]),
                "X": self._process_button(rgbButtons[2]),
                "Y": self._process_button(rgbButtons[3]),
            }
            
            return button_states
            
        except Exception as e:
            logger.error(f"Error getting button states: {e}")
            return {"D-pad": "center", "A": False, "B": False, "X": False, "Y": False}
    
    def _process_dpad(self, value: int) -> str:
        """
        Process D-pad value into direction.
        
        Args:
            value: Raw D-pad value
        
        Returns:
            str: Direction as string
        """
        # Map POV values to directions (in hundredths of degrees)
        if value == 0:
            return "up"
        elif value == 9000:
            return "right"
        elif value == 18000:
            return "down"
        elif value == 27000:
            return "left"
        return "center"
    
    def _process_button(self, value: int) -> bool:
        """
        Process button value to boolean state.
        
        Args:
            value: Raw button value
        
        Returns:
            bool: True if button is pressed
        """
        return value != 0
    
    def exit(self) -> None:
        """
        Clean up controller resources.
        """
        try:
            self._update_state(force=True)
            self.stop_spring_force()
            self.controller.steering_shutdown()
            del self.controller
            gc.collect()
            logger.info("Steering controller shutdown successful")
        except Exception as e:
            logger.error(f"Error during controller shutdown: {e}")


class WheelTester:
    """
    Test utility for the wheel controller.
    
    Provides methods to test different aspects of the wheel controller:
    - Recording states
    - Profiling spring force behavior
    - Testing force feedback parameters systematically
    """
    
    def __init__(self, controller: WheelController):
        """
        Initialize the wheel tester.
        
        Args:
            controller: Wheel controller instance
        """
        self.controller = controller
        self.test_start_time = time.time()
        self.logger = logging.getLogger("WheelTester")
    
    def record_states(self, duration: int = 10, interval: float = 0.5) -> Dict:
        """
        Record and monitor controller states for a specified duration.
        
        Args:
            duration: Recording duration in seconds
            interval: Sampling interval in seconds
        
        Returns:
            Dict of final controller state
        """
        self.logger.info(f"Recording states for {duration} seconds...")
        states = {}
        start_time = time.time()
        
        while time.time() - start_time < duration:
            curr_state = self.controller.get_state_engines()
            
            # Initialize states dict if empty
            if not states:
                states = curr_state.copy()
                continue
            
            # Log changes in state
            for (curr_key, curr_values), (key, value) in zip(curr_state.items(), states.items()):
                if states[key] != curr_values and key != "lAZ":  # Ignore acceleration Z which changes frequently
                    print(f"{key}: {value} --> {curr_values}")
            
            states = curr_state.copy()
            print("----")
            time.sleep(interval)
        
        self.logger.info("State recording completed")
        return states
    
    def test_force_feedback_profile(self, 
                                   offset_range: Tuple[int, int] = (-100, 100),
                                   sat_range: Tuple[int, int] = (10, 100),
                                   coef_range: Tuple[int, int] = (10, 100),
                                   step_size: int = 5) -> pd.DataFrame:
        """
        Test all combinations of force feedback parameters.
        
        Args:
            offset_range: Range of offset values (min, max)
            sat_range: Range of saturation values (min, max)
            coef_range: Range of coefficient values (min, max)
            step_size: Step size for parameter increments
        
        Returns:
            DataFrame of test results
        """
        self.logger.info("Starting comprehensive force feedback test...")
        df_data = {}
        
        # Test all combinations of parameters
        for sat in range(sat_range[0], sat_range[1] + 1, step_size):
            print(f"Saturation: {sat}")
            for coef in range(coef_range[0], coef_range[1] + 1, step_size):
                print(f"  Coefficient: {coef}")
                for offset in range(offset_range[0], offset_range[1] + 1, step_size):
                    print(f"    Offset: {offset}", end="\r")
                    
                    # Apply force feedback
                    self.controller.play_spring_force(
                        offset_percentage=offset,
                        saturation_percentage=sat,
                        coefficient_percentage=coef,
                    )
                    
                    # Record state
                    time.sleep(0.1)  # Allow time for force to apply
                    state = self.controller.get_state_engines()
                    
                    # Store data
                    timestamp = time.time() - self.test_start_time
                    df_data.setdefault("timestamp", []).append(timestamp)
                    df_data.setdefault("offset", []).append(offset)
                    df_data.setdefault("saturation", []).append(sat)
                    df_data.setdefault("coefficient", []).append(coef)
                    df_data.setdefault("wheel_angle", []).append(self.controller.get_angle() * MAX_ANGLE_RANGE / 2)
                    
                    # Add all state parameters
                    for key, value in state.items():
                        if isinstance(value, (int, float)):
                            df_data.setdefault(key, []).append(value)
                        elif isinstance(value, (list, tuple)):
                            # Handle first element of lists (most relevant)
                            if value:
                                df_data.setdefault(f"{key}_0", []).append(value[0])
        
        # Create DataFrame
        df = pd.DataFrame(df_data)
        
        # Save results
        output_file = f"wheel_test_profile_{time.strftime('%Y%m%d_%H%M%S')}.xlsx"
        df.to_excel(output_file, index=False)
        self.logger.info(f"Force feedback test completed. Results saved to {output_file}")
        
        # Stop forces
        self.controller.stop_spring_force()
        
        return df
    
    def test_force_response(self, 
                           offset: int, 
                           saturation: int, 
                           coefficient: int,
                           duration: float = 5.0,
                           sample_rate: float = 100) -> pd.DataFrame:
        """
        Test wheel response to a specific force feedback configuration over time.
        
        Args:
            offset: Center offset value (-100 to 100)
            saturation: Saturation value (0 to 100)
            coefficient: Coefficient value (0 to 100)
            duration: Test duration in seconds
            sample_rate: Samples per second
        
        Returns:
            DataFrame of response data
        """
        self.logger.info(f"Testing response to offset={offset}, sat={saturation}, coef={coefficient}")
        
        # Calculate time step
        time_step = 1.0 / sample_rate
        
        # Initialize result data
        df_data = {
            "timestamp": [],
            "offset": [],
            "saturation": [],
            "coefficient": [],
            "wheel_angle": [],
            "wheel_angle_normalized": [],
            "phase": []
        }
        
        # Phase 1: Reset to center
        self._run_test_phase(
            df_data, "Centering",
            0, 100, 100,  # Full force to center
            2.0, time_step
        )
        
        # Phase 2: Apply test force
        self._run_test_phase(
            df_data, "Testing",
            offset, saturation, coefficient,
            duration, time_step
        )
        
        # Phase 3: Return to center
        self._run_test_phase(
            df_data, "Returning",
            0, 100, 100,  # Full force to center
            2.0, time_step
        )
        
        # Create DataFrame
        df = pd.DataFrame(df_data)
        
        # Save results
        output_file = f"wheel_response_o{offset}_s{saturation}_c{coefficient}_{time.strftime('%Y%m%d_%H%M%S')}.xlsx"
        df.to_excel(output_file, index=False)
        self.logger.info(f"Response test completed. Results saved to {output_file}")
        
        return df
    
    def _run_test_phase(self, 
                      df_data: Dict[str, List], 
                      phase_name: str,
                      offset: int, 
                      saturation: int, 
                      coefficient: int,
                      duration: float,
                      time_step: float) -> None:
        """
        Run a single phase of the response test.
        
        Args:
            df_data: Dictionary to store results
            phase_name: Name of the test phase
            offset: Center offset value
            saturation: Saturation value
            coefficient: Coefficient value
            duration: Phase duration in seconds
            time_step: Sampling interval in seconds
        """
        # Apply force
        self.controller.play_spring_force(
            offset_percentage=offset,
            saturation_percentage=saturation,
            coefficient_percentage=coefficient
        )
        
        # Record data for duration
        start_time = time.time()
        while time.time() - start_time < duration:
            current_time = time.time() - self.test_start_time
            wheel_angle = self.controller.get_angle(translate=False)
            wheel_angle_degrees = self.controller.get_angle(translate=True) * MAX_ANGLE_RANGE / 2
            
            # Store data
            df_data["timestamp"].append(current_time)
            df_data["phase"].append(phase_name)
            df_data["offset"].append(offset)
            df_data["saturation"].append(saturation)
            df_data["coefficient"].append(coefficient)
            df_data["wheel_angle"].append(wheel_angle_degrees)
            df_data["wheel_angle_normalized"].append(wheel_angle / AXIS_MAX_VALUE)
            
            # Sleep for precise timing
            next_sample = start_time + (time.time() - start_time) // time_step * time_step + time_step
            sleep_time = max(0, next_sample - time.time())
            if sleep_time > 0:
                time.sleep(sleep_time)


# Main test functions
def main_test():
    """Run basic controller tests"""
    try:
        # Initialize controller
        controller = WheelController()
        
        # Create tester
        tester = WheelTester(controller)
        
        # Run monitoring test
        print("Starting controller state monitor. Press Ctrl+C to stop.")
        tester.record_states(duration=30)
        
        # Clean up
        controller.exit()
        print("Test completed successfully.")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user.")
        if 'controller' in locals():
            controller.exit()
        
    except Exception as e:
        print(f"Test failed: {e}")
        if 'controller' in locals():
            controller.exit()


def force_feedback_test():
    """Run force feedback tests"""
    try:
        # Initialize controller
        controller = WheelController()
        
        # Create tester
        tester = WheelTester(controller)
        
        # Run targeted tests
        for offset in [-80, -40, 0, 40, 80]:
            tester.test_force_response(
                offset=offset,
                saturation=100,
                coefficient=50,
                duration=3.0
            )
            time.sleep(1)
        
        # Clean up
        controller.exit()
        print("Force feedback test completed successfully.")
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user.")
        if 'controller' in locals():
            controller.exit()
        
    except Exception as e:
        print(f"Test failed: {e}")
        if 'controller' in locals():
            controller.exit()


if __name__ == "__main__":
    # Select test to run
    test_type = "basic"
    
    if test_type == "basic":
        main_test()
    elif test_type == "force":
        force_feedback_test()
    else:
        print(f"Unknown test type: {test_type}")