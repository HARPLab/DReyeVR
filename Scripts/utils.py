import argparse
import csv
import glob
import os
import shutil
import subprocess
from datetime import datetime
from typing import Any, Dict, List, Optional, Tuple

# current version of CARLA supported (git tag)
SUPPORTED_CARLA: str = "0.9.13"
SUPPORTED_SCENARIO_RUNNER: str = "v0.9.13"

# backup directory for overwritten files
BACKUPS_DIR: str = "Backups"
os.makedirs(BACKUPS_DIR, exist_ok=True)

CARLA_PATH_FILE: str = os.path.join("Scripts", "Paths", "DReyeVR.csv")
SR_PATH_FILE: str = os.path.join("Scripts", "Paths", "ScenarioRunner.csv")


def print_line() -> None:
    print("*" * 50)


def print_subtitle(title: str):
    print()
    print("*" * 25 + title + "*" * 25)


def print_cp(src: str, dest: str, verbose: Optional[bool] = True) -> None:
    if verbose:
        print(f"{src} -> {dest}")


def print_status(b: bool) -> str:
    return "OK" if b else "FAILED"


def verify_installation(root_dir: str, check_files: List[str] = []) -> None:
    assert os.path.exists(root_dir)
    # check that all these files are in root_dir
    for f in check_files:
        if not os.path.exists(os.path.join(root_dir, f)):
            raise Exception(f'Expected file "{f}" not present in "{root_dir}"')
    print(f"Verified installation ({root_dir})")


def get_leaf_from_path(path: str) -> str:
    if advanced_is_dir(path):
        # include the path separator to ensure it is still recognized as a directory
        return os.path.basename(os.path.normpath(path)) + os.sep
    return os.path.basename(path)


def get_git_tag(root_dir: str) -> str:
    # TODO: make this more robust
    current = os.getcwd()
    os.chdir(root_dir)
    # this command works well on vanilla repos
    git_tag_cmd: str = "git describe --tags --exact-match"
    # this command works well when on repo forks
    git_tag_cmd_fork: str = "git symbolic-ref -q --short HEAD"
    try:
        detected: bytes = subprocess.check_output(git_tag_cmd.split(" "))
    except Exception as e:
        try:
            print("...Attempting backup git tag cmd...")
            detected_backup: bytes = subprocess.check_output(
                git_tag_cmd_fork.split(" ")
            )
            detected = detected_backup
        except Exception as e2:
            print(f'First try: {type(e).__name__}: "{e}"')
            print(f'Second try: {type(e2).__name__}: "{e2}"')
            detected: bytes = b"Unknown"
    os.chdir(current)
    return detected.decode("ascii").strip()


def verify_version(root_dir: str, expected: str) -> bool:
    assert os.path.exists(root_dir)
    detected = get_git_tag(root_dir)
    if detected != expected:
        print(f"Only {expected} version is supported! Detected version: {detected}")
        progress = input("Do you want to continue anyways? (y/n) ")
        print()
        if progress.lower() == "y":
            print(f"Proceeding on {root_dir} ({detected})")
            return True
        print(f"Skipping install")
        return False

    print(f"Verified version ({detected})")
    return True


def update_backups(root_type: str) -> None:
    # keep a backup of the backups (rename latest root to timestamp)
    old_name: str = advanced_join([BACKUPS_DIR, root_type])
    if os.path.exists(old_name):
        timestamp: str = datetime.now().strftime("%m.%d.%Y_%H.%M.%S")
        backup_path: str = os.path.normpath(advanced_join([BACKUPS_DIR, root_type]))
        new_name: str = f"{backup_path}.{timestamp}"
        os.rename(old_name, new_name)
    # new backups (latest)
    os.makedirs(old_name, exist_ok=True)


def copy_file(src: str, dest: str, verbose: Optional[bool] = False) -> None:
    if os.path.exists(dest):  # overwriting an existing file
        if os.path.isfile(dest):
            final_file: str = dest  #  final output
        else:
            final_file: str = os.path.join(
                dest, get_leaf_from_path(src)
            )  #  final output
    else:
        advanced_create(dest)
        assert os.path.exists(dest)
        copy_file(src, dest, verbose)  # now the dest should exist
        return

    assert os.path.exists(dest)
    if os.path.exists(final_file):
        backup_path = advanced_join([BACKUPS_DIR, dest])
        if os.path.isdir(dest):
            # create the backup directory if it doesn't exist
            os.makedirs(backup_path, exist_ok=True)
        else:
            # create the directory to the destination FILE if it doesn't exist
            path_to_backup_file: str = os.path.split(backup_path)[0]
            try:
                os.makedirs(path_to_backup_file, exist_ok=True)
            except NotADirectoryError:
                # for some reason Windows requires that making directories
                # must include the path separator at the end else its a "file"
                # and raises NotADirectoryErr, so I must append os.sep    >:(
                os.makedirs(path_to_backup_file + os.sep, exist_ok=True)
        shutil.copy(final_file, backup_path)
    shutil.copy(src, dest)
    print_cp(src, final_file, verbose)
    # ensure the actual files/dirs exist!
    assert os.path.exists(src)
    assert os.path.exists(final_file)
    if advanced_is_dir(dest):
        assert get_leaf_from_path(src) == get_leaf_from_path(final_file)


def copy_dir(src: str, dest: str, verbose: Optional[bool] = True) -> None:
    raise NotImplementedError("Not using copy_dir, instead copying files recursively")
    import distutils.dir_util

    print_cp(src, dest)
    # shutil.copytree(src, dest) # raises exception if dest exists!
    if os.path.exists(dest):
        backup_path = advanced_join([BACKUPS_DIR, dest])
        distutils.dir_util.copy_tree(dest, backup_path)  # always make a backup
    distutils.dir_util.copy_tree(src, dest)  # overwrites instead of exception!
    for f in os.listdir(src):
        print_cp(os.path.join(src, f), os.path.join(dest, f), verbose)


def make_platform_indep(src: str, dest: str) -> Tuple[str, str]:
    # convert the src/dest to platform-independent paths (not just '/')
    if False:
        src: str = os.path.join(*src.split("/"))
        if os.path.isabs(dest):  # absolute path
            dest: str = os.path.join("/", *dest.split("/"))
        else:
            dest: str = os.path.join(*dest.split("/"))
    # TODO: check that this works on Windows, MacOS, Linux
    _src = os.path.abspath(src)
    _dest = os.path.abspath(dest)
    if advanced_is_dir(src):
        _src += os.sep
    if advanced_is_dir(dest):
        _dest += os.sep
    return _src, _dest


def advanced_is_dir(name: str) -> bool:
    # assuming something like "A" is a file, "A/" is a directory
    # how to decide what kind of file (file, dir) dest is?
    if os.path.exists(name) and os.path.isdir(name):
        # easy case, this alr exists
        return True
    # if not, we'll need to just analyze the string itself
    # check if the last character is the os separator
    # https://docs.python.org/3/library/os.html#os.sep
    return name[-1] == os.sep or name[-1] == "/"  # check OS sep or linux sep


def advanced_create(
    name: str, contents: str = "", hard_replace: Optional[bool] = False
) -> None:
    # create a file or directory (if advanced_is_dir(name) == True) with requested content
    # hard_replace always deletes existing files in favour of new one
    norm_name: str = os.path.normpath(name)
    if os.path.exists(norm_name) and (
        advanced_is_dir(norm_name) != advanced_is_dir(norm_name)
    ):
        # if an existing file has been found that conflicts
        if hard_replace:  # always replace
            advanced_rm(os.path.normpath(name))
        else:
            remove = input(f"Found existing {name}, should delete? (y/n) ")
            if remove.lower() == "y":
                os.remove(os.path.normpath(name))
            else:
                # path already exists
                return
    if advanced_is_dir(name):
        # already exists a file with this name
        os.makedirs(name, exist_ok=True)
    else:
        path, filename = os.path.split(name)
        if len(path) > 0:
            os.makedirs(path, exist_ok=True)
        open(name, "a").close()
        with open(name, "w") as f:
            f.write(contents)

    assert os.path.exists(name)


def advanced_cp(src: str, dest: str, verbose: Optional[bool] = False) -> None:
    leaf = get_leaf_from_path(src)
    src, dest = make_platform_indep(src, dest)
    if os.path.isfile(src):
        copy_file(src, dest, verbose=verbose)
    elif advanced_is_dir(src):
        if os.path.isfile(dest):
            raise Exception("Cannot copy a directory to a file")
        # copy_dir(src, os.path.join(dest, leaf), verbose=verbose)
        # recursively copy files as a * glob
        advanced_cp(os.path.join(src, "*"), advanced_join([dest, leaf]), verbose)
    else:
        try:
            files = glob.glob(src)
            for f in files:
                if advanced_is_dir(f):
                    f += os.sep
                advanced_cp(f, dest, verbose=verbose)
        except Exception as e:
            print(f"Unknown filetype: {src}")
            raise e
        # raise NotImplementedError


def advanced_rm(path: str) -> None:
    if os.path.isfile(path):
        os.remove(path)
    elif os.path.isdir(path):
        shutil.rmtree(path)
    else:
        try:
            files = glob.glob(path)
            for f in files:
                advanced_rm(f)
        except Exception as e:
            print(f"Unknown filetype: {path}")
            raise e
        # raise NotImplementedError


def advanced_join(paths: List[str]) -> str:
    # effectively same as os.path.join but does not truncate
    # if an entry in path is an absolute path
    assert len(paths) > 0
    path_names: List[str] = []
    for p in paths:
        # since these were initially written for linux which uses
        # forward slash ('/') as the path separator
        path_without_sep: List[str] = p.split("/")
        path_sep: str = os.path.join(*path_without_sep)
        drive, path_sep = os.path.splitdrive(path_sep)
        # only include path with separators, strip first sep to not truncate
        path_names.append(path_sep.strip(os.sep))
    if os.path.isabs(paths[0]):  # absolute path
        path_names = [os.sep] + path_names  # add absolute path marker
    joined_str: str = os.path.join(*path_names)
    if advanced_is_dir(paths[-1]):
        joined_str += os.sep  # keep the fact that this is a directory
    return joined_str


def generate_correspondences(csv_file: str) -> Dict[str, str]:
    assert os.path.exists(csv_file)
    corr: Dict[str, str] = {}
    with open(csv_file, "r") as f:
        for row in csv.reader(f, delimiter=","):
            assert len(row) == 2
            DReyeVR_path, CARLA_path = row
            # print(" --> ".join(row))
            corr[DReyeVR_path] = CARLA_path
    return corr


def expand_correspondences_glob(corr: Dict[str, str]) -> Dict[str, str]:
    # expands regex in LHS (DReyeVR paths) to absolute files
    # NOTE: this only supports expanding globs on the LHS (DReyeVR paths)
    expanded = {}
    for k in corr.keys():
        for expanded_k in glob.glob(k):
            expanded[expanded_k] = corr[k]
    return expanded


def get_all_files(corr: Dict[str, str]) -> Dict[str, str]:
    files_map = {}
    for k in corr.keys():
        if advanced_is_dir(k):
            only_files = []
            for root, _, files in os.walk(k):
                for f in files:
                    filepath = advanced_join([root, f])
                    if os.path.isfile(filepath):
                        only_files.append(filepath)
                        assert os.path.exists(filepath)

            files_map[k] = list(sorted(only_files))
        else:
            files_map[k] = [k]  # already a file
    for group in files_map.values():
        for f in group:
            assert not advanced_is_dir(f)
    return files_map


def check_env_var(user_in: str, env_var: str) -> Optional[str]:
    if user_in is not None:
        # just use the path provided to us
        return user_in

    if env_var not in os.environ or len(os.environ[env_var]) == 0:
        print(f"Unable to find installation or user input for {env_var}")
        return None

    alternatie_path = os.environ[env_var]
    print(
        f'No user input, defaulting to environment variable: "{env_var}" == "{alternatie_path}"'
    )
    use_alt_path = input(f"Should proceed on {alternatie_path}? (y/n) ")
    if use_alt_path.lower() == "y":
        return alternatie_path
    return None


def default_args(
    DOC_STRING: str = "", with_sr: bool = True, other_args: List[Tuple[Any]] = []
) -> dict:
    print(DOC_STRING)
    print()
    argparser = argparse.ArgumentParser(description=DOC_STRING)
    argparser.add_argument(
        "--carla",
        metavar="C",
        default=None,
        type=str,
        nargs="?",  # 0 or 1 argument
        help="Path for Carla root (can be relative or absolute)",
    )
    argparser.add_argument(
        "--scenario-runner",
        metavar="SR",
        default=None,
        type=str,
        nargs="?",  # 0 or 1 argument
        help="Path for Scenario Runner root (can be relative or absolute)",
    )
    argparser.add_argument(
        "--env",
        action="store_true",
        help="Whether or not to check the environment variables",
    )
    for args in other_args:
        name: str = args.pop("name")
        argparser.add_argument(name, **args)
    args = argparser.parse_args()
    if args.env or (args.carla is None and args.scenario_runner is None):
        args.carla = check_env_var(args.carla, "CARLA_ROOT")
        print()
        if with_sr:
            args.scenario_runner = check_env_var(
                args.scenario_runner, "SCENARIO_RUNNER_ROOT"
            )

    return args
