# Installing `DReyeVR` to a working Carla 0.9.13 build

## Part One: Prerequisites

- Make sure your machine satisfies the prerequisites required by Carla: [Windows](https://carla.readthedocs.io/en/0.9.13/build_windows), [Linux](https://carla.readthedocs.io/en/0.9.13/build_linux), [Mac\*](https://github.com/GustavoSilvera/carla/blob/m1/Docs/build_mac.md)
- **IMPORTANT**: If on **Windows** you will **need** [`Make-3.81`](https://gnuwin32.sourceforge.net/packages/make.htm) as per the [Carla documentation](https://carla.readthedocs.io/en/latest/build_windows/#system-requirements)
- If you have previously installed Carla in your PYTHONPATH, you'll need to remove any prior PythonAPI installations
  - For instance, if you [installed carla via pip](https://pypi.org/project/carla/), you'll **need to uninstall** it to proceed.
    ```bash
    pip uninstall carla
    ```

### System requirements

- **x64 system.** The simulator should run in any 64 bits Windows system.
- **165 GB disk space.** CARLA itself will take around 32 GB and the related major software installations (including Unreal Engine) will take around 133 GB.
- **An adequate GPU.** CARLA aims for realistic simulations, so the server needs at least a 6 GB GPU although 8 GB is recommended. A dedicated GPU is highly recommended for machine learning.
- **Two TCP ports and good internet connection.** 2000 and 2001 by default. Make sure that these ports are not blocked by firewalls or any other applications.

!!! Warning
**If you are upgrading from CARLA 0.9.12 to 0.9.13**: you must first upgrade the CARLA fork of the UE4 engine to the latest version. See the [**Unreal Engine**](#unreal-engine) section for details on upgrading UE4

### Software requirements

#### Minor installations

- [**CMake**](https://cmake.org/download/) generates standard build files from simple configuration files. **We recommend you use version 3.15+**.
- [**Git**](https://git-scm.com/downloads) is a version control system to manage CARLA repositories.
- [**Make**](http://gnuwin32.sourceforge.net/packages/make.htm) generates the executables. It is necessary to use **Make version 3.81**, otherwise the build may fail. If you have multiple versions of Make installed, check that you are using version 3.81 in your PATH when building CARLA. You can check your default version of Make by running `make --version`.
- [**7Zip**](https://www.7-zip.org/) is a file compression software. This is required for automatic decompression of asset files and prevents errors during build time due to large files being extracted incorrectly or partially.
- [**Python3 x64**](https://www.python.org/downloads/) is the main scripting language in CARLA. Having a x32 version installed may cause conflict, so it is highly advisable to have it uninstalled.

!!! Important
Be sure that the above programs are added to the [environment path](https://www.java.com/en/download/help/path.xml). Remember that the path added should correspond to the progam's `bin` directory.

#### Python dependencies

Starting with CARLA 0.9.12, users have the option to install the CARLA Python API using `pip3`. Version 20.3 or higher is required. To check if you have a suitable version, run the following command:

```sh
pip3 -V
```

Python version should be [3.7](https://www.python.org/downloads/release/python-379/) with numpy version 1.21.6

If you need to upgrade:

```sh
pip3 install --upgrade pip
```

You must install the following Python dependencies:

```sh
pip3 install --user setuptools
pip3 install --user wheel
```

#### Major installations

##### Visual Studio 2019

Get the 2019 version of Visual Studio from [here](https://www.techspot.com/downloads/7241-visual-studio-2019.html). Choose **Community** for the free version. Use the _Visual Studio Installer_ to install three additional elements:

- **Windows 8.1 SDK.**, **Windows 10 SDK (10.0.16xxx.x and 10.0.19xxx.x)** Select it in the _Installation details_ section on the right or go to the _Indivdual Components_ tab and look under the _SDKs, libraries, and frameworks_ heading.
- **x64 Visual C++ Toolset.** In the _Workloads_ section, choose **Desktop development with C++**. This will enable a x64 command prompt that will be used for the build. Check that it has been installed correctly by pressing the `Windows` button and searching for `x64`. Be careful **not to open a `x86_x64` prompt**.
- **.NET framework 4.6.2**. In the _Workloads_ section, choose **.NET desktop development** and then in the _Installation details_ panel on the right, select `.NET Framework 4.6.2 development tools`. This is required to build Unreal Engine.

!!! Important
Other Visual Studio versions may cause conflict. Even if these have been uninstalled, some registers may persist. To completely clean Visual Studio from the computer, go to `Program Files (x86)\Microsoft Visual Studio\Installer\resources\app\layout` and run `.\InstallCleanup.exe -full`

!!! Note
It is also possible to use Visual Studio 2022 using the above steps and substituting the Windows 8.1 SDK for the Windows 11/10 SDK. To override the default Visual Studio 2019 Generator in CMake, specify GENERATOR="Visual Studio 17 2022" when using the makefile commands (see [table](build_windows.md#other-make-commands)). You may specify any generator that works with the build commands as specific in the build scripts, for a full list run `cmake -G` (Ninja has been tested to work for building LibCarla so far).

##### Unreal Engine

Starting with version 0.9.12, CARLA uses a modified fork of Unreal Engine 4.26. This fork contains patches specific to CARLA.

Be aware that to download this fork of Unreal Engine, **you need to have a GitHub account linked to Unreal Engine's account**. If you don't have this set up, please follow [this guide](https://www.unrealengine.com/en-US/ue4-on-github) before going any further.

To build the modified version of Unreal Engine:

**1.** In a terminal, navigate to the location you want to save Unreal Engine and clone the _carla_ branch:

```sh
    git clone --depth 1 -b carla https://github.com/CarlaUnreal/UnrealEngine.git .
```

!!! Note
Keep the Unreal Engine folder as close as `C:\\` as you can because if the path exceeds a certain length then `Setup.bat` will return errors in step 3. E.g. `C:\\UnrealEngine\`

**2.** Run the configuration scripts:

```sh
    Setup.bat
    GenerateProjectFiles.bat
```

**3.** Compile the modified engine:

> 1.  Open the `UE4.sln` file inside the source folder with Visual Studio 2019.

> 2.  In the build bar ensure that you have selected '**Development Editor**', '**Win64**' and '**UnrealBuildTool**' options. Check [this guide](https://docs.unrealengine.com/en-US/ProductionPipelines/DevelopmentSetup/BuildingUnrealEngine/index.html) if you need any help.

> 3.  In the solution explorer, right-click `UE4` and select `Build`.

(It is expected to show 3 succeeded, 0 failed, 0 up-to-date, 0 skipped, otherwise will cause errors later)

Launch UE and goes to Edit > Plugins > Virtual Reality > Enable SteamVR

**4.** Once the solution is compiled you can open the engine to check that everything was installed correctly by launching the executable `Engine\Binaries\Win64\UE4Editor.exe`.

!!! Note
If the installation was successful, this should be recognised by Unreal Engine's version selector. You can check this by right-clicking on any `.uproject` file and selecting `Switch Unreal Engine version`. You should see a pop-up showing `Source Build at PATH` where PATH is the installation path that you have chosen. If you can not see this selector or the `Generate Visual Studio project files` when you right-click on `.uproject` files, something went wrong with the Unreal Engine installation and you will likely need to reinstall it correctly.

!!! Important
A lot has happened so far. It is highly advisable to restart the computer before continuing.

## Part Two: Build Carla

**NOTE** You'll need a terminal on Linux/Mac. On Windows you'll be fine with the same x64 Native Tools CMD prompt that you used to build Carla.

```bash
mkdir CarlaDReyeVR && cd CarlaDReyeVR # doing everything in this "CarlaDReyeVR" directory

#####################################################
########### install OUR UnrealEngine fork ###########
#####################################################
# Rather than https://github.com/CarlaUnreal/UnrealEngine UE4, you SHOULD clone https://github.com/HARPLab/UnrealEngine
# but otherwise all instructions remain the same.

# Linux: https://carla.readthedocs.io/en/0.9.13/build_linux/#unreal-engine
# Windows: https://carla.readthedocs.io/en/0.9.13/build_windows/#unreal-engine

#####################################################
################### install Carla ###################
#####################################################
# Linux: https://carla.readthedocs.io/en/0.9.13/build_linux/
# Windows: https://carla.readthedocs.io/en/0.9.13/build_windows/
git clone https://github.com/carla-simulator/carla
cd carla
git checkout 0.9.13
# Solution for 404 error: change CONTENT_LINK in Update.bat to https://carla-assets.s3.us-east-005.backblazeb2.com/%CONTENT_ID%.tar.gz
./Update.sh # linux/mac
Update.bat # Windows

# before continue:
# 1. git clone https://github.com/carla-simulator/carla to other folder
# replace Util/InstallersWin and Util/BuildTools of 0.9.13 with those of 0.9.15
# 2. In Util/BuildTools/windows.mk remove dowloadplugins in line 74, and all contents from line 87 to end
# 3. In carla\Util\BuildTools\Setup.bat change Carla version (line 378) to 0.9.13
# 4. In carla\Util\InstallersWin\install_zlib.bat change zlib version (line 51) to 1.2.11
# 5. Download SDL2 from https://github.com/libsdl-org/SDL/releases/download/release-2.30.8/SDL2-devel-2.30.8-VC.zip , unzip and add those to Path:
# - SDL2_LIBRARY: SDL/src/
# - SDL2_INCLUDE_DIR: SDL/include/
# - SDL2_SDLMAIN_LIBRARY: SDL/

make PythonAPI
# ERROR with recast:copy SDL/ to ./Build/recast-src/RecastDemo/Contrib
# ERROR with boost: Install boost zip file manually and unzip it, and copy to Build/
# ERROR with dependencies: mkdir "PythonAPI/carla/dependencies/lib"
# ERROR: <error: reference to 'Server' is ambiguous> & <errof:reference to 'Client' is ambiguous>:
# --> at LibCarla/source/test/common/test_streaming.cpp Line 58, 93 Change Server<tcp::Server> to carla::streaming::low_level::Server<tcp::Server> | Line 63, 96 change Client<tcp::Client> to carla::streaming::low_level::Client<tcp::Client>

make launch # to build vanilla Carla

```

## Part Three: Install Plugins

```bash
#####################################################
############## install DReyeVR plugins ##############
#####################################################
# (optional) install SRanipal (eye tracking)
# install SRanipal from: https://github.com/scivi-tools/scivi.ue.board unzip Vive-SRanipal-Unreal-Plugin.zip
mv /PATH/TO/Plugins/SRanipal Unreal/CarlaUE4/Plugins/

# (optional) install LogitechWheelPlugin (steering wheel)
git clone https://github.com/HARPLab/LogitechWheelPlugin
mv LogitechWheelPlugin/LogitechWheelPlugin Unreal/CarlaUE4/Plugins/ # install to carla

cd .. # back to main directory

#####################################################
############## install scenario_runner ##############
#####################################################
# (optional) while you don't NEED scenario runner, it is certainly useful from a research pov
git clone https://github.com/carla-simulator/scenario_runner -b v0.9.13

#####################################################
################## install DReyeVR ##################
#####################################################
git clone https://github.com/HARPLab/DReyeVR
cd DReyeVR
# the CARLA= and SR= variables are optional
make install CARLA=../carla SR=../scenario_runner
# or
make install CARLA=../carla
make install SR=../scenario_runner

# run filesystem checks after installing
make check CARLA=../carla
cd ..

#####################################################
################## build everything #################
#####################################################
cd carla
# ERROR set was unexpected ... --> Change %GENERATOR% to "%GENERATOR%" in rpclib, gtest, recast, proj in Util/InstallersWin/

make PythonAPI  # build the PythonAPI (and LibCarla) again
make launch     # launch in editor
make package    # create an optimized package
# (from UE) ERROR: Failed to build "C:/UnrealEngine/Engine/Programs/AutomationTool/Saved\UATTempProj.proj": Open VisualStudio: HoloLens.Automation->References: and in the Browse section, which was initially an empty list, click on browse: and locate the Windows.winmd file from the desired Windows SDK.
make check      # run Carla unit tests
# ERROR: Assertion Failed: ResourceTableFrameCounter == INDEX_NONE ...
# --> In Unreal/CarlaUE4/Config/DefaultEngine.ini change DefaultGraphicsRHI_DX11 → DefaultGraphicsRHI_DX12
```

<br>

## Simple install

Technically, the above prerequisites are all you really need to install DReyeVR and get a barebones VR ego-vehicle with **no eyetracking** and **no racing wheel integration**. If this suits your needs, simply skip down to the [Install DReyeVR Core](Install.md#installing-dreyevr-core) section of this doc and set the following variables in `Unreal/CarlaUE4/Source/CarlaUE4/CarlaUE4.Build.cs` to `false`:

```c#
/////////////////////////////////////////////////////////////
// Edit these variables to enable/disable features of DReyeVR
bool UseSRanipalPlugin = true;
bool UseLogitechPlugin = true;
...
/////////////////////////////////////////////////////////////
```

- NOTE: you only need to install the SRanipal plugin if `UseSRanipalPlugin` is enabled, and similarly you only need to install the Logitech plugin if `UseLogitechPlugin` is enabled.

# Installing DReyeVR Plugins

Before installing `DReyeVR`, we'll also need to install the dependencies:

- **SteamVR**: Required for VR
- **SRanipal\***: Required for eye tracking (with HTC Vive Pro Eye), optional otherwise
- **LogitechWheelPlugin\***: Required for Logitech Steering Wheel, optional otherwise

(\* = optional, depends on the features you are looking for)

## SteamVR

### **Download Steam and SteamVR**

- You'll need to use [SteamVR](https://store.steampowered.com/app/250820/SteamVR/) for the VR rendering environment, so you should first download the [Steam client application](https://store.steampowered.com/about/).
  - From within the steam client, you can browse in store->search "[SteamVR](https://store.steampowered.com/app/250820/SteamVR/)" and download the free-to-install system utility.
    <!-- - In the Editor for Carla go to `Settings->Plugins->Virtual Reality->SteamVR` and enable the plugin -->
    <!-- - Note that on Linux this you may need to install it through the [Valve GitHub repo](https://github.com/ValveSoftware/SteamVR-for-Linux) -->
    <!-- - <img src = "Figures/Install/steamvr-enabled.jpg" alt="UE4DropDown" width=50%> -->
- You should be able to launch SteamVR from the client and in the small pop-up window reach both settings and "show VR view"
  - Make sure to calibrate the VR system to your setup and preferences!
- Additionally we recommend disabling the "Motion Smoothing" effect within SteamVR Settings to avoid nasty distortion artifacts during rendering.
  - <img src = "Figures/Install/steamvr-settings.jpg" alt="SteamVR-settings" width=50%>

---

## HTC Eye Tracker Plugin

### **Download `SRanipal`**

0. _What is SRanipal?_
   - We are using [HTC's SRanipal plugin](https://developer.vive.com/resources/vive-sense/sdk/vive-eye-tracking-sdk-sranipal/) as the means to communicate between Unreal Engine 4 and the Vive's Eye Tracker.
   - To learn more about SRanipal and for **first-time-setup**, see this [guide on foveated rendering using SRanipal](https://forum.vive.com/topic/7434-getting-started-with-vrs-foveated-rendering-using-htc-vive-pro-eye-unreal-engine/) by HTC developer MariosBikos_HTC
1. You'll need a (free-to-create) [Vive developer account](https://hub.vive.com/sso/login) to download the following:
   - a) [`VIVE_SRanipalInstaller_1.3.2.0.msi`](https://hub.vive.com/en-US/download/VIVE_SRanipalInstaller_1.3.2.0.msi) -- executable to install Tobii firmware
   - b) [`SDK_v1.3.0s.0.zip`](https://github.com/scivi-tools/scivi.ue.board) -- includes the Unreal plugin
     - **IMPORTANT**: The SRanipal versions above 1.3.6.0 are NOT supported and cause wild crashes!
   - **If the download links above don't work for you, make sure you have a Vive Developer account! (Or [contact](mailto:gustavo@silvera.cloud) us directly to help you)**
2. Install the Tobii firmware by double-clicking the `.msi` installer
   - Once installed, you should see the `SR_runtime.exe` program available from the Start Menu. Launch it as administrator and you should see the robot head icon in the Windows system tray as follows:
   - ![SR_runtime](https://mariosbikos.com/wp-content/uploads/2020/02/image-30.png)
     - Image Credit: [MariosBikos](https://forum.htc.com/topic/7434-getting-started-with-vrs-foveated-rendering-using-htc-vive-pro-eye-unreal-engine)

### **Installing SRanipal UE4 Plugin**

- After downloading the `.zip` file, unzipping it should present a directory similar to this
  - ```
    SDK
    - 01_C/
    - 02_Unity/
    - 03_Unreal/
    - Eye_SRanipal_SDK_Guide.pdf
    - Lip_SRanipal_SDK_Guide.pdf
    ```
  - Then, unzip the SRanipal unreal plugin and copy over the `03_Unreal/Plugins/SRanipal/` directory to the Carla installation
  - ```bash
    # in SDK/
    cd 03_Unreal
    unzip Vive-SRanipal-Unreal-Plugin.zip # creates the PLugins/SRanipal folder
    # assumes CARLA_ROOT is defined, else just use your Carla path
    cp -r Plugins/SRanipal $CARLA_ROOT/Unreal/CarlaUE4/Plugins/
    ```
- It is recommended to re-calibrate the SRanipal eye tracker plugin for every new participant in an experiment. You can do this by entering SteamVR home, and clicking the small icon in the bottom menu bar to calibrate eye tracker to the headset wearer.
  - ![Calibration](Figures/Install/steamvr-home.jpg)
  - You can find more information by checking out this [guide on foveated rendering using SRanipal](https://forum.vive.com/topic/7434-getting-started-with-vrs-foveated-rendering-using-htc-vive-pro-eye-unreal-engine/) by HTC developer MariosBikos_HTC

---

## Logitech Wheel Plugin

### **Installing Logitech Wheel Plugin**

- This is only for those who have a Logitech steering wheel/pedals driving setup. This hardware is not required to experience the VR experience (you can simply use keyboard/mouse) but greatly adds to the immersion and allows for granular analog controls.
  - For reference, we used this [Logitech G923 Racing Wheel & Pedals](https://www.logitechg.com/en-us/products/driving/driving-force-racing-wheel.html).
- We'll be using this [LogitechWheelPlugin](https://github.com/HARPLab/LogitechWheelPlugin) to interact with UE4 and map hardware inputs to actions.
  - Clone the repo and move the requisite folder to the Carla plugins folder
  - ```bash
    git clone https://github.com/HARPLab/LogitechWheelPlugin
    mv LogitechWheelPlugin/LogitechWheelPlugin $CARLA_ROOT/Unreal/CarlaUE4/Plugins
    ```
  - You should then see a Logitech Plugin enabled when you boot up the editor again:
  - ![LogitechPlugin](Figures/Install/LogitechPlugin.jpg)

---

## Sanity Check

- After installing these plugins, you should see a `Unreal/CarlaUE4/Plugins` that looks like this:
- ```
  Plugins
  ├── Carla                              # unchanged
  │   └── ...
  ├── CarlaExporter                      # unchanged
  │   └── ...
  ├── LogitechWheelPlugin                # if installed
  │   ├── Binaries
  │   ├── Doc
  │   ├── Logitech
  │   ├── LogitechWheelPlugin.uplugin
  │   ├── Resources
  │   └── Source
  └── SRanipal                           # if installed
      ├── Binaries
      ├── Config
      ├── Content
      ├── Resources
      ├── Source
      └── SRanipal.uplugin
  ```
- If you still have questions or issues, feel free to post an issue on our [Issues](https://github.com/HARPLab/DReyeVR/issues) page and we'll try our best to help you out.

<br>
<br>

# Installing `DReyeVR` Core

<!-- (Once you are done with this step, you should have a carla repo that looks just like this [Carla fork](https://github.com/HARPLab/carla/tree/DReyeVR-0.9.13) we created with the installation (and other minor things) pre-applied.) -->

- **IMPORTANT** The installation requires that `make`, `python` and `git` are available on your shell.
- You only need to install to a `CARLA` directory, ScenarioRunner is optional.
  - If you don't provide the `make` variables `CARLA=...` or `SR=...` the installation wizard will automatically detect your install destination by looking at the environment variables `CARLA_ROOT` and `SCENARIO_RUNNER_ROOT` required by Carla.

```bash
# the CARLA= and SR= variables are optional
make install CARLA=../carla SR=../scenario_runner
# or
make install CARLA=../carla
make install SR=../scenario_runner

# run filesystem checks after installing
make check CARLA=../carla
```

**NOTE:** to learn more about how the DReyeVR `make` system works, see [`Scripts/README.md`](../Scripts/README.md)

# Building `DReyeVR` PythonAPI

## Using [`conda`](https://www.anaconda.com/products/distribution) for the PythonAPI

- While not required for DReyeVR, we highly recommend compartmentalizing Python installations via isolated environments such as [`anaconda`](https://www.anaconda.com/products/distribution)
  - First download and install Anaconda to your machine from [here](https://www.anaconda.com/products/distribution).

```bash
# in /PATH/TO/CARLA/
conda create --name carla13 python=3.7.2
conda activate carla13 # will need to run this ONCE before opening a new terminal!
conda install numpy
```

- **READ THIS FIRST (Linux)**: You might run into a problem when compiling Boost 1.72.0 (required by `LibCarla`).
    <details>

    <summary> Show instructions to get Anaconda working on Linux </summary>

  - ```bash
      # find anaconda install:
      which python3
      ... PATH/TO/ANACONDA/envs/carla/bin/python3 # example output

      # go to carla/install dir from here
      cd PATH/TO/ANACONDA/envs/carla/include

      # create a symlink between python3.7 -> python3.7m
      ln -s python3.7m python3.7
    ```

    Install `boost_1_72_0.tar.gz` manually from https://github.com/jerry73204/carla/releases/tag/fix-boost and place file in `Build/boost_1_72_0.tar.gz`
    Open `Util/BuildTools/Setup.sh` (or `Util/BuildTools/Setup.bat` on Windows)
    Find the section named `Get boost` includes and comment out the `wget` lines.
    Now when you `make LibCarla` again, the `boost` errors should be resolved.

    - For more information see the bottom of this [SO post](https://stackoverflow.com/questions/42839382/failing-to-install-boost-in-python-pyconfig-h-not-found)
    </details>

  - **READ THIS FIRST (Windows)**: Windows anaconda is a bit more of a pain to deal with.
    <details>

    <summary> Show instructions to get Anaconda working on Windows </summary>

    1. Enable your environment
       ```bat
       conda activate carla13
       ```
    2. Add carla to "path" to locate the PythonAPI and ScenarioRunner. But since Anaconda [does not use the traditional `PYTHONPATH`](https://stackoverflow.com/questions/37006114/anaconda-permanently-include-external-packages-like-in-pythonpath) you'll need to:
       - 3.1. Create a file `carla.pth` in `\PATH\TO\ANACONDA\envs\carla\Lib\site-packages\`
       - 3.2. Insert the following content into `carla.pth`:
         ```bat
           C:\PATH\TO\CARLA\PythonAPI\carla\dist
           C:\PATH\TO\CARLA\PythonAPI\carla\agents
           C:\PATH\TO\CARLA\PythonAPI\carla
           C:\PATH\TO\CARLA\PythonAPI
           C:\PATH\TO\CARLA\PythonAPI\examples
           C:\PATH\TO\SCENARIO_RUNNER\
         ```
    3. Install the specific carla wheel (`whl`) to Anaconda

       ```bash
       conda activate carla13
       pip install --no-deps --force-reinstall PATH\TO\CARLA\PythonAPI\carla\dist\carla-0.9.13-cp37-cp37m-win_amd64.whl

       # if applicable (and you installed Scenario runner)
       cd %SCENARIO_RUNNER_ROOT%
       pip install -r requirements.txt # install all SR dependencies
       ```

    4. Finally, you might run into problems with `shapely` (scenario-runner dependency) and Conda. Luckily the solution is simple:
       - Copy the files:
         - `PATH\TO\ANACONDA\envs\carla13\Lib\site-packages\shapely\DLLs\geos.dll`
         - `PATH\TO\ANACONDA\envs\carla13\Lib\site-packages\shapely\DLLs\geos_c.dll`
       - To destination:
         - `PATH\TO\ANACONDA\envs\carla13\Library\bin\`
    5. Now finally, you should be able to verify all PythonAPI actions work as expected via:
       ```bat
       conda activate carla13
       python
       >>> Python 3.7.2 (default, Feb 21 2019, 17:35:59) [MSC v.1915 64 bit (AMD64)] :: Anaconda, Inc. on win32
       >>> Type "help", "copyright", "credits" or "license" for more information.
       >>> import carla
       >>> from DReyeVR_utils import find_ego_vehicle
       >>> from scenario_runner import ScenarioRunner
       ```
       With all these imports passing (no error/warning messages), you're good to go!

  </details>

Now you can finally build the PythonAPI to this isolated conda environment.

```bash
conda activate carla13
(carla13) make PythonAPI # builds LibCarla and PythonAPI to your (conda) python environment
```

- NOTE: You'll need to run `conda activate carla13` every time you open a new terminal if you want to build DReyeVR since the shell needs to know which python environment to use. Luckily this minor inconvenience saves us from the significant headaches that arise when dealing with multiple `python` projects, Carla installations, and versions, etc.

## Sanity Check:

It is nice to verify that the Carla PythonAPI is correctly visible in conda, to do this you should see all the following attributes in the `carla` module once calling `dir` on it.

<details>
<summary>Show instructions to verify Carla PythonAPI is installed </summary>

```bash
# in your terminal (linux) or cmd (Windows)
conda activate carla13   # (if using conda)
(carla13) python         # should default to python3!!
```

```python
#in Python
...
>>> import carla
>>> dir(carla)
# the following output means carla is not correctly installed :(
>>> ['__doc__', '__file__', '__loader__', '__name__', '__package__', '__path__', '__spec__']

# OR the following output means carla is installed :)
>>> ['Actor', 'ActorAttribute', 'ActorAttributeType', 'ActorBlueprint', 'ActorList', 'ActorSnapshot', 'ActorState', 'AttachmentType', 'BlueprintLibrary', 'BoundingBox', 'CityObjectLabel', 'Client', 'ClientSideSensor', 'CollisionEvent', 'Color', 'ColorConverter', 'DReyeVREvent', 'DVSEvent', 'DVSEventArray', 'DebugHelper', 'EnvironmentObject', 'FakeImage', 'FloatColor', 'GearPhysicsControl', 'GeoLocation', 'GnssMeasurement', 'IMUMeasurement', 'Image', 'Junction', 'LabelledPoint', 'Landmark', 'LandmarkOrientation', 'LandmarkType', 'LaneChange', 'LaneInvasionEvent', 'LaneInvasionSensor', 'LaneMarking', 'LaneMarkingColor', 'LaneMarkingType', 'LaneType', 'LidarDetection', 'LidarMeasurement', 'Light', 'LightGroup', 'LightManager', 'LightState', 'Location', 'Map', 'MapLayer', 'MaterialParameter', 'ObstacleDetectionEvent', 'OpendriveGenerationParameters', 'OpticalFlowImage', 'OpticalFlowPixel', 'Osm2Odr', 'Osm2OdrSettings', 'RadarDetection', 'RadarMeasurement', 'Rotation', 'SemanticLidarDetection', 'SemanticLidarMeasurement', 'Sensor', 'SensorData', 'ServerSideSensor', 'TextureColor', 'TextureFloatColor', 'Timestamp', 'TrafficLight', 'TrafficLightState', 'TrafficManager', 'TrafficSign', 'Transform', 'Vector2D', 'Vector3D', 'Vehicle', 'VehicleControl', 'VehicleDoor', 'VehicleLightState', 'VehiclePhysicsControl', 'VehicleWheelLocation', 'Walker', 'WalkerAIController', 'WalkerBoneControlIn', 'WalkerBoneControlOut', 'WalkerControl', 'Waypoint', 'WeatherParameters', 'WheelPhysicsControl', 'World', 'WorldSettings', 'WorldSnapshot', '__builtins__', '__cached__', '__doc__', '__file__', '__loader__', '__name__', '__package__', '__path__', '__spec__', 'bone_transform', 'bone_transform_out', 'command', 'libcarla', 'vector_of_bones', 'vector_of_bones_out', 'vector_of_gears', 'vector_of_ints', 'vector_of_transform', 'vector_of_vector2D', 'vector_of_wheels']
```

</details>

<br>

## Future modifications

Additionally, if you make changes to the `PythonAPI` source and need to rebuild (`make PythonAPI` again) when using Conda you should reinstall the `.whl` file to ensure your changes will be reflected in the environment:

- ```bash
  conda activate carla

  pip install --no-deps --force-reinstall /PATH/TO/CARLA/PythonAPI/carla/dist/carla-YOUR_VERSION.whl
  ```

# Upgrading `DReyeVR`

If you currently have an older version of `DReyeVR` installed and want to upgrade to a newer version, it is best to re-install DReyeVR from a fresh Carla repository. You can manually delete the `carla` repository and re-clone it directly (carefully ensuring the versions match) or use our provided scripts which attempt to reset the repository for you:

<details>
<summary> Show instructions to use DReyeVR scripts to reset CARLA repo</summary>

**IMPORTANT:** the `DReyeVR` clean script will overwrite and reset the Carla repository you specify, so make your backups now if you have any unstaged code. (`git reset --hard HEAD` and `git clean -fd` will be invoked, so if you commit your local changes they will be safe)

```bash
# first go to CARLA and clean it so no old DReyeVR builds linger
cd /PATH/TO/Carla/
make clean

# it is a good idea to clean the Content/ directory which is not tracked by Carla's git system
rm -rf Unreal/CarlaUE4/Content/

# re-install the Content fresh from Carla's servers
./Update.sh # Linux/Mac
Update.bat  # Windows

# next, go to DReyeVR and get the latest updates
cd /PATH/TO/DReyeVR/
git pull origin main # or dev, or whatever branch

# next, run the DReyeVR-cleaner to automatically hard-reset the Carla repo
# accept the prompt to hard-clean CARLA, note that this will reset tracked and remove untracked files
make clean CARLA=/PATH/TO/CARLA SR=/PATH/TO/SR # both args are optional

# now, you can cleanly install DReyeVR over Carla again
make install CARLA=/PATH/TO/CARLA SR=/PATH/TO/SR # both args are optional

# it's a good idea to check that the Carla repository has all the expected files
make check CARLA=/PATH/TO/CARLA SR=/PATH/TO/SR # both args are optional

# finally, you can go back to Carla and rebuild
cd /PATH/TO/Carla
make PythonAPI
make launch
make package
```

---

As long as you have no errors in the previous sections, you should be able to just build the `Carla` project with our `DReyeVR` files as follows:

</details>

<br>
<br>

# Building `DReyeVR` UE4

If you are not interested in using SRanipal or the LogitechWheelPlugin, you can disable these at compile-time by changing the variables in `Unreal/CarlaUE4/Source/CarlaUE4/CarlaUE4.Build.cs` to `false`:

```c#
  /////////////////////////////////////////////////////////////
  // Edit these variables to enable/disable features of DReyeVR
  bool UseSRanipalPlugin = true;
  bool UseLogitechPlugin = true;
  ...
  /////////////////////////////////////////////////////////////
```

Finally, open the project directory in any terminal (Linux/Mac) or `Windows x64 Native Tools Command Prompt for VS 2019` (Windows) and run:

```bash
make PythonAPI  # build the PythonAPI & LibCarla

make launch     # build the development UE4 game in editor

make package    # build the optimized UE4 packaged game (shipping), if error with this command, comment lines 151-153, 163-165 in carla/LibCarla/source/test/server/test_benchmark_streaming.cpp
```

# Running `DReyeVR`

With the shipping package built, run the Carla (with DReyeVR installed) executable in VR mode with:

```bash
# on Unix
cd /PATH/TO/CARLA/Dist/CARLA_Shipping_0.9.13-dirty/LinuxNoEditor/ # or MacNoEditor on MacOS
./CarlaUE4.sh -vr

# on Windows x64 Visual C++ Toolset
cd \PATH\TO\CARLA\Build\UE4Carla\0.9.13-dirty\WindowsNoEditor\
CarlaUE4.exe -vr
# CarlaUE4.exe -vr -quality-level=Low
# Optional flag: -quality-level=Low
```

**NOTE:** To greatly boost the framerates without losing much visual fidelity you can run with the additional argument `-quality-level=Low` which we modified from vanilla Carla to preserve the same rendering distance.

**NOTE 2** You also don't necessarily NEED to run DReyeVR in VR. If you omit the `-vr` flag then you will be greeted with a flat-screen Carla game with the same features available for DReyeVR, just not in VR.

<br>

# Now what?

Now that you've successfully installed DReyeVR continue to [`Usage.md`](Usage.md) to learn how to use DReyeVR for your own VR driving research simulator.
