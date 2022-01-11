# DReyeVR
### Welcome to DReyeVR, a VR driving simulator for behavioural and interactions research.

![Fig1](Docs/Figures/demo.gif)
<!-- Welcome to the DReyeVR wiki! -->

This project extends the [`Carla 0.9.11`](https://carla.org/2020/12/22/release-0.9.11/) [[GitHub](https://github.com/carla-simulator/carla/tree/0.9.11)] build that adds virtual reality integration, a first-person maneuverable ego-vehicle, eye tracking support, and several immersion enhancements.

## Highlights
### Ego Vehicle
Fully drivable **virtual reality (VR) ego-vehicle** with [SteamVR integration](https://github.com/ValveSoftware/steamvr_unreal_plugin/tree/4.23) (see [EgoVehicle.h](DReyeVR/EgoVehicle.h))
- SteamVR HMD head tracking (orientation & position)
  - We have tested with the following devices:
    | device | Eye tracking |
    | --- | --- |
    | [HTC Vive Pro Eye](https://business.vive.com/us/product/vive-pro-eye-office/) | YES ([SRanipal](https://developer-express.vive.com/resources/vive-sense/eye-and-facial-tracking-sdk/) 1.3.1.1)|
    | [Oculus/Meta Quest 2](https://www.oculus.com/quest-2/) | NO |
- Vehicle controls 
  - Generic keyboard WASD + mouse
  - Support for Logitech Steering wheel with this open source [LogitechWheelPlugin](https://github.com/drb1992/LogitechWheelPlugin) 
    - Includes force-feedback with the steering wheel.
    - We used a [Logitech G923 Racing Wheel & Pedals](https://www.logitechg.com/en-us/products/driving/driving-force-racing-wheel.html)
- Realistic rear-view mirrors (WARNING: **very** performance intensive)
- Vehicle dashboard
  - Speedometer
  - Gear indicator
  - Turn signals
- "Ego-centric" audio 
  - Responsive engine revving (throttle-based)
  - Turn signal clicks
  - Gear switching) 
- Fully compatible with the existing Carla [PythonAPI](https://carla.readthedocs.io/en/0.9.11/python_api/) and [ScenarioRunner](https://github.com/carla-simulator/scenario_runner/tree/v0.9.11)
  - Minor modifications were made. See [PythonAPI.md](Docs/PythonAPI.md) documentation.
- Fully compatible with the Carla [Recorder and Replayer](https://carla.readthedocs.io/en/0.9.11/adv_recorder/) 
  - Including HMD pose/orientation for first-person reenactment
  - Ability to handoff control to Carla's AI wheeled vehicle controller
### Ego Sensor
Carla-compatible **ego-vehicle sensor** (see [EgoSensor.h](DReyeVR/EgoSensor.h))
- **Eye tracking** support through [SRanipal Vive eye tracking](https://developer.vive.com/resources/vive-sense/sdk/vive-eye-and-facial-tracking-sdk/)
  - Note this requires use of the [HTC Vive Pro Eye](https://enterprise.vive.com/us/product/vive-pro-eye-office/) VR headset
  - Eye tracker data includes:
    - Timing information (based off headset, world, and eye-tracker)
    - 3D Eye gaze ray (left, right, & combined)
    - 2D Pupil position (left & right)
    - Pupil diameter (left & right)
    - Eye Openness (left & right)
    - See [DReyeVRData.h:EyeTracker](Carla/Sensor/DReyeVRData.h) for the complete list
  - Eye reticle plotting in real time
- Real-time user inputs (throttle, steering, brake, turn signals, etc.)
- Gaze focus info (3D world hit point, hit actor)
- Image (screenshot) frame capture based on the camera. WARNING: very performance intensive.
- Fully compatible with the LibCarla data serialization for streaming to a PythonAPI client (see [LibCarla/Sensor](LibCarla/Sensor))
  - We have also tested and verified support for (`rospy`) ROS integration our sensor data streams

### Other additions:
- Custom DReyeVR config file for one-time runtime params. See [DReyeVRConfig.ini](Configs/DReyeVRConfig.ini)
  - Especially useful to change params without recompiling everything.
  - Uses standard c++ io management to read the file with minimal performance impact. See [DReyeVRUtils.h](DReyeVR/DReyeVRUtils.h).
- World ambient audio
  - Birdsong, wind, smoke, etc. (Sourced from [Unreal starter content](https://docs.unrealengine.com/4.27/en-US/Basics/Projects/Browser/Packs/))
- Non-ego-centric audio (Engine revving from non-ego vehicles)
- Recorder/replayer media functions
  - Added in-game keyboard commands Play/Pause/Forward/Backward/etc.

## Install
See [`Docs/Install.md`](Docs/Install.md) to either:
- Install and build `DReyeVR` on top of a working Carla (0.9.11) build. 
- Use our [Carla fork](https://github.com/HARPLab/carla/tree/DReyeVR-0.9.11) with `DReyeVR` pre-installed.

## OS compatibility
| OS | VR | Eye tracking | Audio | Keyboard+Mouse | Racing wheel |
| --- | --- | --- | --- | --- | --- |
| Windows | Y | Y | Y | Y | Y |
| Linux | Y | N | Y | Y | N |
- While Windows (10) is recommended for optimized VR support, all our work translates to Linux systems except for the eye tracking and hardware integration which have Windows-only dependencies.
  - Unfortunately the eye-tracking firmware is proprietary & does not work on Linux
    - This is (currently) only supported on Windows because of some proprietary dependencies between [HTC SRanipal SDK](https://developer.vive.com/resources/knowledgebase/vive-sranipal-sdk/) and Tobii's SDK. Those interested in the Linux discussion for HTC's Vive Pro Eye Tracking can follow the subject [here (Vive)](https://forum.vive.com/topic/6994-eye-tracking-in-linux/), [here (Vive)](https://forum.vive.com/topic/7012-vive-pro-eye-on-ubuntu-16-or-18/), and [here (Tobii)](https://developer.tobii.com/community/forums/topic/vive-pro-eye-with-stream-engine/).
  - Additionally, the [LogitechWheelPlugin](https://github.com/drb1992/LogitechWheelPlugin) we use only has Windows support currently. Though it should be possible to use the G923 on Linux as per the [Arch Wiki](https://wiki.archlinux.org/title/Logitech_Racing_Wheel).

## Documentation & Guides
- See [`Docs/Usage.md`](Docs/Usage.md) to learn how to use several key DReyeVR features
- See [`Docs/SetupVR.md`](Docs/SetupVR.md) to learn how to quickly and minimally set up VR with Carla
- See [`Docs/Sounds.md`](Docs/Sounds.md) to see how we added custom sounds and how you can add your own custom sounds
- See [`Docs/Signs.md`](Docs/Signs.md) to add custom in-world directional signs and dynamically spawn them into the world at runtime
- See [`Docs/LODs.md`](Docs/LODs.md) to learn how we tune the Level-Of-Detail modes for vehicles for a more enjoyable VR experience

## Citation
If you use our work, please cite the corresponding [paper](https://arxiv.org/abs/2201.01931):
```bibtex
@misc{silvera2022dreyevr,
      title={DReyeVR: Democratizing Virtual Reality Driving Simulation for Behavioural & Interaction Research}, 
      author={Gustavo Silvera and Abhijat Biswas and Henny Admoni},
      year={2022},
      eprint={2201.01931},
      archivePrefix={arXiv},
      primaryClass={cs.HC}
}
```

## Acknowledgements

- This project builds upon and extends the [CARLA 0.9.11 simulator](https://carla.org/)
- This repo includes some code from CARLA: Computer Vision Center (CVC) at the Universitat Autonoma de Barcelona (UAB) & Intel Corporation.
- This repo includes some code from HTC Corporation. See [SRanipal/](SRanipal). These are only the minimal modifications required to fix bugs in [SRanipal 1.3.1.1](https://developer-express.vive.com/resources/vive-sense/eye-and-facial-tracking-sdk/).
- This repo includes some code from Hewlett-Packard Development Company, LP. See [Tools/Diagnostics/nvidia.ph](Tools/Diagnostics/nvidia.ph). This is a modified diagnostic tool used during development. 

## Licenses
- Custom DReyeVR code is distributed under the MIT License.
- Unreal Engine 4 follows its [own license terms](https://www.unrealengine.com/en-US/faq).
- Code used from other sources that is prefixed with a Copyright header belongs to those individuals/organizations. 
- CARLA specific licenses (and dependencies) are described on their [GitHub](https://github.com/carla-simulator/carla#licenses)