#!/bin/bash

install_over_carla() {
    # this function denotes the specific files that get copied over from DReyeVR to a compatible Carla directory
    echo -e "\nInstalling DReyeVR"
    echo -e "Copying files..."
    cp -v -r DReyeVR $CARLA_ROOT/Unreal/CarlaUE4/Source/CarlaUE4/
    cp -v Configs/CarlaUE4.Build.cs $CARLA_ROOT/Unreal/CarlaUE4/Source/CarlaUE4/
    cp -v Configs/CarlaGameMode.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Game/
    cp -v Configs/Default*.ini $CARLA_ROOT/Unreal/CarlaUE4/Config/
    cp -v Configs/DReyeVRConfig.ini $CARLA_ROOT/Unreal/CarlaUE4/Config/
    cp -v -r Blueprints/DReyeVR $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/
    cp -v -r Blueprints/2Wheeled $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/
    cp -v -r Blueprints/Game $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/
    mkdir -p $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/DReyeVR/Sounds/
    cp -v -r Sounds/*.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Blueprints/Vehicles/DReyeVR/Sounds/
    mkdir -p $CARLA_ROOT/Unreal/CarlaUE4/Content/DReyeVR/ # new DReyeVR content folder
    cp -v -r Content/DReyeVR_Signs $CARLA_ROOT/Unreal/CarlaUE4/Content/DReyeVR/
    cp -v Content/Default.Package.json $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Config/
    cp -v Maps/* $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Maps/
    cp -v LibCarla/Sensor/data/* $CARLA_ROOT/LibCarla/source/carla/sensor/data/
    cp -v LibCarla/Sensor/s11n/* $CARLA_ROOT/LibCarla/source/carla/sensor/s11n/
    cp -v LibCarla/SensorRegistry.h $CARLA_ROOT/LibCarla/source/carla/sensor/
    cp -v LibCarla/SensorData.cpp $CARLA_ROOT/PythonAPI/carla/source/libcarla/
    cp -v Configs/CarlaUE4.uproject $CARLA_ROOT/Unreal/CarlaUE4/
    cp -v PythonAPI/*.py $CARLA_ROOT/PythonAPI/examples/
    cp -v Carla/Vehicle/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Vehicle/
    cp -v Carla/Sensor/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Sensor/
    cp -v Carla/Game/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Game/
    cp -v Carla/Settings/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/
    cp -v Carla/Actor/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Actor/
    cp -v Carla/Recorder/* $CARLA_ROOT/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Recorder/
    cp -v Tools/BuildTools/*.sh $CARLA_ROOT/Util/BuildTools/
    cp -v Tools/BuildTools/test_streaming.cpp $CARLA_ROOT/LibCarla/source/test/common/
    echo -e "\n${G}Installation of DReyeVR successful!${NC}"
}

install_over_scenario_runner(){
    # this function denotes the specific files that get copied over from DReyeVR to a compatible ScenarioRunner directory
    echo -e "\nInstalling ScenarioRunner"
    echo -e "Copying files..."
    cp -v ScenarioRunner/run_experiment.py $SCENARIO_RUNNER_ROOT/
    cp -v ScenarioRunner/route_scenario.py $SCENARIO_RUNNER_ROOT/srunner/scenarios/
    cp -v ScenarioRunner/carla_data_provider.py $SCENARIO_RUNNER_ROOT/srunner/scenariomanager/
    echo -e "\n${G}Installation of ScenarioRunner successful!${NC}"
}