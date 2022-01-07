#!/usr/bin/env python

import argparse
import csv  # writing fps data
import time
import pandas as pd
import os

"""IMPORTANT"""
# NOTE: this script assumes it is run in DReyeVR/Diagnostics/python
# So it does NOT need to be in carla's PythonAPI


def main():
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        '-d', '--dir',
        metavar='D',
        default="results/collectl",  # cwd
        type=str,
        help='data directory for outputs')
    argparser.add_argument(
        '-c1', '--collectl',
        metavar='C1',
        default="collectl_data.csv",
        type=str,
        help='csv file for collectl data',
        required=False)
    argparser.add_argument(
        '-c2', '--carla',
        metavar='C2',
        default="carla_metadata.csv",
        type=str,
        help='csv file for carla metadata',
        required=False)
    argparser.add_argument(
        '-s', '--start',
        metavar='S',
        default="00:00:00.00",
        type=str,
        help='timestamp when carla starts hh:mm:ss.ms',
        required=False)
    argparser.add_argument(
        '-e', '--end',
        metavar='E',
        default="00:00:00.00",
        type=str,
        help='timestamp when carla ends hh:mm:ss.ms',
        required=False)

    args = argparser.parse_args()
    output_dir = args.dir
    collectl_data = args.collectl
    carla_data = args.carla
    carla_start = args.start
    carla_end = args.end

    # read and write files
    path = os.path.join(os.getcwd(), output_dir)
    collectl_path = os.path.join(path, collectl_data)
    carla_path = os.path.join(path, carla_data)
    result = os.path.join(path, "combined.csv")
    with open(collectl_path, 'r') as collectl:
        with open(carla_path, 'r') as carla:
            df_collectl = pd.read_csv(collectl)
            df_carla = pd.read_csv(carla)
            df_carla = df_carla.dropna(axis=1)
            df_combined = df_collectl.join(df_carla)
            # add start/end to csv
            df_timestamps = pd.DataFrame({"[CARLA]t_start": [carla_start],
                                          "[CARLA]t_end": [carla_end]})
            df_combined = df_combined.join(df_timestamps)
            df_combined.to_csv(result, index=False)

    print("combined", collectl_data,"and", carla_data, "in", output_dir)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
