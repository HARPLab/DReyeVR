#!/bin/bash

# the reason for this patch's existance is that the latest (at time of writing) SRanipal plugin 
# source code has an enum "FrameworkStatus::ERROR" in Source/SRanipalEye/Public/SRanipalEye_Framework.h
# which directly conflicts with some lingering #defines from UE4's WebRTC or CEF3 code which include a
# #define ERROR 0, this unfortunately gets propogated through the build and causes syntax errors when
# trying to compile SRanipal directly. The easiest fix with least ramifications is to simply rename the
# SRanipal's FrameworkStatus::ERROR variable to something specific like FrameworkStatus::ERROR_SRANIPAL

set -e

source $(dirname "$0")/utils.sh

CARLA_ROOT=$(update_root $CARLA_ROOT $1) # see if we need to override the carla root

cd $CARLA_ROOT

# where the plugin should be installed
PLUGIN_DIR="Unreal/CarlaUE4/Plugins"
PLUGIN_DIR=$(to_abs_path $PLUGIN_DIR) # convert to absolute path

# which files need to be modified
HEADER_FILE="SRanipal/Source/SRanipalEye/Public/SRanipalEye_Framework.h"
SOURCE_FILE="SRanipal/Source/SRanipalEye/Private/SRanipalEye_Framework.cpp"

verify_installation $PLUGIN_DIR $HEADER_FILE "SRanipal"

NEW_ERROR="ERROR_SRANIPAL" # make sure this matches with EgoSensor.cpp

if grep -q $NEW_ERROR "$PLUGIN_DIR/$HEADER_FILE"; then # check if already been applied
    echo -e "${Y}Patch has already been applied!${NC}"
    exit 0
fi

sed -i "s/ERROR/${NEW_ERROR}/g" $PLUGIN_DIR/$HEADER_FILE
sed -i "s/ERROR/${NEW_ERROR}/g" $PLUGIN_DIR/$SOURCE_FILE

echo -e "${G}Completed SRanipal patching${NC}"

# back to original working directory
cd $OLDPWD