#!/bin/bash

source $(dirname "$0")/utils.sh

carla_install() {
    ROOT=$1
    # this function denotes the specific files that get copied over from DReyeVR to a compatible Carla directory
    echo -e "\nInstalling DReyeVR to ${ROOT}"
    cp -v -r DReyeVR $ROOT/Unreal/CarlaUE4/Source/CarlaUE4/
    # configs
    cp -v Configs/CarlaUE4.Build.cs $ROOT/Unreal/CarlaUE4/Source/CarlaUE4/
    cp -v Configs/CarlaGameMode.uasset $ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Game/
    cp -v Configs/Default*.ini $ROOT/Unreal/CarlaUE4/Config/
    cp -v Configs/DReyeVRConfig.ini $ROOT/Unreal/CarlaUE4/Config/
    cp -v Configs/CarlaUE4.uproject $ROOT/Unreal/CarlaUE4/
    # blueprints
    cp -v -r Blueprints/DReyeVR $ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/
    cp -v -r Blueprints/2Wheeled $ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/
    cp -v -r Blueprints/Game $ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/
    # content
    mkdir -p $ROOT/Unreal/CarlaUE4/Content/DReyeVR/ # new DReyeVR content folder
    cp -v -r Content/DReyeVR_Signs $ROOT/Unreal/CarlaUE4/Content/DReyeVR/
    cp -v -r Content/Custom $ROOT/Unreal/CarlaUE4/Content/DReyeVR/
    cp -v Content/Default.Package.json $ROOT/Unreal/CarlaUE4/Content/Carla/Config/
    # maps
    cp -v Maps/* $ROOT/Unreal/CarlaUE4/Content/Carla/Maps/
    # LibCarla
    cp -v LibCarla/Sensor/data/* $ROOT/LibCarla/source/carla/sensor/data/
    cp -v LibCarla/Sensor/s11n/* $ROOT/LibCarla/source/carla/sensor/s11n/
    cp -v LibCarla/SensorRegistry.h $ROOT/LibCarla/source/carla/sensor/
    cp -v LibCarla/SensorData.cpp $ROOT/PythonAPI/carla/source/libcarla/
    # PythonAPI
    cp -v PythonAPI/*.py $ROOT/PythonAPI/examples/
    # Carla
    cp -v Carla/Vehicle/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Vehicle/
    cp -v Carla/Sensor/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Sensor/
    cp -v Carla/Game/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Game/
    cp -v Carla/Settings/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/
    cp -v Carla/Traffic/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Traffic/
    cp -v Carla/Actor/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Actor/
    cp -v Carla/Recorder/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Recorder/
    cp -v Carla/Weather/* $ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Weather/
    # BuildTools
    cp -v Tools/BuildTools/*.sh $ROOT/Util/BuildTools/
    cp -v Tools/BuildTools/test_streaming.cpp $ROOT/LibCarla/source/test/common/
    echo -e "${G}Installation of DReyeVR to Carla successful!${NC}"
}

scenario_runner_install() {
    ROOT=$1
    # this function denotes the specific files that get copied over from DReyeVR to a compatible ScenarioRunner directory
    echo -e "\nInstalling ScenarioRunner to ${ROOT}"
    cp -v ScenarioRunner/run_experiment.py $ROOT/
    cp -v ScenarioRunner/route_scenario.py $ROOT/srunner/scenarios/
    cp -v ScenarioRunner/carla_data_provider.py $ROOT/srunner/scenariomanager/
    echo -e "${G}Installation of DReyeVR to ScenarioRunner successful!${NC}"
}