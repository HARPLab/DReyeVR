#!/bin/bash

# Export the pythonpath to include the Carla python module
# NOTE: this requires the PythonAPI to be built on the machine this script is run
export PYTHONPATH=$PYTHONPATH:~/carla/PythonAPI/carla/dist/carla-0.9.10-py3.8-linux-x86_64.egg &&

# Source the main ROS setup, in our case we're using ROS Noetic
source /opt/ros/noetic/setup.bash &&

# Source the Carla ROS bridge setup file
source ~/carla-ros-bridge/catkin_ws/devel/setup.bash &&

# Ensure that the Carla python module is available (via the first command)
python -c 'import carla;print("Success")'

# launch the carla ROS bridge with our DReyeVR launch file
# roslaunch carla_ros_bridge_DReyeVR.launch
