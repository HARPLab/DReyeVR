import argparse
import csv
import os
import sys
import time
from pprint import pprint

import numpy as np
from DReyeVR_utils import DReyeVRSensor, find_ego_sensor, find_ego_vehicle

try:
    import rospy
    from std_msgs.msg import String
except ImportError:
    rospy = None
    String = None
    print("Rospy not initialized. Unable to use ROS for logging")

import carla


def create_ros_msg(ego_sensor: DReyeVRSensor, delim: str = "; "):
    assert rospy is not None and String is not None
    s = "rosT=" + str(rospy.get_time()) + delim
    for key in ego_sensor.data:
        s += f"{key}={ego_sensor.data[key]}{delim}"
    return String(s)


def init_ros_pub(IP_SELF, IP_ROSMASTER, PORT_ROSMASTER):
    assert rospy is not None and String is not None
    # set the environment variables for ROS_IP
    os.environ["ROS_IP"] = IP_SELF
    os.environ["ROS_MASTER_URI"] = "http://" + IP_ROSMASTER + ":" + str(PORT_ROSMASTER)

    # init ros publisher
    try:
        rospy.set_param("bebop_ip", IP_ROSMASTER)
        rospy.init_node("dreyevr_node")
        return rospy.Publisher("dreyevr_pub", String, queue_size=10)
    except ConnectionRefusedError:
        print("RospyError: Could not initialize rospy connection")
        sys.exit(1)


def main():

    argparser = argparse.ArgumentParser(description=__doc__)
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
        "-rh",
        "--roshost",
        metavar="Rh",
        default="172.31.25.167",
        help="IP of the host ROS server (default: 192.168.86.33)",
    )
    argparser.add_argument(
        "-rp",
        "--rosport",
        metavar="Rp",
        default=11311,
        help="TCP port for ROS server (default: 11311)",
    )
    argparser.add_argument(
        "-sh",
        "--selfhost",
        metavar="Sh",
        default="163.221.139.137",
        help="IP of the ROS node (this machine) (default: 192.168.86.123)",
    )

    args = argparser.parse_args()

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    sync_mode = True  # synchronous mode
    np.random.seed(int(time.time()))

    if rospy is not None:
        # tunable parameters for your configuration
        IP_SELF = args.selfhost  # where the rosnode is being run (here)
        # NOTE: that IP_SELF may not be the local host if passing main network to VM
        # where the rosmaster (carla roslaunch) is being run
        IP_ROSMASTER = args.roshost
        PORT_ROSMASTER = args.rosport
        pub = init_ros_pub(IP_SELF, IP_ROSMASTER, PORT_ROSMASTER)

    world = client.get_world()
    sensor = DReyeVRSensor(world)
    
    # Note: set the vehicle speed to be stable
    vehicle = find_ego_vehicle(world)
     # stop the vehicle


    def publish_and_print(data):
        # global df
        vehicle.set_target_velocity(carla.Vector3D(x=1.0, y =0.0, z=0.0))
        sensor.update(data)
        if rospy is not None:
            msg: String = create_ros_msg(sensor)
            pub.publish(msg)  # publish to ros master
        pprint(sensor.data)  # more useful print here (contains all attributes)
        time.sleep(0.1)

    # subscribe to DReyeVR sensor
    sensor.ego_sensor.listen(publish_and_print)
    try:
        while True:
            if sync_mode:
                world.tick()
            else:
                world.wait_for_tick()
    finally:
        if sync_mode:
            settings = world.get_settings()
            settings.synchronous_mode = False
            settings.fixed_delta_seconds = None
            world.apply_settings(settings)


if __name__ == "__main__":
    try:
        main()

    except KeyboardInterrupt:
        pass
    finally:
        print("\ndone.")
