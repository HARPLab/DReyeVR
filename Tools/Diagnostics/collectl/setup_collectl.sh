#!/bin/bash

R='\033[0;31m'         # Red
G='\033[0;32m'         # Green
Y='\033[0;33m'         # Yellow
B='\033[0;34m'         # Blue
NC='\033[0m'           # No Color

# first install collectl (native on Linux, or WSL on Windows) 
# apache works fine for the Web stuff
sudo apt install collectl && echo -e "${G}Successfully installed collectl${NC}"

# in order to monitor Nvidia cards (gpu usage & memory) we need to pass the --import nvidia flag to collectl
# however, this requires nvidia.ph which does not work natively, our patched version does as it directly
# calls nvidia-smi to query the gpu
sudo cp ./nvidia.ph /usr/share/collectl/
sudo cp ./nvidia.ph /etc/perl/

# Windows only!
system="$(uname -a)"
if [[ $system == *"Microsoft"* ]];
then
    # make a symlink to the Windows nvidia-smi executable:
    # as per this question https://stackoverflow.com/questions/57100015/how-do-i-run-nvidia-smi-on-windows
    # the executable is stored somewhere like: C:\Windows\System32\DriverStore\FileRepository\nvdm*\nvidia-smi.exe
    ln -s /mnt/c/Windows/System32/DriverStore/FileRepository/nv_dispi.inf_*/nvidia-smi.exe nvidia-smi

    # then move it to /usr/bin/ in order to be callable from anywhere in WSL.
    # As per this https://stackoverflow.com/questions/38920710/how-can-i-run-a-windows-executable-from-wsl-ubuntu-bash
    # windows executables should be natively runnable from WSL
    sudo mv nvidia-smi /usr/bin/

    echo -e "${G}Successfully created nvidia-smi symlink for Windows${NC}"
fi
# ready to go!
echo "Ready!"