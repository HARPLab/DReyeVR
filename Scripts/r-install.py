#!/usr/bin/env python

import glob
import os
from typing import List, Optional

from utils import (
    CARLA_PATH_FILE,
    SR_PATH_FILE,
    SUPPORTED_CARLA,
    SUPPORTED_SCENARIO_RUNNER,
    advanced_cp,
    advanced_join,
    default_args,
    expand_correspondences_glob,
    generate_correspondences,
    get_leaf_from_path,
    print_line,
    update_backups,
    verify_installation,
    verify_version,
)

DOC_STRING = "Simple script to update the DReyeVR repository (reverse-install) with Carla changes"


def r_install(src: str, dest: str, ROOT: str, verbose: Optional[bool] = False) -> None:
    # copy from dest into src (reverse install)
    leafname = get_leaf_from_path(src)
    if not os.path.exists(src):
        raise Exception(f"Unable to locate src: \"{src}\"")
    dest_is_root: bool = dest == os.path.dirname(dest)
    if (not os.path.exists(dest) or dest_is_root) and ROOT not in dest:
        dest: str = advanced_join([ROOT, dest, leafname])
    if os.path.exists(dest):
        if verbose:
            print(f"{dest} -- found")
        if os.path.isdir(src):
            # if src is a directory, we only want to r-install files it contains
            # not the entire directory (which could contain things we don't need)
            all_DReyeVR_files: List[str] = glob.glob(os.path.join(src, "*"))
            all_CARLA_files: List[str] = glob.glob(os.path.join(dest, "*"))
            for CARLA_file in all_CARLA_files:
                # check if DReyeVR_file is in a substring of all_files
                for DReyeVR_file in all_DReyeVR_files:
                    if DReyeVR_file in CARLA_file:
                        r_install(DReyeVR_file, CARLA_file, ROOT, verbose=verbose)
                        break # shouldn't be any more files that satisfy this
            return
        elif os.path.isdir(dest):
            dest = os.path.join(dest, "*")
        advanced_cp(dest, src, verbose=verbose)
    else:
        print(f"{dest} -- not found")


def update_DReyeVR_repo(
    ROOT: str = None,
    corr_file: str = CARLA_PATH_FILE,
    verify_files: List[str] = [],
    git_tag: Optional[str] = None,
    verbose: Optional[bool] = True,
) -> None:
    if ROOT is None:
        return
    # begin r-install process
    ROOT = os.path.abspath(ROOT)  # convert to absolute path
    verify_installation(ROOT, verify_files)
    if verify_version(ROOT, git_tag) == False:
        return
    print_line()
    print(f"Reverse-installing from ({ROOT})")
    update_backups(os.path.abspath(os.getcwd()))

    corr = generate_correspondences(corr_file)
    all_corr = expand_correspondences_glob(corr)

    for src in all_corr.keys():
        r_install(src, all_corr[src], ROOT, verbose)

    print("Success!")


if __name__ == "__main__":
    args = default_args(DOC_STRING)

    print(DOC_STRING)
    print_line()
    update_DReyeVR_repo(
        args.carla,
        corr_file=CARLA_PATH_FILE,
        git_tag=SUPPORTED_CARLA,
        verify_files=["CHANGELOG.md"],
        verbose=True,
    )
    print()
    update_DReyeVR_repo(
        args.scenario_runner,
        corr_file=SR_PATH_FILE,
        git_tag=SUPPORTED_SCENARIO_RUNNER,
        verify_files=["scenario_runner.py"],
        verbose=True,
    )

    print()
    print("Done reverse-install")
    print_line()
