#!/usr/bin/env python

import argparse
import numpy as np
import csv
import time
import sys
import glob
import os
import matplotlib.pyplot as plt
import datetime


"""IMPORTANT"""
# NOTE: this script assumes it is run in Diagnostics/python/
# So it does NOT need to be in carla's PythonAPI, but it DOES need
# the CARLA_DIR (-c, --carladir) environment variable to be set


def save_to_file(data, dir_path, filename, suffix=""):
    assert os.path.exists(dir_path)
    filename = f"{filename}{suffix}.csv"
    filepath = os.path.join(os.getcwd(), dir_path, filename)
    print(f"saving data to: {filepath}")
    mode = "w" if os.path.exists(filepath) else "w+"
    with open(filepath, mode) as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(data.keys())
        # writes ordered
        csv_writer.writerows(zip(*[data[key] for key in data.keys()]))
        # csv_writer.writerows(zip(*data.values())) # unordered


def plot(
    x,
    y,
    title,
    title_x="",
    title_y="",
    trim=(0, 0),
    out_dir="graphs",
    show_mean=True,
    colour="r",
    lines=True,
    suffix="",
):
    max_len = min(len(x), len(y))
    x = np.array(x[trim[0] : max_len - trim[1]])
    y = np.array(y[trim[0] : max_len - trim[1]])

    fig = plt.figure()
    plt.grid(True)
    plt.xlabel(title_x, fontsize=16)
    plt.ylabel(title_y, fontsize=16)
    plt.xticks()
    plt.yticks()
    plt.tick_params(labelsize=15)
    mean_str = f", mean: {np.mean(y):.3f}" if show_mean else ""
    graph_title = f"{title}{mean_str}"
    plt.title(graph_title, fontsize=18)
    plt.plot(x, y, colour + "o")
    if lines:
        plt.plot(x, y, color=colour, linewidth=1)
    plt.tight_layout()
    os.makedirs(out_dir, exist_ok=True)
    filename = (f"{title}{suffix}.jpg").replace(" ", "_")  # no spaces!
    filepath = os.path.join(os.getcwd(), out_dir, filename)
    print(f"Saving graph to: {filepath}")
    fig.savefig(filepath)
    plt.close(fig)


def main():
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        "-c",
        "--carladir",
        metavar="C",
        default="",  # cwd
        type=str,
        help="Directory for Carla",
    )
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
        "-d",
        "--dir",
        metavar="D",
        default="data",  # cwd
        type=str,
        help="data directory for outputs",
    )
    argparser.add_argument(
        "-f",
        "--file",
        metavar="F",
        default="carla_data",
        type=str,
        help="name of output file",
    )
    argparser.add_argument(
        "-i",
        "--interval",
        metavar="I",
        default="0.1",  # in seconds
        type=float,
        help="intervals (s) of which to take framerate",
    )

    args = argparser.parse_args()
    carla_dir = args.carladir
    output_dir = args.dir
    py_delay = args.interval
    filename = args.file

    os.makedirs(output_dir, exist_ok=True)

    """Import Carla given the Carla Directory"""
    if carla_dir == "":
        print("Need to pass in the CARLA environment directory!")
        exit(1)

    egg_locn = os.path.join(carla_dir, "PythonAPI", "carla", "dist")
    print(f"Trying to load python .egg from {egg_locn}")
    python_egg = glob.glob(os.path.join(egg_locn, "carla-*.egg"))
    try:
        # sourcing python egg file
        sys.path.append(python_egg[0])
        import carla  # works if the python egg file is properly sourced
    except Exception as e:
        print("Error:", e)
        exit(1)
    print(f"Success! Continuing with script, pinging every {py_delay:.3f}s")

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)  # should be running already

    world = client.get_world()
    data = {}
    data["[CARLA]Idx"] = []
    data["[CARLA]Fps"] = []

    # to get FPS, we can get the world dt and compute the inv
    i = 0
    try:
        while True:
            # seconds=X presents maximum delay to wait for server
            delta_t = world.wait_for_tick(seconds=0.5).timestamp.delta_seconds
            fps = 1 / delta_t
            data["[CARLA]Idx"].append(i)
            data["[CARLA]Fps"].append(fps)
            # sleep for the interval
            time.sleep(py_delay)
            i += 1
            print(f"FPS: {fps:.3f}", end="\r")  # no flush (slow & expensive)
    except KeyboardInterrupt:
        print("Stopped by user.")
    except RuntimeError:
        print("Simulator disconnected.")
    finally:  # simulator disconnected
        ts = time.time()
        timestamp = datetime.datetime.fromtimestamp(ts).strftime("%Y-%m-%d-%H-%M-%S")
        suffix = f"_{timestamp}"
        save_to_file(data, output_dir, filename=filename, suffix=suffix)
        plot(
            x=data["[CARLA]Idx"],
            y=data["[CARLA]Fps"],
            title="CARLA FPS",
            title_x="",
            title_y="fps",
            out_dir=output_dir,
            show_mean=True,
            lines=True,
            suffix=suffix,
        )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
