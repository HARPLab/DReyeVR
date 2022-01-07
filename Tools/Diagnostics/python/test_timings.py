#!/usr/bin/env python

from test_spawn_debug import DebugHelper, Point as Pt
import argparse
import time
import sys
import glob
import os

"""IMPORTANT"""
# NOTE: this script assumes it is in carla's PythonAPI

python_egg = glob.glob(os.getcwd() + '/../carla/dist/carla-*.egg')
try:
    # sourcing python egg file
    sys.path.append(python_egg[0])
    import carla  # works if the python egg file is properly sourced
    print("Successfully imported Carla and sourced .egg file")
except Exception as e:
    print("Error:", e)

# just some enums to clear things up later
EYES = ["COMBO", "LEFT", "RIGHT"]
COMBO = EYES[0]
LEFT = EYES[1]
RIGHT = EYES[2]

# debugging colours
RED = "\033[1;31m"
BLUE = "\033[1;34m"
GREEN = "\033[0;32m"
RESET = "\033[0;0m"


def spawn_DReyeVR_sensor(world):
    # spawn a DReyeVR sensor and begin listening
    spectator = world.get_spectator()
    dreyevrBP = [x for x in  # how to get DReyeVRSensor
                 world.get_blueprint_library().filter("sensor.dreyevr.*")]
    DReyeVRSensor = world.spawn_actor(dreyevrBP[0],
                                      spectator.get_transform(),
                                      attach_to=spectator)
    print("Spawned DReyeVRsensor: " + DReyeVRSensor.type_id)
    return DReyeVRSensor


def save_to_file(data, name: str, dir_path="timing_data"):
    filename = name+".txt"
    if not os.path.exists(os.path.join(os.getcwd(), dir_path)):
        os.mkdir(dir_path)
    filepath = os.path.join(os.getcwd(), dir_path, filename)
    mode = 'w' if os.path.exists(filepath) else 'w+'
    with open(filepath, mode) as file:
        file.write(name + ":" + str(data))
    print("Wrote to", filename)


def output_results(carla, sranipal, carla_stream):
    save_to_file(carla, "carla")
    save_to_file(sranipal, "sranipal")
    save_to_file(carla_stream, "carla_stream")


def main():
    argparser = argparse.ArgumentParser(
        description=__doc__)
    argparser.add_argument(
        '--host',
        metavar='H',
        default='127.0.0.1',
        help='IP of the host server (default: 127.0.0.1)')
    argparser.add_argument(
        '-p', '--port',
        metavar='P',
        default=2000,
        type=int,
        help='TCP port to listen to (default: 2000)')
    argparser.add_argument(
        '-d', '--duration',
        metavar='D',
        default=100,
        type=int,
        help='Time (in seconds) that the test runs for')

    args = argparser.parse_args()

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    sync_mode = False  # synchronous mode

    world = client.get_world()
    # spawn a DReyeVR sensor and begin listening
    DReyeVR_sensor = spawn_DReyeVR_sensor(world)

    carla_time = []
    sranipal_time = []
    carla_stream_time = []

    def update_information(data):
        # is updated on the sensor's listen thread
        if(data.timestamp_carla > 0 and data.timestamp_carla_stream > 0 and data.timestamp_sranipal > 0):
            # get the useful data and add it to our time lists
            t_carla = data.timestamp_carla / 1000.0
            t_sranipal = data.timestamp_sranipal / 1000.0  # convertion from ms to s
            t_carla_stream = data.timestamp_carla_stream
            # append to global lists
            carla_time.append(t_carla)
            sranipal_time.append(t_sranipal)
            carla_stream_time.append(t_carla_stream)

    # subscribe to DReyeVR sensor
    DReyeVR_sensor.listen(update_information)
    print("Press 'r' in the simulator to begin recording data")
    while len(carla_time) == 0:
        time.sleep(0.01)
    print("Sensor connected and beginning recording")
    try:
        print("Starting test, should take", args.duration, "seconds")
        time.sleep(args.duration)  # sleep for 5 minutes while collecting data
        output_results(carla_time, sranipal_time, carla_stream_time)
    finally:
        # TODO: fix bug where this does not actually remove the sensor from the simulation
        carla.command.DestroyActor(DReyeVR_sensor)

        if sync_mode:
            settings = world.get_settings()
            settings.synchronous_mode = False
            settings.fixed_delta_seconds = None
            world.apply_settings(settings)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        print('\ndone.')
