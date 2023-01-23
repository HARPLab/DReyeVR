#!/usr/bin/env python

import os
from typing import Optional

from utils import (
    BACKUPS_DIR,
    CARLA_PATH_FILE,
    SR_PATH_FILE,
    SUPPORTED_CARLA,
    SUPPORTED_SCENARIO_RUNNER,
    advanced_cp,
    advanced_join,
    advanced_is_dir,
    get_leaf_from_path,
    get_all_files,
    default_args,
    generate_correspondences,
    expand_correspondences_glob,
    print_line,
    update_backups,
    verify_installation,
    verify_version,
)

DOC_STRING: str = "Install DReyeVR atop a working Carla and/or ScenarioRunner directory"


def install_over(
    ROOT: Optional[str],
    correspondences_file: str,
    git_tag: str,
    check_files: Optional[str] = None,
) -> None:
    if ROOT is None:
        return
    # begin installation process
    ROOT_ABS = os.path.abspath(ROOT)  # convert to absolute path
    # verify that the root exists and files in "check_files" exist
    verify_installation(ROOT_ABS, check_files)
    # verify the git version of this repository matches
    if verify_version(ROOT_ABS, git_tag) == False:
        return
    print_line()
    print(f"Installing DReyeVR over {ROOT_ABS}")
    update_backups(ROOT_ABS)
    # generate the DReyeVR -> root path correspondences
    corr = generate_correspondences(correspondences_file)
    all_corr = expand_correspondences_glob(corr)
    all_files_within = get_all_files(all_corr)
    for DReyeVR_path, CARLA_path in all_corr.items():
        if advanced_is_dir(DReyeVR_path):
            # installing a directory (many files within)
            for local_file in all_files_within[DReyeVR_path]:
                dest_path: str = advanced_join([ROOT, CARLA_path, local_file])
                advanced_cp(local_file, dest_path, verbose=True)
        else:
            # installing an individual file
            assert os.path.isfile(DReyeVR_path)
            file_leafname: str = get_leaf_from_path(DReyeVR_path)
            dest_path: str = advanced_join([ROOT, CARLA_path, file_leafname])
            advanced_cp(DReyeVR_path, dest_path, verbose=True)
    print()
    print(f"Installation success!")
    print(f"Backups created in {advanced_join([BACKUPS_DIR, ROOT_ABS])}")


if __name__ == "__main__":
    args = default_args(DOC_STRING)

    print_line()
    install_over(
        ROOT=args.carla,
        correspondences_file=CARLA_PATH_FILE,
        git_tag=SUPPORTED_CARLA,
        check_files=["CHANGELOG.md"],
    )
    print()
    install_over(
        ROOT=args.scenario_runner,
        correspondences_file=SR_PATH_FILE,
        git_tag=SUPPORTED_SCENARIO_RUNNER,
        check_files=["scenario_runner.py"],
    )
    print()
    print("Done installation")
    print_line()
