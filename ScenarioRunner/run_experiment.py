#!/usr/bin/env python3

import time
import glob
import os
import argparse
import sys
from threading import Thread
from datetime import datetime
import argparse

try:
    CARLA_ROOT: str = os.getenv("CARLA_ROOT")
    egg_dir = os.path.join(CARLA_ROOT, "PythonAPI", "carla", "dist")
    sys.path.extend([glob.glob(os.path.join(egg_dir, f"carla-*.egg"))[0],
                     os.path.join(CARLA_ROOT, "PythonAPI"),
                     os.path.join(CARLA_ROOT, "PythonAPI", "carla"),
                     os.path.join(CARLA_ROOT, "PythonAPI", "examples")])
except IndexError:
    print(f"Unable to find Carla PythonAPI file in {egg_dir}")

import carla

from scenario_runner import ScenarioRunner

recorder_file = None


def start_scenario_runner(scenario_runner_instance):
    try:
        print("Starting scenario runner")
        result = scenario_runner_instance.run()
    finally:
        if scenario_runner_instance is not None:
            scenario_runner_instance.destroy()
            del scenario_runner_instance
    print("Stopped scenario runner, Result:", result)


def wait_until_SR_loaded(scenario_runner_instance, ping_freq_s=0.5):
    # wait until scenario_runner world has been loaded
    while scenario_runner_instance.world is None:
        time.sleep(ping_freq_s)


def run_schematic(argparser, scenario_runner_instance):
    wait_until_SR_loaded(scenario_runner_instance)
    print(10 * "=" + "schematic mode" + 10 * "=")

    # can be completely avoided if --visualize is False
    # NOTE: this import uses export PYTHONPATH=$PYTHONPATH:${CARLA_ROOT}/PythonAPI/examples
    raise NotImplementedError
    from dreyevr.examples.schematic_mode import schematic_run

    # for full definitions of these args see no_rendering_mode.py
    args = argparser.parse_known_args(
        [
            "-v",
            "--verbose",
            "--host",
            "-p",
            "-port",
            "--res",
            "--filter",
            "--map",
            "--no-rendering",
            "--show-triggers",
            "--show-connections",
            "--show-spawn-points",
        ]
    )
    # TODO: figure out why this is not working
    schematic_run(args)


def start_recording(client, args, scenario_runner_instance):
    wait_until_SR_loaded(scenario_runner_instance)
    time_str: str = datetime.now().strftime("%m_%d_%Y_%H_%M_%S")
    filename: str = f"exp_{args.title}_{time_str}.rec"

    global recorder_file  # to "return" from this thread
    recorder_file = client.start_recorder(filename)
    print("Recording on file: %s" % recorder_file)


def stop_recording(client):
    global recorder_file
    print(f"Stopping recording, file saved to \"{recorder_file}\"")
    client.stop_recorder()


def scenario_runner_args(parser):
    # NOTE: see scenario_runner.py for a better explanation of these
    parser.add_argument("-v", "--version", action="version", version="0.9.13")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=2000)
    parser.add_argument("--timeout", default="10.0")
    parser.add_argument("--trafficManagerPort", default="8000")
    parser.add_argument("--trafficManagerSeed", default="0")
    parser.add_argument("--sync", action="store_true")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--route", nargs="+", type=str)
    parser.add_argument("--agent")
    parser.add_argument("--agentConfig", type=str, default="")
    parser.add_argument("--output", action="store_true", default=True)
    parser.add_argument("--junit", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--configFile", default="")
    parser.add_argument("--additionalScenario", default="")
    parser.add_argument("--debug", action="store_true", default=False)
    parser.add_argument("--reloadWorld", action="store_true", default=True)
    parser.add_argument("--record", type=str, default="")
    parser.add_argument("--randomize", action="store_true")
    parser.add_argument("--repetitions", default=1, type=int)
    parser.add_argument("--waitForEgo", action="store_true")
    parser.add_argument("--outputDir", default="")

    # arguments we're ignoring
    parser.add_argument("--file", action="store_true")
    parser.add_argument("--scenario", default=None)
    parser.add_argument("--openscenario", default=None)


def main():
    # Define arguments that will be received and parsed

    description = (
        "DReyeVR Scenario Runner & Experiment Runner\n" "Current version: 0.9.13"
    )

    argparser = argparse.ArgumentParser(
        description=description, formatter_class=argparse.RawTextHelpFormatter
    )

    argparser.add_argument("-t", "--title", default="anonymous")
    argparser.add_argument("--visualize", action="store_true", default=False)

    # also add all requisite scenario_runner arguments
    scenario_runner_args(argparser)

    args = argparser.parse_args()

    if not args.route:
        print("Please specify the route mode, other modes are not supported\n\n")
        argparser.print_help(sys.stdout)
        exit(1)

    client = carla.Client(args.host, args.port)

    scenario_runner_instance = ScenarioRunner(args)

    # start the listening recorder thread (background, waits for scenario to begin)
    recording_thread = Thread(
        target=start_recording, args=(client, args, scenario_runner_instance,)
    )
    recording_thread.start()  # run in background

    if args.visualize is True:
        schematic_thread = Thread(
            target=run_schematic, args=(argparser, scenario_runner_instance,)
        )
        schematic_thread.start()

    start_scenario_runner(scenario_runner_instance)

    # finish recording
    stop_recording(client)


if __name__ == "__main__":
    main()
