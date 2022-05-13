#!/usr/bin/env python

import os

from utils import default_args

# the reason for this patch's existance is that the latest (at time of writing) SRanipal plugin
# source code has an enum "FrameworkStatus::ERROR" in Source/SRanipalEye/Public/SRanipalEye_Framework.h
# which directly conflicts with some lingering #defines from UE4's WebRTC or CEF3 code which include a
# #define ERROR 0, this unfortunately gets propogated through the build and causes syntax errors when
# trying to compile SRanipal directly. The easiest fix with least ramifications is to simply rename the
# SRanipal's FrameworkStatus::ERROR variable to something specific like FrameworkStatus::ERROR_SRANIPAL

DOC_STRING: str = "Patch SRanipal for use with DReyeVR"


PLUGIN_DIR: str = "Unreal/CarlaUE4/Plugins"

# which files need to be modified
HEADER_FILE: str = "SRanipal/Source/SRanipalEye/Public/SRanipalEye_Framework.h"
SOURCE_FILE: str = "SRanipal/Source/SRanipalEye/Private/SRanipalEye_Framework.cpp"

NEW_ERROR: str = "ERROR_SRANIPAL"  # make sure this matches with EgoSensor.cpp


def patch_file(filepath: str) -> None:
    filepath = os.path.abspath(filepath)
    if not os.path.exists(filepath):
        raise Exception(f"file {filepath} does not exist")

    # read from file to string
    with open(filepath, "r") as f:
        file_data: str = f.read()

    if NEW_ERROR in file_data:
        print(f"Patch has already been applied for {filepath}")
        return

    # perform operations on the string
    file_data = file_data.replace("ERROR", NEW_ERROR)

    # write back to file
    with open(filepath, "w") as f:
        f.write(file_data)
    print(f'Successfully patched "{filepath}"')


if __name__ == "__main__":
    args = default_args(DOC_STRING, with_sr=False)

    # first patch header file
    if args.carla is not None:
        patch_file(os.path.join(args.carla, PLUGIN_DIR, HEADER_FILE))
        patch_file(os.path.join(args.carla, PLUGIN_DIR, SOURCE_FILE))
        print()
        print("Completed SRanipal patches for DReyeVR")
    else:
        print("No Carla root provided")
