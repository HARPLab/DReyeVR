# Carla Configuration Files
Here we hold an accumulation of all the modified Carla config files so far.
### Summary 
- `CarlaSettingsDelegate.cpp` replaces [`carla/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/CarlaSettingsDelegate.cpp`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/CarlaSettingsDelegate.cpp)
  - Used to get full-render distance in `-quality-level=Low` mode as described in [`VR-mode.md`](Guides/VR-mode.md)
- `DefaultInput.ini` replaces [`carla/Unreal/CarlaUE4/Config/DefaultInput.ini`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/Config/DefaultInput.ini)
  - Used to specify all the custom inputs controls that we created in [`VR-player.md`](VR-player.md)
- `DefaultEngine.ini` replaces [`carla/Unreal/CarlaUE4/Config/DefaultEngine.ini`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/Config/DefaultEngine.ini)
  - Used to define the default GameInstance, GameMode, Map, and other settings
- `CarlaUE4.uproject` replaces [`carla/Unreal/CarlaUE4/CarlaUE4.uproject`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/CarlaUE4.uproject)
  - Used to specify the custom plugins used such as `SteamVR` and `SRanipal`
- `CarlaUE4.Build.cs` replaces [`carla/Unreal/CarlaUE4/Source/CarlaUE4/CarlaUE4.Build.cs`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/Source/CarlaUE4/CarlaUE4.Build.cs)
  - Used to add custom modules for our cpp `Ego-Vehicle` agent such as `HeadMountedDisplay` and other plugins
- `Carla.Build.cs` replaces [`carla/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Carla.Build.cs`](https://github.com/carla-simulator/carla/blob/master/Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Carla.Build.cs) 
  - Used to add custom modules/plugins for our cpp `EyeTracker` sensor such as `SRanipal`
