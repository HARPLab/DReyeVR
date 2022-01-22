#!/bin/bash

DOC_STRING="Install DReyeVR atop working (1) Carla and/or (2) SRanipal installations"
USAGE_STRING="Usage: $0 [-h|--help, --carla=/PATH/TO/CARLA, --scenario-runner=/PATH/TO/SCENARIO_RUNNER]"

INSTALL_CARLA=false
INSTALL_CARLA_ROOT=
INSTALL_SCENARIO_RUNNER=false
INSTALL_SCENARIO_RUNNER_ROOT=

while [[ $# -gt 0 ]]; do
  case "$1" in
    --carla )
      INSTALL_CARLA=true;
      INSTALL_CARLA_ROOT="$2";
      shift ;;
    --scenario-runner )
      INSTALL_SCENARIO_RUNNER=true;
      INSTALL_SCENARIO_RUNNER_ROOT="$2";
      shift ;;
    -h | --help )
      echo "$DOC_STRING"
      echo "$USAGE_STRING"
      exit 1
      ;;
    * )
      shift ;;
  esac
done

set -e # fail on error

# current version of CARLA supported
SUPPORTED_CARLA="0.9.13"
SUPPORTED_SCENARIO_RUNNER="v0.9.13"

source $(dirname "$0")/utils.sh
source $(dirname "$0")/installers.sh

if ${INSTALL_CARLA}; then
    CARLA_ROOT=$(update_root $CARLA_ROOT $INSTALL_CARLA_ROOT)

    verify_root "Carla" $CARLA_ROOT

    verify_installation $CARLA_ROOT CHANGELOG.md "Carla"

    verify_version $CARLA_ROOT $SUPPORTED_CARLA "Carla" carla_install
fi

if ${INSTALL_SCENARIO_RUNNER}; then
    SCENARIO_RUNNER_ROOT=$(update_root $SCENARIO_RUNNER_ROOT $INSTALL_SCENARIO_RUNNER_ROOT)
    
    verify_root "ScenarioRunner" $SCENARIO_RUNNER_ROOT
    
    verify_installation $SCENARIO_RUNNER_ROOT scenario_runner.py "ScenarioRunner" 
    
    verify_version $SCENARIO_RUNNER_ROOT $SUPPORTED_SCENARIO_RUNNER "ScenarioRunner" scenario_runner_install
fi