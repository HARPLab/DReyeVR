import os, sys, glob
from typing import List

CARLA_ROOT: str = os.getenv("CARLA_ROOT")
if CARLA_ROOT is None:
    raise Exception("Unable to find CARLA_ROOT in environment variable!")

"""Add Carla PythonAPI .egg file"""
egg_dir: str = os.path.join(CARLA_ROOT, "PythonAPI", "carla", "dist")
carla_eggs: List[str] = glob.glob(os.path.join(egg_dir, f"carla-*.egg"))
for egg in carla_eggs:
    sys.path.append(egg)
if len(carla_eggs) == 0:
    print(f'No PythonAPI .egg file in "{egg_dir}"')
    print("Make sure you built PythonAPI and an .egg was produced!")

"""Add all Carla paths required by module"""
# see https://carla-scenariorunner.readthedocs.io/en/latest/getting_scenariorunner/#update-from-source
sys.path.extend(
    [
        os.path.join(CARLA_ROOT, "PythonAPI", "carla", "agents"),
        os.path.join(CARLA_ROOT, "PythonAPI", "carla"),
        os.path.join(CARLA_ROOT, "PythonAPI", "examples"),
        os.path.join(CARLA_ROOT, "PythonAPI"),
    ]
)

print("Carla import successful!")
