# Using Carla ROS bridge

## Our (Software) Adventure
It would be great if we could do everything on a single machine, but unfortunately software dependencies and limitations make it very difficult to accomplish this. In particular we can note the following impmortant dependencies:
- [`SRanipal`](https://developer.vive.com/resources/vive-sense/sdk/vive-eye-tracking-sdk-sranipal/) requires Windows. HTC only provides precompiled Windows (`.dll`) binaries for important libraries.
  - Reverse engineering the HTC Vive Pro's Eye tracker to work on Linux is possible but out of scope for this project.
- [`ROS`](http://wiki.ros.org/ROS/Installation) (basically) requires Linux for a proper [`ROS master`](http://wiki.ros.org/Installation) and `ROS` environment
  - However, unless you want to go through several flaming hoops compiling `ROS` from source for a [custom distro](http://wiki.ros.org/Distributions), you'll need to use **exactly** [`Ubuntu 20.04 LTS`](https://releases.ubuntu.com/20.04/) for `ROS Noetic` (which supports Python3) and the other `Ubuntu` flavours for corresponding ros builds
    - In our case we had already set up a Linux machine running [`Pop!_OS 20.10`](https://pop.system76.com/) (`Ubuntu` derivative) and did not want to dual boot/reinstall, so we instead took the approach of running our `ROS master` on a vanilla [`Ubuntu 20.04 LTS`](https://releases.ubuntu.com/20.04/) virtual machine. For explanations on how we managed to set this up (using `qemu`) see our [VM-guide](../ROS-bridge/README.md). 
  - [ROS Windows support](http://wiki.ros.org/Installation/Windows) is still in beta stage, we'd rather not use it.

Long story short... we'll need at least one Windows machine for `CarlaVR` + `SRanipal` Eye tracking, and at least one Linux machine (possibly with the exact `Ubuntu 20.04 LTS` virtual machine) for the `ROS master`.

We'd also like to use the compiled `PythonAPI` on the Windows build to interact with our (remote) `ROS master`, but this becomes difficult as well. The two major `ROS` python libraries are:
- [`rospy`](http://wiki.ros.org/rospy) which requires a fully functional "local `ROS` environment" which as described earlier is **not supported** on Windows
- [`roslibpy`](https://github.com/gramaziokohler/roslibpy) which only uses `WebSockets` and connects to `rosbridge 2.0` which both have **incompatibility** issues with `carla-ros-bridge`

The only option now is to use a `PythonAPI` that was compiled on `Linux` to use `rospy` and publish our custom topics. 
It is also highly reccomended to install the Windows Subsystem for Linux ([`WSL`](https://docs.microsoft.com/en-us/windows/wsl/install-win10)) to get proper ssh features and Linux commands. 

### Prerequisites:
- Windows machine for `SRanipal` eye tracking 
- Linux machine (`Ubuntu 20.05 LTS`) for `ROS` master and python scripts
  - Technically the python scripts do not need to be on the same machine as the `ROS master` (`Ubuntu 20.04 LTS` machine)

### Our pipeline outlined
Our configuration is as follows
- **Machine A**: our primary physical Linux machine running `Pop!_OS 20.10`
- **Machine B**: our primary physical Windows machine running our `Carla` build with `SRanipal` installed
- **Machine C**: our virtual Linux machine running `Ubuntu 20.04 LTS` for `ROS` with a `tap` network backend

**IMPORTANT:** We must first ensure that all machines are `ssh`-able to and from each other. 

For our case, giving **Machine C** a proper DHCP-assigned IP-address required a `tap` network backend as described in our VM-guide. Note that this passes through the `ethernet` (ex. `enp5s0`) from **Machine A** so it is no longer usable by **Machine A** which now defaults to using `Wifi` (ex. `wlp4s0`). 

Running `ifconfig` on **Machine A** gives us:

```properties
# (cleaned up for brevity)
br0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.86.42  ...
        ...
        ...

enp5s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        ...
        ...

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1 ...
        ...
        ...

tap0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        ...
        ...

wlp4s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.86.123  ...
        ...
        ...

```
Note the important `IP` addresses we care about: `192.168.86.42`(the bridge ip address [`tap0` + `enp5s0`]) for `ssh`ing from **Machine C** to **Machine A** and `192.168.86.123`(the wifi ip address [`wlp4s0`]) for `ssh`ing from anywhere else to **Machine A**.

Running `ifconfig` on **Machine B** gives us:

```properties
enp5s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.86.182 ...
        ...
        ...

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1 ...
        ...
        ...
```

This is the simplest setup since `192.168.86.182` is the ip address for `ssh`ing from anywhere to **Machine B**

Running `ifconfig` on **Machine C** gives us:

```properties
ens3: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 10.0.2.15 ...
        ...
        ...

ens5: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.86.33 ...
        ...
        ...

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1 ...
        ...
        ...
```

And as expected we have yet another IP address for `ssh`ing into **Machine C**, `192.168.86.33`. Feel free to ignore the `10.0.2.15` as that's a dummy network used in the default `qemu` vm's that we aren't using. 

#### Summary of our IP addresses:

We have: 
- `192.168.86.42` for **Machine C** -> **Machine A** (use with `rosbridge`)
- `192.168.86.123` for **X** -> **Machine A** (use with `ssh` and as `SELF_IP` in `python` scripts)
- `192.168.86.182` for **X** -> **Machine B** (use with `python` scripts to connect to Windows `Carla` simulator instance)
- `192.168.86.33` for **X** -> **Machine C** (use with `IP_ROSMASTER` in `python` scripts to connect to `ROS master`)

Recall that the VM IP addressess (**Machine C**) can be completely avoided if the secondary Linux machine is `Ubuntu 20.04 LTS`.

### Sourcing 

Additionally, before any `roslaunch` you'll need to source the `setup.bash` (or `.zsh` or `.sh`) and update the `PYTHONPATH` for the `ROS` setup. To do this in your shell you'll need to `source` the script as following:
```bash
source ./setup.sh
```

### Roslaunch

In order to run our custom ROS launchfile you can run the following:
```properties
# in our case we have 192.168.86.182:2000 as the Windows Carla instance
roslaunch carla_ros_bridge_DReyeVR.launch host:=192.168.86.182 port:=2000 fixed_delta_seconds:=0.0
```
This runs with a baseline asynchronous config with no fixed timestep. 
- This keeps the framerate from affecting the speed of the simulator, but is not reproducable accurately. 
- It would be better to run with a fixed timestep which would be fine if our simulator ran at a consistent fixed framerate. 
  - For now since we are still debugging it is fine to just let it run at variable framerates.

### Rostopic/rosbag

To see the published messages, simply use `rostopic` as per usual:
```properties
# list rostopic topics
$ rostopic list

/carla/debug_marker
/carla/status
/carla/weather_control
/carla/world_info
/clock
/rosout
/rosout_agg
```

To record into a `rosbag`, you can also simply use `rosbag record -a` as usual. The output of a `rosbag info` should look something like this (as you can see it includes the `/dreyevr_pub` topic).

```properties
$ rosbag info 2021-01-21-16-42-36.bag

path:        2021-01-21-16-42-36.bag
version:     2.0
duration:    6.0s
start:       Dec 31 1969 16:51:58.84 (3118.84)
end:         Dec 31 1969 16:52:04.87 (3124.87)
size:        4.7 MB
messages:    2972
compression: none [2/2 chunks]
types:       carla_msgs/CarlaStatus    [0a6e668a0d517dead8f5c68762fc1dab]
             carla_msgs/CarlaWorldInfo [7a3a7a7fc8c213a8bec2ce7928b0a46c]
             rosgraph_msgs/Clock       [a9c97c1d230cfc112e270351a944ee47]
             rosgraph_msgs/Log         [acffd30cd6b6de30f120938c17c593fb]
             std_msgs/String           [992ce8a1687cec8c8bd883ec73ca41d1]
topics:      /carla/status        979 msgs    : carla_msgs/CarlaStatus   
             /carla/world_info      1 msg     : carla_msgs/CarlaWorldInfo
             /clock              1007 msgs    : rosgraph_msgs/Clock      
             /dreyevr_pub         980 msgs    : std_msgs/String          
             /rosout                5 msgs    : rosgraph_msgs/Log         (2 connections)

```

To echo any message, similarly can use `rostopic echo /<topic>` as usual.


## Troubleshooting

### [WARN] 0 bytes were received
If you are not getting any data from the `rostopic echo /dreyevr_pub` output, and then also see a warning from the `ROS master` such as this:

`[WARN] [1611557662.983417, 172.775030]: Inbound TCP/IP connection failed: connection from sender terminated before handshake header received. 0 bytes were received. Please check sender for additional details.`

Then this is likely the case that your `IP_SELF` is incorrect in the python script. This is used to set the `ROS_IP` of the local machine in the environment, and should just be your machine's ip address from the local network. 
- NOTE FOR VM USERS: This can be different than usual if using the `tap0` network backend from our VM-guides since it will send one network (your ethernet) to the VM and the host machine may be left with a secondary network. 

### NameError: name 'Sensor' is not defined
This occurs because you did not apply the patch to the `actor_factory.py` located at `carla-ros-bridge/ros-bridge/carla_ros_bridge/src/carla_ros_bridge/actor_factory.py` which includes the proper `Sensor` import.

### RuntimeError: resolve: Host not found (authoritative)
It is very likely that either the wrong Host/port is being used to connect to the remote (Windows) machine, or the `ssh` configuration is not set up correctly. Double check the IP address is correct by testing via `ssh` to and from the remote machine, if (in WSL) the `systemctl ssh status` is not running then enable it.

### RospyError: Could not initialize rospy connection
This generally happens when you do not have the correct IP address and port of the `ROS master`. Double check that you are using the correct `IP_ROSMASTER` and `PORT_ROSMASTER` in your script for your setup.

### IndexError: list index out of range
Our implementation of sourcing the python .egg file (adding to system path) depends on calling `./test_ros_publish.py` from within the `examples/` directory in the packaged `PythonAPI`. You can tweak this to be whatever `cwd` you want, simply change the glob query on line 12 of `test_ros_publish.py`.

### ImportError: No module named carla
This occurs because of a faulty import for the python `.egg` file. As described above you should primarily be working in a `Linux` environment (not necessarily `Ubuntu 20.04 LTS` like for `ROS`) to use `rospy`. Double check that the `carla/dist/carla-0.9.10-...-linux-x86_64.egg` file exists. 

