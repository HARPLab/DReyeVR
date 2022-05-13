#!/usr/bin/env python

import os
import subprocess
from typing import List, Optional

from utils import (
    BACKUPS_DIR,
    CARLA_PATH_FILE,
    SR_PATH_FILE,
    SUPPORTED_CARLA,
    SUPPORTED_SCENARIO_RUNNER,
    advanced_cp,
    advanced_join,
    advanced_rm,
    default_args,
    expand_correspondences_glob,
    generate_correspondences,
    get_leaf_from_path,
    print_line,
    verify_installation,
    verify_version,
)

DOC_STRING = "Script to clean original repository with backups and/or hard-clean"


def clean_replace(
    ROOT: str = None,
    corr_file: str = CARLA_PATH_FILE,
    verify_files: List[str] = [],
    git_tag: Optional[str] = None,
    verbose: Optional[bool] = True,
) -> None:
    if ROOT is None:
        return
    if not os.path.exists(BACKUPS_DIR):
        print("No backups made! Unable to clean-replace...")
        print("To make a backup, first run the installer script")
        return
    # begin clean process
    ROOT = os.path.abspath(ROOT)  # convert to absolute path
    verify_installation(ROOT, verify_files)
    if verify_version(ROOT, git_tag) == False:
        return
    print_line()
    print(f'Cleaning root "{ROOT}"')

    corr = generate_correspondences(corr_file)
    all_corr = expand_correspondences_glob(corr)

    for k in all_corr.keys():
        leafname = get_leaf_from_path(k)
        rel_dest: str = all_corr[k]
        expected_path: str = advanced_join([ROOT, rel_dest, leafname])
        # by virtue of the latest backup dir not having a timestamp suffix, this is the
        # default backup dir we use to replace files
        backup_path: str = advanced_join([BACKUPS_DIR, ROOT, rel_dest, leafname])
        if os.path.exists(expected_path):
            if os.path.exists(backup_path):
                if verbose:
                    print(f'Replacing "{expected_path}" with backup "{backup_path}"')
                # only append leaf if not a directory
                # cp -r A B should result in B/A, not B/{contents of A} or B/A/A
                if os.path.isdir(k):
                    expected_path = advanced_join([ROOT, rel_dest])
                advanced_cp(backup_path, expected_path, verbose=False)
            else:
                if verbose:
                    print(f"Removing {expected_path} as there is no backup entry")
                advanced_rm(expected_path)
        else:
            if verbose:
                print(f"{leafname} not found, nothing to clean")

    print("Success!")


def hard_clean(repo: Optional[str], force: Optional[bool] = False) -> None:
    if repo is None:
        return
    if force is False:
        warning: str = (
            "WARNING: Performing hard-clean, this irreversible action "
            "will reset all tracked CARLA files and remove untracked ones. "
            "Are you sure you want to continue? (y/n)"
        )
        proceed = input(warning)
        if proceed.lower() != "y":
            print(f"Skipping hard clean of {repo}")
            return
    # performs a deep clean through git
    # TODO: make sure this is robust
    cwd = os.getcwd()
    os.chdir(repo)
    git_status_cmd = "git status"
    status = subprocess.check_call(git_status_cmd.split(" "))
    git_reset_cmd = "git reset --hard HEAD"
    status = subprocess.check_call(git_reset_cmd.split(" "))
    git_clean_cmd = "git clean -fd"  # delete untracked files and directories
    status = subprocess.check_call(git_clean_cmd.split(" "))
    os.chdir(cwd)


if __name__ == "__main__":
    args = default_args(
        DOC_STRING,
        other_args=[
            {"name": "--verbose", "action": "store_true"},
            {"name": "--hard", "action": "store_true"},
        ],
    )

    print_line()
    clean_replace(
        args.carla,
        corr_file=CARLA_PATH_FILE,
        git_tag=SUPPORTED_CARLA,
        verify_files=["CHANGELOG.md"],
        verbose=args.verbose,
    )
    hard_clean(args.carla, force=args.hard)
    print()

    print_line()
    clean_replace(
        args.scenario_runner,
        corr_file=SR_PATH_FILE,
        git_tag=SUPPORTED_SCENARIO_RUNNER,
        verify_files=["scenario_runner.py"],
        verbose=args.verbose,
    )
    hard_clean(args.scenario_runner, force=args.hard)
    print()
    print("Done clean")
    print_line()
