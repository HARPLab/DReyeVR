# Walker Area Spawn System

This system automatically spawns walkers when a vehicle enters a predefined area. It's designed to work with the DReyeVR simulation environment and runs in asynchronous mode.

## Files

1. **walker_spawn_config.json** - Configuration file defining the trigger area, walker spawn/target positions, and behavior parameters
2. **DReyeVR_area_walker_spawn.py** - Main script that tracks vehicle position and spawns walkers

## Configuration File (walker_spawn_config.json)

### Main Sections:

#### `simulation`

-   `sync_mode`: false (runs in asynchronous mode as requested)
-   `delta_t`: Time step for simulation updates
-   `update_frequency`: How often to check vehicle position

#### `trigger_area`

-   `center`: Center coordinates of the circular trigger area (x, y, z)
-   `radius`: Radius of the trigger area in meters
-   `description`: Human-readable description

#### `walker_spawn`

-   `number_of_walkers`: Maximum number of walkers to spawn
-   `spawn_positions`: Array of starting positions for walkers (x, y, z coordinates)
-   `target_positions`: Array of target positions where walkers will walk to
-   `walker_speed`: Speed configuration (min, max, default values in m/s)
-   `walker_filter`: Blueprint filter for walker types
-   `generation`: Walker generation (1, 2, or "All")
-   `is_invincible`: Whether walkers are invincible

#### `vehicle_tracking`

-   `role_name`: Role name of the vehicle to track (usually "hero")
-   `check_interval`: How often to check vehicle position (seconds)
-   `spawn_once_per_entry`: Whether to spawn walkers only once per area entry

#### `logging`

-   `enable_debug`: Enable debug logging
-   `log_file`: Log file name

## Usage

### Basic Usage:

```bash
python DReyeVR_area_walker_spawn.py
```

### With Custom Configuration:

```bash
python DReyeVR_area_walker_spawn.py --config my_walker_config.json
```

### With Custom CARLA Connection:

```bash
python DReyeVR_area_walker_spawn.py --host 192.168.1.100 --port 2000
```

### Command Line Arguments:

-   `--host`: CARLA server IP address (default: 127.0.0.1)
-   `--port`: CARLA server port (default: 2000)
-   `--config`: Path to configuration file (default: walker_spawn_config.json)
-   `--log-level`: Logging level (DEBUG, INFO, WARNING, ERROR)

## How It Works

1. **Vehicle Tracking**: The script continuously monitors the position of the hero vehicle
2. **Area Detection**: When the vehicle enters the defined trigger area, the system detects this entry
3. **Walker Spawning**: Upon area entry, walkers are spawned at the configured starting positions
4. **Walker Behavior**: Each walker is assigned a target position and walks there at the configured speed
5. **Entry Management**: Optionally spawn walkers only once per area entry (configurable)

## Configuration Tips

### Setting Up the Trigger Area:

1. Determine the center coordinates where you want the trigger area
2. Set an appropriate radius (e.g., 50 meters for a large area)

### Defining Walker Positions:

1. **spawn_positions**: Place these where you want walkers to initially appear
2. **target_positions**: Place these where you want walkers to walk to
3. The system will pair spawn and target positions by index (wrapping around if needed)

### Speed Configuration:

-   Set `min` and `max` to the same value for consistent speed
-   Use different values for random speed variation
-   `default` is used as fallback

### Example Coordinate Setup:

```json
"spawn_positions": [
    {"x": -389.46, "y": 9917.06, "z": 0.3,  // Left side of road
    {"x": 0, "y": 0, "z": 0.3}    // Right side of road
],
"target_positions": [
    {"x": 413.30, "y": 9917.06, "z": 0.3},   // Cross to other side
    {"x": -10, "y": 10, "z": 0.3}     // Cross to other side
]
```

## Integration with DReyeVR

This script is designed to work alongside the DReyeVR system. Make sure:

1. Your vehicle has the role_name "hero" (or configure accordingly)
2. CARLA simulation is running
3. The DReyeVR ego vehicle is spawned before running this script

## Logging

The script provides detailed logging including:

-   Vehicle position tracking
-   Area entry/exit events
-   Walker spawning success/failure
-   Error messages and warnings

Set `--log-level DEBUG` for maximum verbosity during development.

## Notes

-   The script runs in asynchronous mode by default (sync_mode: false)
-   Walkers are automatically cleaned up when the script exits
-   The system supports multiple spawn/target position pairs
-   Walker speed can be randomized within the specified range
-   The trigger area is circular - modify the code for other shapes if needed
