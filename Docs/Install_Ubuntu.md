# Installing Carla on Ubuntu 20.04
This guide is for installing Carla 0.9.13 (along with Unreal Engine 4.26) build on Ubuntu 20.04.5 (Focal Fosca). It is largely based on [this tutorial](https://carla.readthedocs.io/en/latest/build_linux/) by Carla, and can be thought of as a shorter updated of it specifically for Ubuntu 20.04 and DReyeVR. Check out the original tutorial tutorial for more detailed explanations of some the commands. Commands themselves, however, slightly differ from the original version.

## Part One: Prerequisites
- **Ubuntu 20.04**
- **130 GB disk space.** Carla will take around 31 GB and Unreal Engine will take around 91 GB so have about 130 GB free to account for both of these plus additional minor software installations. 
- **An adequate GPU**. CARLA aims for realistic simulations, so the server needs at least a 6 GB GPU although 8 GB is recommended. A dedicated GPU is highly recommended for machine learning. 
- **Two TCP ports and good internet connection.** 2000 and 2001 by default. Make sure that these ports are not blocked by firewalls or any other applications. 
- **IMPORTANT: Proprietary drivers installed.** Please make sure that you are using the latest stable version of proprietary drivers (for instance, Nvidia). Check [this guide](https://linuxconfig.org/how-to-install-the-nvidia-drivers-on-ubuntu-20-04-focal-fossa-linux) for installing and updating the Nvidia driver on Ubuntu 20.04.

## Installing software requirements
Please use the following set of commands to install the necessary software packages. 
**Note**: you might want to go over each of the listed packages separately to ensure that each was installed correctly to avoid issues during the installation. Later on, the missing packages will be harder to track. 
```bash
sudo apt-add-repository "deb http://apt.llvm.org/focal/ llvm-toolchain-focal main"
sudo apt-get install build-essential clang-8 lld-8 g++-7 cmake ninja-build libvulkan1 python-dev python3-dev python3-pip libpng-dev libtiff5-dev libjpeg-dev tzdata sed curl unzip autoconf libtool rsync libxml2-dev git
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/lib/llvm-8/bin/clang++ 180 &&
sudo update-alternatives --install /usr/bin/clang clang /usr/lib/llvm-8/bin/clang 180
```

#### Clang issue:
One of the issues specific to Ubuntu 20.04 installation is the **clang** compiler. The CARLA team uses clang-8 and LLVM's libc++. while clang-8 can be installed on Ubuntu 20.04, it is rather outdated and does not catch some of the existing bugs within carla. If you want to use a different version of clang and libc++, follow the instructions below:
<details>
<summary>how to use a different clang version</summary>
Uninstall clang-8 (installed if you executed the previous command):

```bash
sudo apt-get remove clang-8 lld-8
sudo rm /usr/bin/clang /usr/bin/clang++
```

Install version 10:
```bash
sudo apt-get install clang-10 lld-10
```
Set up clang and clang++ commands:
```bash
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/lib/llvm-10/bin/clang++ 180 &&
sudo update-alternatives --install /usr/bin/clang clang /usr/lib/llvm-10/bin/clang 180
```
Create a symbolic link that would tell the system to use version 10 whenever it encounters `usr/bin/clang-8` in one of the CARLA setup or installation scripts:
```bash
sudo ln -s /usr/bin/clang /usr/bin/clang-8
sudo ln -s /usr/bin/clang++ /usr/bin/clang++-8
```
</details>
<br>


### Python dependencies
Version 20.3 or higher of **pip3** is required. To check if you have a suitable version, run the following command:
```bash
pip3 -V
```
If you need to upgrade:
```bash
pip3 install --upgrade pip
```
You might be prompted to add the directory of the newer version to PATH. If yes, do so.

You must install the following Python dependencies:
```
pip3 install --user -Iv setuptools==47.3.1 &&
pip3 install --user distro &&
pip3 install --user wheel auditwheel &&
pip3 install nose2
```
Some of the installation and setup scripts use the deprecated **python** command. One way to avoid the issues caused by this is with a symbolic link:
```
sudo ln -s /usr/bin/python3 /usr/bin/python
```
If your file is named other than **python3**, substitute python 3 for your python version.
<br>

### Unreal Engine
This section does not require updates or modifications. Please follow the exact steps from the corresponding section of [this tutorial](https://carla.readthedocs.io/en/latest/build_linux/).
<br> 

## Part Two: Build CARLA
**Note**: downloading aria2 with `sudo apt-get install aria2` will speed up the following commands.

Clone the CARLA repository:
```bash
git clone https://github.com/carla-simulator/carla -b 0.9.13
```
Once download is finished, from the new **carla** directory, run
```bash
Update.sh
```

Navigate to the **carla** directory. Open **./Util/BuildTools/Setup.sh** and replace `XERCESC_VERSION=3.2.3` (line 428) with `XERCESC_VERSION=3.2.4`. Next, open **./Util/BuildTools/BuildOSM2ODR.sh**, and replace all instances of `xerces-c-3.2.3` with `xerces-c-3.2.4`.


Follow the rest of the instructions from the [tutorial](https://carla.readthedocs.io/en/latest/build_linux/).


Now that you have a working CARLA 0.9.13 build, go back to the [DReyeVR installation guide](https://github.com/HARPLab/DReyeVR/blob/main/Docs/Install.md) and follow the rest of the steps to install DReyeVR on top of it.

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
