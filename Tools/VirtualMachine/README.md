# Using a Virtual Machine for ROS master

In this guide we'll highlight the important features of a `kvm` accelerated `qemu` based virtual machine running `Ubuntu 20.04 LTS` for clean `ROS noetic` compatibility.  

### Reason?
Installing `ROS noetic` requires jumping through several flaming hoops if you want to install `ROS` on a different distribution than vanilla `Ubuntu 20.04 LTS`, for our case this becomes the easiest way to go. 

**However** if you are already daily-driving a Linux machine running `Ubuntu 20.04 LTS` then you can skip this portion all-together if you'd like and install the `ROS master` locally.

### What is qemu?
From [qemu.org](https://www.qemu.org/): "QEMU is a (FAST!) generic and open source machine emulator and virtualizer". This highlights some of the main features being "Full-system emulation" and "Virtualization" that we'll use. Additionally there is lots of documentation and community support for [qemu on Ubuntu](https://help.ubuntu.com/community/KVM/Installation) and its derivatives

### Creating the Virtual Machine
**Ideally have KVM support:**

First it will be important to have `kvm` support on your machine to run instructions directly on the CPU rather than virtualizing them, to verify this check `kvm-ok` in any shell.

**Installing Dependencies**
```properties
sudo apt-get install qemu-kvm qemu
# then reboot your system
```
It is advisable to reboot the system after installation.

**Creating the Virtual Machine**

- Note that ideally the VM has at least 20G of disk space, make sure your local disk has at least this much free space.
- The `.iso` system file can be downloaded from [Canonical's website](https://ubuntu.com/download/desktop)

```properties
# create a 20G virtual disk image file at ~/Ubuntu20.04/
qemu-img create ubuntu.img 20G

# download the Ubuntu 20.04.1 LTS iso from https://ubuntu.com/download/desktop
# and place the .iso in the same working directory

# then boot the Virtual machine for the first time with
# Qemu parameters:
#    -m: memory in MB passed to VM
#    -smp cores=X: number of CPU cores passed to VM
#    -cpu host: what kind of CPU is being emulated (use host to mirror CPU specs)
qemu-system-x86_64 -hda ubuntu.img -boot a -cdrom ./ubuntu-20.04.1-desktop-amd64.iso -enable-kvm -m 8192 -smp cores=8 -cpu host
```
Then follow the typical Ubuntu installation guide through the GUI installer. 

After the first installation/boot, the system can be launched with default `qemu-system-x86_64`. Feel free to delete the `.iso` file at this point.
```properties
qemu-system-x86_64 -hda ubuntu20.img -name FossaVM -boot a -m 8192 -smp cores=8 -cpu host -enable-kvm -vga virtio 
```

To run it with spice see below.

### Running with Spice for copy+paste
[Spice](https://spice-space.org/) is an "open source solution for remote access to virtual machines in a seamless way" for easy usb device sharing, copy paste, and an [improved grpahical experience](https://www.linux-kvm.org/page/SPICE) for KVM based virtual machines.

To do this, install the [spice-vdagent](https://gitlab.freedesktop.org/spice/linux/vd_agent) in the guest machine:
```properties
# in the guest machine
sudo apt install spice-vdagent
```

Then on the host machine, you'll need to use the [`remote-viewer`](http://manpages.ubuntu.com/manpages/bionic/man1/remote-viewer.1.html) to view the virtual machine's graphical interface.

To add this to the VM startup script see our [`FossaVM.sh`](FossaVM.sh) script we added
```properties
# first start the SPICE server by waiting (concurrently) for the VM to start
(sleep 0.3 && remote-viewer spice+unix://${SPICE_UNIX}) &
```
and 
```properties
-spice unix,addr=${SPICE_UNIX},disable-ticketing \
-device virtio-serial-pci \
-device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
-chardev spicevmc,id=spicechannel0,name=vdagent \
-monitor stdio \
```
Where `SPICE_UNIX` is a Unix socket (memory based socket, faster than TCP) defined in the script. Note that it is also possible to use a TCP host and port with `remote-viewer spice://${SPICE_ADDR}:${SPICE_PORT}` and `-spice port=${SPICE_PORT},addr=${SPICE_ADDR},disable-ticketing \`. 

### Setting up the Network backend 
Finally, our last hurdle is that when we try to connect to the virtual machine from the network, it is unavailable because the default `qemu` network backend is [completely emulated](https://qemu.readthedocs.io/en/latest/system/net.html) and thus not assigned a proper IP address for bidirectional ssh from our physical machines to the VM. 

Additionally, "the standard way to connect QEMU to a real network" is to use a **TAP** network interface where "QEMU adds a virtual network device on your host (called `tapN`), and you can then configure it as if it was a real ethernet card" ([source](https://qemu.readthedocs.io/en/latest/system/net.html)).

The simplest guide to follow to set this up is this [Setting up Qemu with a tap interface](https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c) gist. We have compiled a complete script to run through these commands in [`SetupTap.sh`](SetupTap.sh):

**Important** things to keep in mind:
- The `SetupTap.sh` script assumes your ethernet network (to be passthrough) is called `enp5s0`. To edit this see line 6
  - Note that it is only supported to pass through an ethernet connection (ex. can't pass through wifi) which will leave the host without an ethernet card. It is advisable to have multiple available networks so that the host can automatically switch to its backup network card.
- After running `./SetupTap.sh` to create a `tap0` network and `br0` bridge, if you no longer need these you can clean up the network setup back to normal by passing in a `clean` parameter

```properties
# will remove the tap0 and br0 network and revert the ethernet passthrough so it can be used again by the host
./SetupTap.sh clean
```

## Finally...
Once this is all set up it is simple to run the entire setup with the [`FossaVM.sh`](FossaVM.sh) and [`SetupTap.sh`](SetupTap.sh) scripts.
```properties
# start the TAP interface
./SetupTap.sh

# start the Ubuntu VM
./FossaVM.sh

...

# after closing the VM to revert changes to your network
./SetupTap.sh clean
```
