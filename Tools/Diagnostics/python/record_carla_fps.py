#!/usr/bin/env python

import argparse
import csv  # writing fps data
import time
import sys
import glob
import os

"""IMPORTANT"""
# NOTE: this script assumes it is run in DReyeVR/Diagnostics/
# So it does NOT need to be in carla's PythonAPI, but it DOES need
# the CARLA_DIR environment variable to be set

def save_to_file(data, dir_path, filename):
    if not os.path.exists(os.path.join(os.getcwd(), dir_path)):
        os.mkdir(dir_path)
    filepath = os.path.join(os.getcwd(), dir_path, filename)
    mode = 'w' if os.path.exists(filepath) else 'w+'
    with open(filepath, mode) as file:
        csv_writer = csv.writer(file)
        csv_writer.writerow(data.keys())
        # writes ordered
        csv_writer.writerows(zip(*[data[key] for key in data.keys()]))
        # csv_writer.writerows(zip(*data.values())) # unordered


def main():
    argparser = argparse.ArgumentParser(
        description=__doc__)
    argparser.add_argument(
        '-c', '--carladir',
        metavar='C',
        default="",  # cwd
        type=str,
        help='Directory for Carla')
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
        '-d', '--dir',
        metavar='D',
        default="./",  # cwd
        type=str,
        help='data directory for outputs')
    argparser.add_argument(
        '-f', '--file',
        metavar='F',
        default="carla_data.csv",
        type=str,
        help='name of output file')
    argparser.add_argument(
        '-i', '--interval',
        metavar='I',
        default="0.5",  # cwd
        type=float,
        help='intervals of which to take framerate')

    args = argparser.parse_args()
    carla_dir = args.carladir
    output_dir = args.dir
    py_delay = args.interval
    filename = args.file

    """Import Carla given the Carla Directory"""
    if carla_dir == "":
        print("Need to pass in the Carla environment directory!")
        exit(1)

    egg_locn = os.path.join(carla_dir, 'PythonAPI/carla/dist/')
    python_egg = glob.glob(egg_locn + "carla-*.egg")
    try:
        # sourcing python egg file
        sys.path.append(python_egg[0])
        import carla  # works if the python egg file is properly sourced
    except Exception as e:
        print("Error:", e)
        exit(1)

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)  # should be running already

    world = client.get_world()
    data = {}
    data["[CARLA]Idx"] = []
    data["[CARLA]Fps"] = []

    # carla is weird and does not provide a simple means to get fps
    # I'll need to get the world time and compute 1 / delta_t
    i = 0
    while(True):
        try:
            # seconds=X presents maximum delay to wait for server
            delta_t = world.wait_for_tick(seconds=0.5).timestamp.delta_seconds
        except RuntimeError:  # simulator disconnected
            break
        fps = 1 / delta_t
        data["[CARLA]Idx"].append(i)
        data["[CARLA]Fps"].append(fps)
        # sleep for the interval
        time.sleep(py_delay)
        i += 1

    # write to file
    save_to_file(data, output_dir, filename=filename)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
