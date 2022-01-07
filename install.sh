#!/bin/bash

DReyeVR_ROOT=$PWD
CARLA_ROOT=$1
SCENARIO_RUNNER_ROOT=$2

set -e # fail on error

SUPPORTED_CARLA="0.9.11"
SUPPORTED_SCENARIO_RUNNER="v0.9.11"

source Scripts/utils.sh
source Scripts/installers.sh

print_usage

verify_root "Carla" $CARLA_ROOT

verify_installation $CARLA_ROOT CHANGELOG.md "Carla"

verify_version $CARLA_ROOT $SUPPORTED_CARLA "Carla" install_over_carla

echo
echo

verify_root "ScenarioRunner" $SCENARIO_RUNNER_ROOT

verify_installation $SCENARIO_RUNNER_ROOT scenario_runner.py "ScenarioRunner" 

verify_version $SCENARIO_RUNNER_ROOT $SUPPORTED_SCENARIO_RUNNER "ScenarioRunner" install_over_scenario_runner