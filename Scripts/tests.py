#!/usr/bin/env python

import os
import shutil
import glob
from typing import Callable, List, Optional, Tuple

from utils import (
    advanced_is_dir,
    advanced_create,
    advanced_cp,
    advanced_join,
    advanced_rm,
    get_leaf_from_path,
    print_line,
    print_status,
)

DOC_STRING = "Unit tests for installation scripts"

UNIT_TEST_DIR: str = "Tests"


def clear_dir() -> None:
    # shutil.rmtree(os.getcwd())
    for f in glob.glob(os.path.join(os.getcwd(), "*")):
        if os.path.isdir(f):
            shutil.rmtree(f)
        else:
            os.remove(f)


def run_get_leaf_tests() -> bool:
    tests: List[Tuple[str, str]] = [
        (os.path.join("test1", "test2", "file.txt"), "file.txt"),
        (os.path.join("file.txt"), "file.txt"),
        (os.path.join("test1", "file.txt"), "file.txt"),
        (os.path.join("test1", "test2", "something_else"), "something_else"),
        (os.sep, os.sep),
        (os.path.join("heres", "a", "directory") + os.sep, "directory" + os.sep),
        (os.path.join("another", "dir") + os.sep, "dir" + os.sep),
    ]
    for name, expected in tests:
        if get_leaf_from_path(name) != expected:
            print(f'ERROR: expected get_leaf to return "{expected}" for "{name}"')
            return False
    return True


def run_is_dir_tests() -> bool:
    name_and_exp = [
        ("test", False),
        ("test" + os.sep, True),
        (os.sep, True),
        (os.path.join(os.sep, "dir", "file.txt"), False),
        (os.path.join(os.sep, "dir", "dir2" + os.sep), True),
        # often directories in DReyeVR are labeled with '/' sep
        (os.path.join("testing", "linuxsep/"), True),
    ]
    for name, expected in name_and_exp:
        if advanced_is_dir(name) != expected:
            print(f'ERROR: expected is_dir to return "{expected}" for "{name}"')
            return False
    return True


def create_test(filename: str, contents: str) -> bool:
    # test creation
    advanced_create(filename, contents)
    if not os.path.exists(filename):
        raise Exception(f'expected file "{filename}" missing')

    if advanced_is_dir(filename):
        if not os.path.isdir(filename):
            raise Exception(f"Expected a directory for {filename}")
    else:
        # test contents
        exp_contents: str = ""
        with open(filename, "r") as f:
            exp_contents = f.read()
        if exp_contents != contents:
            raise Exception(f'expected contents in "{filename}" incorrect!')
    return True


def run_create_tests():
    clear_dir()  # ensure the working directory is clear
    test_args: List[Tuple[str, str]] = [
        ("dummy.txt", "hello"),
        ("dummy.csv", "hello"),
        ("dummy.png", "hello"),
        ("dummy.txt", ""),
        ("dummy.csv", 5000 * "this is a long string"),
        ("dummy..", ".."),
        (os.path.join("directory1", "directory2", "file.txt"), "hello there"),
        (os.path.join("directory1", "file.txt"), "hello there"),
        ("test_dir" + os.sep, ""),
        ("test_dir" + os.sep, None),
        ("test_dir" + os.sep, "content shouldn't matter"),
        (os.path.join("test_dir2", "test_file"), "content shouldn't matter"),
    ]
    for args in test_args:
        try:
            if create_test(*args) == False:
                return False
        except Exception as e:
            print(f"ERROR: {e}")
            return False
    return True


def cp_test(filename: str, contents: str) -> bool:
    # make a dummy file, fill it, test copying
    advanced_create(filename, contents)

    def check_cp(og_file: str, new_file: str, contents: str) -> None:
        if not os.path.exists(og_file):
            raise Exception(f'original file "{og_file}" missing')

        if not os.path.exists(new_file):
            raise Exception(f'new file "{new_file}" missing')

        # check contents match
        with open(new_file, "r") as f:
            new_contents = f.read()
        if new_contents != contents:
            raise Exception(
                f"New contents ({new_contents}) don't match expected ({contents})"
            )

    # test copying to a completely new file
    new_file: str = f"new_file.txt"
    advanced_cp(filename, new_file)
    check_cp(filename, new_file, contents)

    existing_file: str = f"existing_file"
    # test copying to an existing (empty) file
    advanced_create(existing_file, "")
    advanced_cp(filename, existing_file)

    # test copying to an existing (full) file
    advanced_create(existing_file, "test in existing file")
    advanced_cp(filename, existing_file)
    check_cp(filename, existing_file, contents)

    # test copying to an empty directory
    empty_dir: str = "tmp" + os.sep
    assert advanced_is_dir(empty_dir)
    advanced_cp(filename, empty_dir)
    new_file: str = os.path.join(empty_dir, get_leaf_from_path(filename))
    check_cp(filename, new_file, contents)

    # test copying to a directory with existing file
    full_dir: str = "tmp" + os.sep
    assert advanced_is_dir(full_dir)
    existing_file_in_dir: str = os.path.join(full_dir, existing_file)
    advanced_create(existing_file_in_dir, "some other content")
    advanced_cp(filename, full_dir)  # should overwrite the existing_file_in_dir
    new_file: str = os.path.join(full_dir, get_leaf_from_path(filename))
    check_cp(filename, new_file, contents)
    advanced_cp(filename, existing_file_in_dir)  # file to file directly
    check_cp(filename, new_file, contents)

    # ensure no additional files are created
    tmp_files = ["tmp1.txt", "tmp2.txt", "tmp3.txt"]
    tmp_dir: str = "tmp_dir"
    for tmp_file in tmp_files:
        advanced_cp(filename, os.path.join(tmp_dir, tmp_file))

    dir_data = list(os.walk(tmp_dir))
    assert len(dir_data) == 1

    root_name, subdirs, files = dir_data[0]
    assert root_name == tmp_dir
    assert subdirs == []
    if sorted(files) != sorted(tmp_files):  # sort so order doesn't matter
        raise Exception(
            f"Expected files in {tmp_dir} to be {tmp_files}, instead got {files}"
        )

    return True


def run_cp_tests() -> bool:
    clear_dir()  # ensure the working directory is clear
    """test for advanced_cp"""
    test_args: List[Tuple[str, str]] = [
        ("dummy.txt", "hello"),
        ("dummy.csv", "hello"),
        ("dummy.png", "hello"),
        ("dummy.txt", ""),
        ("dummy.csv", 5000 * "this is a long string"),
        ("dummy..", ".."),
        (os.path.join("directory1", "directory2", "file.txt"), "hello there"),
        (os.path.join("directory1", "file.txt"), "hello there"),
    ]
    for args in test_args:
        try:
            if cp_test(*args) == False:
                return False
        except Exception as e:
            print(f"ERROR: {e}")
            return False
    return True


def run_join_tests() -> bool:
    clear_dir()  # ensure the working directory is clear
    tests: List[Tuple[List[str], str]] = [
        (["this", "is", "a", "path"], os.path.join("this", "is", "a", "path")),
        (
            ["this", "is", "a", "dir" + os.sep],
            os.path.join("this", "is", "a", "dir") + os.sep,
        ),
        (["one_line_path"], os.path.join("one_line_path")),
        (
            ["this", "is", "a"] + 500 * ["long"] + ["path"],
            os.path.join("this", "is", "a", *(500 * ["long"]), "path"),
        ),
        ([os.pardir, os.curdir, "one"], os.path.join(os.pardir, os.curdir, "one")),
        (
            ["relative", "path", os.sep + "absolute", "path" + os.sep],
            os.path.join("relative", "path")
            + os.sep
            + os.path.splitdrive(os.path.join("absolute", "path"))[-1]
            + os.sep,
        ),
        (
            [os.pardir, "test", os.path.abspath(os.getcwd())],  # NOTE: known dir
            os.path.join(os.pardir, "test")
            + os.path.splitdrive(os.path.abspath(os.getcwd()))[-1]
            + os.sep,
        ),
        (
            [os.path.abspath(os.getcwd()), "nonexistant_dir" + os.sep],
            os.path.join(os.path.splitdrive(os.getcwd())[-1], "nonexistant_dir")
            + os.sep,
        ),
    ]
    for args, expected in tests:
        actual: str = advanced_join(args)
        if actual != expected:
            print(f'ERROR: expected join to return "{expected}" for "{args}"')
            print(f'instead got "{actual}"')
            return False
    return True


def rm_test(filename: str) -> bool:
    # test creation
    advanced_create(filename, "some content")
    advanced_rm(filename)
    if os.path.exists(filename):
        raise Exception(f'expected file "{filename}" to be removed')
    return True


def run_rm_tests() -> bool:
    clear_dir()  # ensure the working directory is clear
    test_args: List[Tuple[str, str]] = [
        "dummy.txt",
        "dummy.csv",
        "dummy.png",
        "dummy.txt",
        "dummy.csv",
        os.path.join("directory1", "directory2", "file.txt"),
        os.path.join("directory1", "file.txt"),
        "test_dir" + os.sep,
        os.path.join("test_dir2", "test_file"),
        os.path.join("test_dir2", "test_dir" + os.sep),
    ]
    for args in test_args:
        try:
            if rm_test(args) == False:
                return False
        except Exception as e:
            print(f"ERROR: {e}")
            return False

    # ensure no lingering things in this dir
    advanced_rm(os.path.join(os.getcwd(), "*"))
    dir_data = list(os.walk(os.getcwd()))
    assert len(dir_data) == 1

    root_name, subdirs, files = dir_data[0]
    assert root_name == os.getcwd()
    if len(subdirs) > 0:
        print(
            f"ERROR: found {len(subdirs)} subdirectories remaining in {os.getcwd()}. Expected none."
        )
        return False
    if len(files) > 0:
        print(
            f"ERROR: found {len(files)} files remaining in {os.getcwd()}. Expected none."
        )
        return False

    return True


if __name__ == "__main__":
    print(DOC_STRING)
    print_line()
    tests: List[Tuple[str, Callable[[None], bool]]] = [
        ("leaf", run_get_leaf_tests),
        ("is_dir", run_is_dir_tests),
        ("create", run_create_tests),
        ("cp", run_cp_tests),
        ("join", run_join_tests),
        ("rm", run_rm_tests),
    ]

    # create the unit test dir and go to it. This will be the working directory
    og_dir: str = os.getcwd()
    os.makedirs(UNIT_TEST_DIR, exist_ok=True)
    os.chdir(UNIT_TEST_DIR)

    print(f"Summary:")

    for name, test in tests:
        print(f"{name}:\t\t[{print_status(test())}]")

    # delete the unit test dir
    os.chdir(og_dir)
    if os.path.exists(UNIT_TEST_DIR):
        shutil.rmtree(UNIT_TEST_DIR)
    print()
    print("Done check")
    print_line()
