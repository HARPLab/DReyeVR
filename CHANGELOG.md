## DReyeVR 0.1.3
- Fix bug where other non-ego Autopilto vehicles ignored the EgoVehicle and would routinely crash into it
- Fix bug where some EgoVehicle attributes were missing, notably `"number_of_wheels"`

## DReyeVR 0.1.2
- Update documentation to refer to CarlaUnreal UE4 fork rather than HarpLab fork
- Apply patches for installation of zlib (broken link) and xerces (broken link & `-Dtranscoder=windows` flag) and PythonAPI
- Fix crash on BeginPlay when EgoVehicle blueprint is already present in map (before world start)
- Prefer building PythonAPI with `python` rather than Windows built-in `py -3` (good if using conda) but fallback if `python` is not available 

## DReyeVR 0.1.1
- Update documentation, add developer-centric in-depth documentation
- Adding missing includes for TrafficSign RoadInfoSignal in SignComponent
- Replacing `DReyeVRData.inl` with `DReyeVRData.cpp` and corresponding virtual classes
- Add GitHub workflow for installing DReyeVR atop Carla and building LibCarla/PythonAPI (not CarlaUE4 which requires UnrealEngine)
- Fix bug where disabling mirrors causes a crash since the BP_model3 blueprint would be broken on construction
- Fix bug where replays would spawn another EgoVehicle/EgoSensor in the world (following the Carla spec) which caused multiple overlapping EgoVehicles to be in the world at once. Now we enforced in the Factory function that they are singletons
- Fix bug with replay interpolation not following our EgoVehicle by ticking the pose reenactment in the Carla tick rather than EgoVehicle::Tick
- Fix bug with spectator being reenacted by Carla, preferring to use our own ASpectator
- Automatically possess spectator pawn when EgoVehicle is destroyed. Can re-spawn EgoVehicle by pressing 1. 

## DReyeVR 0.1.0
- Replace existing `LevelScript` (`ADReyeVRLevel`) with `GameMode` (`ADReyeVRGameMode`). This allows us to not need to carry the (large) map blueprint files (ue4 binary) and we can use the vanilla Carla maps without modification. By default we spawn the EgoVehicle in the first of the recommended Carla locations, but this default behavior can be changed in the PythonAPI. For instance, you can delay spawning the EgoVehicle until via PythonAPI where you can specify the spawn transform. Existing functionality is preserved using `find_ego_vehicle` and `find_ego_sensor` which spawn the DReyeVR EgoVehicle if it does not exist in the world. 
- Added `ADReyeVRFactory` as the Carla-esque spawning and registry functionality so the `EgoVehicle` and `EgoSensor` are spawned with the same "Factory" mechanisms as existing Carla vehicles/sensors/props/etc.
- Renamed DReyeVR-specific actors to be addressible in PythonAPI as `"harplab.dreyevr_$X.$id"` such as `"harplab.dreyevr_vehicle.model3"` and `"harplab.dreyevr_sensor.ego_sensor"`. Avoids conflicts with existing Carla PythonAPI scripts that may filter on `"vehicle.*"`.
- Moved all blueprint (`.uasset`) content out of the Carla content (which is now entirely untouched, no need to re-update) to a separate Content folder that lies in `/Game/Content/DReyeVR/` (renamed to support future DReyeVR blueprint changes without relying on Carla content).
- Adding check that SRanipal is within one of our recommended supported versions: `1.3.1.1`, `1.3.2.0`, & `1.3.3.0`. Also provided links in the documentation to download these versions. Additionally included a workaround for the old `patch-sranipal` which is now no longer needed.
- Added custom `LogDReyeVR` macros (`LOG`, `LOG_WARN`, `LOG_ERROR`) for improved logging with attached file, function, and line number to the message. Done in precompilation to avoid runtime performance overhead.
- Improved `make check` to ensure that a 1:1 file correspondence exists between Carla dest and DReyeVR source, and that all files have equal content. 
- Improved vehicle dash with blinking turn signal and fixed bugs with inconsistent gear indicator. 
- Fixed bugs and inconsistencies with replayer media control and special actions such as toggling reverse/turn signals in replay.
- Enabled cooking for `Town06` and `Town07` in package/shipping mode.
- Updated documentation and config file
