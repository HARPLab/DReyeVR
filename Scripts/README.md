# Use Our Makefile System

How to use the DReyeVR [`Makefile`](../Makefile)

## `make install`
```bash
# automatically install over $CARLA_ROOT and $SCENARIO_RUNNER_ROOT
make install

# optionally, try to install on a specific directory (relative or abs)
make install CARLA=../carla
make install SR=../scenario_runner
make install CARLA=../carla SR=../scenario_runner
```

Additionally, a backup of all overwritten files is created in `Backups/` which should look like this:

<details>

<summary> Show expected file tree for first-time backup</summary>

```
# this tree holds all the files in CARLA that have been overwritten by the DReyeVR install
Backups
└── PATH
    └── TO
        └── YOUR
            └── carla
                ├── LibCarla
                │   └── source
                │       ├── carla
                │       │   └── sensor
                │       │       └── SensorRegistry.h
                │       └── test
                │           └── common
                │               └── test_streaming.cpp
                ├── PythonAPI
                │   ├── carla
                │   │   └── source
                │   │       └── libcarla
                │   │           └── SensorData.cpp
                │   └── examples
                │       └── start_recording.py
                ├── Unreal
                │   └── CarlaUE4
                │       ├── CarlaUE4.uproject
                │       ├── Config
                │       │   ├── DefaultEngine.ini
                │       │   ├── DefaultGame.ini
                │       │   └── DefaultInput.ini
                │       ├── Content
                │       │   └── Carla
                │       │       ├── Blueprints
                │       │       │   └── Game
                │       │       │       └── CarlaGameMode.uasset
                │       │       ├── Config
                │       │       │   └── Default.Package.json
                │       │       └── Maps
                │       │           ├── Town01.umap
                │       │           ├── Town02.umap
                │       │           ├── Town03.umap
                │       │           ├── Town04.umap
                │       │           ├── Town05.umap
                │       │           ├── Town06.umap
                │       │           ├── Town07.umap
                │       │           └── Town10HD.umap
                │       ├── Plugins
                │       │   └── Carla
                │       │       └── Source
                │       │           └── Carla
                │       │               ├── Actor
                │       │               │   └── ActorRegistry.cpp
                │       │               ├── Game
                │       │               │   └── CarlaEpisode.h
                │       │               ├── Recorder
                │       │               │   ├── CarlaRecorder.cpp
                │       │               │   ├── CarlaRecorder.h
                │       │               │   ├── CarlaRecorderHelpers.cpp
                │       │               │   ├── CarlaRecorderHelpers.h
                │       │               │   ├── CarlaRecorderQuery.cpp
                │       │               │   ├── CarlaRecorderQuery.h
                │       │               │   ├── CarlaReplayer.cpp
                │       │               │   ├── CarlaReplayer.h
                │       │               │   ├── CarlaReplayerHelper.cpp
                │       │               │   └── CarlaReplayerHelper.h
                │       │               ├── Settings
                │       │               │   └── CarlaSettingsDelegate.cpp
                │       │               ├── Traffic
                │       │               │   ├── TrafficLightManager.cpp
                │       │               │   └── YieldSignComponent.cpp
                │       │               ├── Vehicle
                │       │               │   ├── CarlaWheeledVehicle.cpp
                │       │               │   └── CarlaWheeledVehicle.h
                │       │               └── Weather
                │       │                   ├── Weather.cpp
                │       │                   ├── Weather.h
                │       │                   └── WeatherParameters.h
                │       └── Source
                │           └── CarlaUE4
                │               └── CarlaUE4.Build.cs
                └── Util
                    └── BuildTools
                        ├── BuildOSM2ODR.sh
                        ├── BuildPythonAPI.sh
                        └── Setup.sh
```
</details>

## `make clean`

Cleaning DReyeVR out of Carla is a good step to take when updating DReyeVR versions and/or reverting to a vanilla CARLA installation. 

The approach we use to "clean" carla is to first replace all the overwritten files from `make install` with the backups made in `Backups/`. Then, there is an option to perform a `hard-clean` where `git` is used to `reset --hard HEAD && clean -fd` which will reset all files to the `git HEAD` and then delete all untracked files/directories (`clean -fd`). 

**WARNING** the `hard-clean` step is irreversible and hence requires confirmation that looks like:
```bash
WARNING: Performing hard-clean, this irreversible action will reset all tracked CARLA files and remove untracked ones. Are you sure you want to continue? (y/n)
```

And you are free to skip this step by pressing `n`.

```bash
# automatically clean $CARLA_ROOT and $SCENARIO_RUNNER_ROOT
make clean

# optionally, try to clean on a specific directory
make clean CARLA=../carla
make clean SR=../scenario_runner
make clean CARLA=../carla SR=../scenario_runner

# then it is recommended to go to your CARLA dir and clean from there
cd /PATH/TO/CARLA
make clean # this is the CARLA clean!
```

## `make check`
Check the filesystem tree after installation
```bash
# automatically check $CARLA_ROOT and $SCENARIO_RUNNER_ROOT
make check

# optionally, try to check on a specific directory
make check CARLA=../carla
make check SR=../scenario_runner
make check CARLA=../carla SR=../scenario_runner
```

Expected output after installation
```
**************************************************
Summary:
Carla          [OK]
ScenarioRunner [OK]

Done check
**************************************************
```

## `make r-install`

```bash
# automatically check $CARLA_ROOT and $SCENARIO_RUNNER_ROOT
make r-install

# optionally, try to check on a specific directory
make r-install CARLA=../carla
make r-install SR=../scenario_runner
make r-install CARLA=../carla SR=../scenario_runner
```

## `make test`

This runs the unit tests for the DReyeVR scripts provided in [`tests.py`](tests.py). This is primarily for development purposes, to ensure that the low level filesystem functions work across various platforms (Windows, Linux, MacOS)

The expected output you should get is as follows:
```
Unit tests for installation scripts
**************************************************
Summary:
leaf:           [OK]
is_dir:         [OK]
create:         [OK]
cp:             [OK]
join:           [OK]
rm:             [OK]

Done check
**************************************************
```