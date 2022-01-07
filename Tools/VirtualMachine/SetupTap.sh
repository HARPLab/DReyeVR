#!/bin/bash

# automated script from Tap setup found from:
# https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c

ETH0=enp5s0 # my personal ethernet config
# note that you can only use Ethernet for connecting to bridge

USER=`whoami`

# exit when any command fails
# set -e

if [[ $# -eq 0 ]]; # no arguments => setup tap
then    
    # create a bridge
    sudo brctl addbr br0 
	
    # clear ip of CONNECTION
    sudo ip addr flush dev $ETH0
	
    # add CONNECTION to bridge
    sudo brctl addif br0 $ETH0
    
    # create tap interface
    sudo tunctl -t tap0 -u $USER
    
    # add tap0 to bridge
    sudo brctl addif br0 tap0 
    
    # make sure everything is up
    sudo ifconfig $ETH0 up 
    sudo ifconfig tap0 up 
    sudo ifconfig br0 up 
    
    # check if properly bridged
    sudo brctl show 
    
    # assign ip to br0
    sudo dhclient -v br0 
    
    # message output
    echo "Successfully created tap0 network backend interface"

    exit 0
elif [[ $1 == "cleanup" ]] || [[ $1 == "clean" ]];
then
    # cleanup
    echo "CLEANING UP"
    
    # remove tap interface tap0 from bridge br0
    sudo brctl delif br0 tap0
    
    # delete tap0
    sudo tunctl -d tap0
    
    # remove eth0 from bridge
    sudo brctl delif br0 $ETH0
    
    # bring bridge down
    sudo ifconfig br0 down 
    
    # remove bridge
    sudo brctl delbr br0 
    
    # bring CONNECTION up
    sudo ifconfig $ETH0 up 
    
    # check if an IP is assigned to CONNECTION, if not request one
    sudo dhclient -v $ETH0 
    
    # message output
    echo "Successfully removed tap0 interface"

    exit 0
else
    echo "Use clean or cleanup to to remove the tap interface, or nothing to just setup"

    # error exit
    exit 1
fi

