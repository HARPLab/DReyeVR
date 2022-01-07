#!/bin/sh
SPICE_UNIX=fossa.unix.sock
FOSSA_IMG=ubuntu20.img # filename here

echo "STARTING QEMU VM WITH UNIX SOCKET: ${SPICE_UNIX}"
echo "Reconnect from any terminal with remote-viewer spice+unix://${SPICE_UNIX}"
echo "type \"quit\" to power off the VM when in (qemu) mode\n"

# first start the SPICE server by waiting (concurrently) for the VM to start
(sleep 0.3 && remote-viewer spice+unix://${SPICE_UNIX}) &

# start the VM as the foreground process
qemu-system-x86_64 \
    -drive file=${FOSSA_IMG},format=raw,if=virtio,cache=off \
    -name FossaVM \
    -m 16384 \
    -enable-kvm -cpu host \
    -smp cores=8 \
    -vga virtio \
    -spice unix,addr=${SPICE_UNIX},disable-ticketing \
    -device virtio-serial-pci \
    -device virtserialport,chardev=spicechannel0,name=com.redhat.spice.0 \
    -chardev spicevmc,id=spicechannel0,name=vdagent \
    -monitor stdio \
    -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no \
    -device e1000,netdev=mynet0,mac=52:55:00:d1:55:01 \
    -net nic -net user,smb="$CARLA_ROOT" 

