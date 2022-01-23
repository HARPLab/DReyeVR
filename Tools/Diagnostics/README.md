# Diagnostics

## Note: Mostly for development/WIP

This section highlights debug tools used to ensure all CARLA/DReyeVR performance was going as expected. As a general overview:
- [collectl/](collectl) uses [`collectl`](http://collectl.sourceforge.net/) on Linux (Or WSL) to gather and query system information so we can see how the computer was doing during the running of CARLA (kinda like a terminal task-manager/msi-afterburner)
    - Generally, you'll need to first setup collectl, and thats what the setup script does for you
- [python/](python) contains a bunch of python scripts for handling the `collectl` data but also some simpler ones to simply measure and record carla stats (such as FPS), see `stat_carla.py`

WARNING: Not all of these scripts have been tested on the latest version of DReyeVR/Carla.