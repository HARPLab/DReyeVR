#!/bin/bash

R='\033[0;31m'         # Red
G='\033[0;32m'         # Green
Y='\033[0;33m'         # Yellow
B='\033[0;34m'         # Blue
NC='\033[0m'           # No Color

system="$(uname -a)" # used to check if running on Windows or Linux

# first the prerequisite environment variables
if [[ $system == *"Microsoft"* ]]; # Running on WSL (Windows benchmark)
then
    # Edit this string if on Windows! 
    # (Note that this calls the windows python.exe, and it is a WINDOWS directory, so start with Disk:/)
    CARLA_DIR="C:/Users/PATH/TO/CARLA/"
else
    # Edit this string if on Linux!
    CARLA_DIR="PATH/TO/CARLA/"
fi
# interval (s) between refreshes
INTERVAL=0.5

# other variables
collectl_data="collectl_data.csv"
carla_data="carla_metadata.csv"
out_directory="collectl"

# signal catching 
trap ctrl_c INT
function ctrl_c() {
    if [ $completed == 1 ];
    then
        echo -e "\nwrote ${collectl_data} in ${out_directory}"
        echo -e "wrote ${carla_data} in ${out_directory}"
        if [[ $system == *"Microsoft"* ]]; # Windows only!
        then
            python.exe ../python/combine_collectl.py -d "$out_directory" -c1 "$collectl_data" -c2 "$carla_data" -s $time_start -e $time_end
            python.exe ../python/graph_sys_diagnostics.py -d "$folder_name"
        else
            python3 ../python/combine_collectl.py -d "$out_directory" -c1 "$collectl_data" -c2 "$carla_data" -s $time_start -e $time_end
            python3 ../python/graph_sys_diagnostics.py -d "$folder_name"
        fi
    else
        echo -e "goodbye."
        exit 1
    fi
}
# flag that everything succeeded, gets set at the bottom of the script
completed=0

# pass in the file to write to
if [ "$1" != "" ];
then
    echo -e "${Y}Using custom directory ${1}${NC}"
    out_directory="$1"
fi

# get timestamp to encode time of execution into results
timestamp=$(date +"%F-%H-%M-%S.%1N" ) # gets date as yyyy-mm-dd-hh-mm-ss.ms

#name of the folder
out_directory="${out_directory}_${timestamp}"
folder_name="${out_directory}"

# output always goes in ./data/
out_directory="data/${out_directory}"

mkdir -p "$out_directory"

collectl_file="${out_directory}/${collectl_data}"
touch $collectl_file
# run collectl in background
# includes nvidia monitoring (requires our nvidia.ph patch)
# monitors cpu cores and memory (-sCm)
# prints data in a graph style "-P" timestamped with ms (-oTm)
# outputs data at intervals (-i) of 0.5s
# comma-separated (--sep ",")
collectl --import nvidia, -sCm -oTm -P -i 0.5 --sep "," --quiet > "$collectl_file" &
echo -e "${G}Started collecting system data to $out_directory${NC}"

# run timing script
# NOTE: assumes carla is terminated before this script
echo -e "Waiting for Carla to run..."
# inspired from https://stackoverflow.com/questions/7708715/check-if-program-is-running-with-bash-shell-script
PROC_NUM="0"

# NOTE: there is also CarlaUE4-Win64-Shipping which runs with the release build
# to see all running processes on Windows check ps on powershell

if [[ $system == *"Microsoft"* ]]; # Windows only!
then
    # powershell.exe Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted
    while [ $PROC_NUM -eq 0 ]; do
        PROC_NUM=$(powershell.exe -command 'if((get-process "CarlaUE4" -ea SilentlyContinue) -eq $Null){ return 0 } else { return 1 }') 
        PROC_NUM="${PROC_NUM//[!0-9]/}"  # trim weird Windows characters
        sleep 0.5 # resolution of 0.5s (slower than bash bc using ps1)
    done
else # Linux only!
    while [ $PROC_NUM -eq 0 ]; do
        PROC_NUM=$(ps -ef | grep "carla" | grep -v "grep" | wc -l)
        sleep 0.05 # resolution of 0.05s
    done
fi

echo -e "Found Carla script running... Waiting 10s to let Carla initialize"

# # get quality information if available
# if [ $(ps -ef | grep "carla") == "*--quality-level*=*Low*" ];
# then
#     CarlaQuality="Low"
# else
#     CarlaQuality="Epic"
# fi

# slight delay to let carla initialize
sleep 10

# get when started
time_start=$(date +"%T.%3N")

# run the fps monitor script
echo -e "${G}Starting carla framerate capture python client${NC}"
if [[ $system == *"Microsoft"* ]]; # Running on WSL (Windows benchmark)
then
    # assumes you have pyhon3 installed on Windows
    python.exe ../python/record_carla_fps.py -c $CARLA_DIR -i $INTERVAL -d "$out_directory" -f "$carla_data" &
else
    # python3 on linux
    python3 ../python/record_carla_fps.py -c $CARLA_DIR -i $INTERVAL -d "$out_directory" -f "$carla_data" &
fi

# wait for carla termination
echo -e "Waiting for Carla to terminate..."
if [[ $system == *"Microsoft"* ]]; # Windows only!
then
    # powershell.exe Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted
    while [ $PROC_NUM -ge 1 ]; do
        PROC_NUM=$(powershell.exe -command 'if((get-process "CarlaUE4" -ea SilentlyContinue) -eq $Null){ return 0 } else { return 1 }') 
        PROC_NUM="${PROC_NUM//[!0-9]/}" # trim non-digit characters
        sleep 0.5 # resolution of 0.5s (slower than bash bc using ps1)
    done
else # Linux only!
    while [ $PROC_NUM -ge 1 ]; do
        PROC_NUM=$(ps -ef | grep "carla" | grep -v "grep" | wc -l)
        sleep 0.1 # resolution of 0.1s
    done
fi
time_end=$(date +"%T.%3N")
echo -e "${G}Finished carla script\n${NC}"

echo -e "Carla started at: $time_start"
echo -e "Carla finished at: $time_end"

echo -e "\nCtrl+C to end the collectl process"
completed=1

# move collectl process to foreground
wait


