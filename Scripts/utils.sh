#!/bin/bash

R='\033[0;31m'         # Red
G='\033[0;32m'         # Green
Y='\033[0;33m'         # Yellow
B='\033[0;34m'         # Blue
NC='\033[0m'           # No Color

wd=$(pwd)

print_usage() {
    echo -e "Usage: Enter the location for Carla then ScenarioRunner"
    echo -e "Ex: install.sh PATH/TO/CARLA PATH/TO/SCENARIORUNNER"
    echo
}

verify_root() {
    NAME=$1
    ROOT=$2
    if [ -z "${ROOT}" ]; then
        echo -e "${R}ERROR: No ${NAME} root path provided${NC}"
        exit 1
    fi
}

verify_installation() {
    ROOT=$1
    FILE=$2
    NAME=$3
    if [ ! -f ${ROOT}/${FILE} ]
    then
        echo -e "${R}Could not verify a correct ${NAME} installation at ${ROOT}${NC}"
        exit 1
    else
        echo -e "${G}Found a ${NAME} installation at ${ROOT}${NC}"
    fi
}

verify_version() {
    ROOT=$1
    SUPPORTED=$2
    NAME=$3
    INSTALL_FN=$4
    # getting version from branch/tag from: https://stackoverflow.com/questions/18659425/get-git-current-branch-tag-name
    version=$(cd ${ROOT} && git symbolic-ref -q --short HEAD || git describe --tags --exact-match && cd ${wd})
    if [ "${version}" == "${SUPPORTED}" ] ; then
        echo -e "${G}Verified compatible ${NAME} version${NC}"
        $4 # run the install script
    else
        echo -e "${R}Only ${NAME} version ${SUPPORTED} is supported! Detected version: ${version}${NC}"
        read -p "$(echo -e ${Y}"Do you want to continue anyways? (y/n) "${NC})" -n 1 -r CONFIRM
        echo # move cursor to a new line
        if [[ ${CONFIRM} =~ ^[Yy]$ ]]; then
            echo -e "${B}Attempting to install to ${ROOT} (${version})${NC}"
            $4 # run the install script
        else
            echo -e "${B}Skipping install${NC}"
        fi
    fi
}