import json
import os
import sys
from pathlib import Path
from turtle import speed

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# sys.path.append(os.path.join(os.getenv("DReyeVR"), "PythonAPI"))
from HapticSharedControl.haptic_algo import *
from HapticSharedControl.path_planning import *
from HapticSharedControl.simulation import *
from HapticSharedControl.utils import *

plt.style.use("default")


__file_path__ = Path(__file__).resolve().parent

# Load the vehicle configuration
with open(f"{__file_path__}/HapticSharedControl/wheel_setting.json", "r") as f:
    vehicle_config = json.load(f)
vehicle = Vehicle(vehicle_config=vehicle_config)
R = vehicle.minimum_turning_radius
print("Minimum Turning Radius:", R, "m")

# define initial and final points
trial = str(input("Enter trial number 0(sample) L-R(1-6) R-L(8-12): "))
recorded_path = None
if trial == "0":
    P_0 = [-1.47066772, -13.22415039]  # [x, y] in carla -> [y, x] in matplotlib
    P_d = [-0.37066772, -29.02415039]
    P_f = [-6.87066772, -21.62415039]
    yaw_0 = -90
    yaw_d = -100
    yaw_f = 180
elif trial == "1":
    P_0 = [-2.13139057, -13.58492756]
    P_d = [-0.26648349, -27.88323593]
    P_f = [-8.6554842, -21.6907711]
    yaw_0 = - 2.3220824999999934 - 90
    yaw_d = - 29.3604774 - 90
    yaw_f = 105.7937202 + 90
elif trial == "2":
    P_0 = [-2.0546906, -15.14080429]
    P_d = [-0.16853675, -27.82400513]
    P_f = [-7.68998575, -21.31655693]
    yaw_0 = -1.9093017999999944 - 90
    yaw_d = 30.0815544 - 90
    yaw_f = 93.37516379 + 90
elif trial == "3":
    P_0 = [-2.33013225, -13.42747879]
    P_d = [-0.27527067, -27.43535042]
    P_f = [-7.19522429, -21.22905159]
    yaw_0 = -1.153442400000003 -90
    yaw_d = - 29.3213272 - 90
    yaw_f = 90.37275675 + 90
elif trial == "4":
    P_0 = [-2.59471703, -13.76090431]
    P_d = [-0.84754181, -27.41300201]
    P_f = [-7.63892126, -21.31984329]
    yaw_0 = - 1.9469986000000006 - 90
    yaw_d = - 25.734725999999995 - 90
    yaw_f = 90.29345572 + 90
elif trial == "5":
    P_0 = [-2.46604896, -12.84768295]
    P_d = [-0.76953948, -27.44445229]
    P_f = [-7.99046612, -21.05567169]
    yaw_0 = - 3.3721313000000066 - 90
    yaw_d = - 20.9653397 - 90
    yaw_f = 85.99301052 + 90
elif trial == "6":
    P_0 = [-2.68211842, -13.66109943]
    P_d = [-0.15471956, -27.84520721]
    P_f = [-7.58486843, -21.00892448]
    yaw_0 = - 1.5638045999999974 - 90
    yaw_d = - 31.523597700000003 - 90
    yaw_f = 85.90444851 + 90
# reverse direction
elif trial == "8":
    P_0 = [-2.21546435, -31.60477066]
    P_d = [1.03518212, -18.32664871]
    P_f = [-7.7015214, -21.18217278]
    yaw_0 = 90 - (178.7055664 - 180)
    yaw_d = 90 - (124.3087578 - 180)
    yaw_f = 88.27893066 + 90
elif trial == "9":
    P_0 = [-1.3271035, -34.64346695]
    P_d = [1.54469788, -19.11241913]
    P_f = [-7.67417145, -21.5524559]
    yaw_0 = 182.4389496 - 90
    yaw_d = 125.65747833 - 90
    yaw_f = 88.90112317 + 90
elif trial == "10":
    P_0 = [-2.41052723, -31.86808777]
    P_d = [2.61411977, -16.33299065]
    P_f = [-7.40135479, -21.47801208]
    yaw_0 = 174.07925419999998 - 90
    yaw_d = 125.3021584 - 90
    yaw_f = 87.58251953 + 90
elif trial == "11":
    P_0 = [-1.99927211, -29.94470215]
    P_d = [2.38471651, -17.51079941]
    P_f = [-7.10684538, -21.3453064]
    yaw_0 = 181.86206049999998 - 90
    yaw_d = 121.4338913 - 90
    yaw_f = 88.89004517 + 90
elif trial == "12":
    P_0 = [-3.46834612, -30.75714493]
    P_d = [-0.9832288, -17.28814125]
    P_f = [-7.75748634, -21.73072243]
    yaw_0 = 177.7886658 - 90
    yaw_d = 147.1214333 - 90
    yaw_f = 89.86245729 + 90
else:
    print("Invalid trial number")
    exit()

if trial != "0":
    df = pd.read_excel(f"{__file_path__.parent}/data/trials/trial{trial}.xlsx")
    x = df["LocationX"].to_list()
    y = df["LocationY"].to_list()
    yaw = df["RotationYaw"].to_list()
    recorded_path = np.array([y, x])
else:
    recorded_path = None

# Simulation Parameters
# calculate the bezier path

n_points = 50

n_steps = None
speed = 1.0  # m/s
init_steering_angle = 10  # abs value

if (P_0 != [] and yaw_0 is not None) and (P_d != [] and yaw_d is not None):
    path1, control_points1, params1 = calculate_bezier_trajectory(
        start_pos=P_0,
        end_pos=P_d,
        start_yaw=yaw_0,
        end_yaw=yaw_d,
        n_points=n_points,
        turning_radius=R,
        show_animation=False,
    )
    # Run the simulation
    path1, trajectory1, yaw_angles_deg1 = simulation(
        path=path1,
        param=params1,
        i_points=P_0,
        f_points=P_d,
        i_yaw=-(180 + yaw_0),
        speed=1 * speed,
        init_steering_angle=1 * init_steering_angle,
        vehicle_config=vehicle_config,
        n_steps=n_steps,
    )
else:
    path1 = None
    trajectory1 = None
    yaw_angles_deg1 = None

print("=====================================")

if (P_d != [] and yaw_d is not None) and (P_f != [] and yaw_f is not None):
    # backward so reverse the yaw angle (+180)
    path2, control_points2, params2 = calculate_bezier_trajectory(
        start_pos=P_d,
        end_pos=P_f,
        start_yaw=180 + yaw_d,
        end_yaw=180 + yaw_f,
        n_points=n_points,
        turning_radius=R,
        show_animation=False,
    )

    # Run the simulation
    path2, trajectory2, yaw_angles_deg2 = simulation(
        path=path2,
        param=params2,
        i_points=P_d,
        f_points=P_f,
        i_yaw=-(yaw_d + 180), #  rotate and flip
        speed=-1 * speed,
        init_steering_angle=-1 * init_steering_angle,
        vehicle_config=vehicle_config,
        n_steps=n_steps,
    )
else:
    path2 = None
    trajectory2 = None
    yaw_angles_deg2 = None

# # Plot the trajectories
plot_trajectory(
    paths=[path1, path2],
    trajectories=[trajectory1, trajectory2],
    yaw_angles_deg=[yaw_angles_deg1, yaw_angles_deg2],
    recorded_path=recorded_path,
)
#
with open("../data/paths/driving_path_left2right.txt", "r") as f:
    data_left2right = f.readlines()
    data_left2right = [line.strip().split(",") for line in data_left2right]
    data_left2right = [[float(val) for val in line[::-1]] for line in data_left2right]
    data_left2right = np.array(data_left2right)

with open("../data/paths/driving_path_right2left.txt", "r") as f:
    data_right2left = f.readlines()
    data_right2left = [line.strip().split(",") for line in data_right2left]
    data_right2left = [[float(val) for val in line[::-1]] for line in data_right2left]
    data_right2left = np.array(data_right2left)

with open("../data/paths/hitachi.txt", "r") as f:
    data_hitachi = f.readlines()
    data_hitachi = [line.strip().split(",") for line in data_hitachi]
    data_hitachi = [[float(val) for val in line[::-1]] for line in data_hitachi]
    data_hitachi = np.array(data_hitachi)


predefined_path = {
    "0": {
        "P_0": [-1.47066772, -13.22415039],
        "P_d": [-0.37066772, -29.02415039],
        "P_f": [-6.87066772, -21.62415039],
        "yaw_0": 90 - (-90),
        "yaw_d": 90 - (-80),
        "yaw_f": 90 - 0,
        "forward paths": None,
        "backward paths": None,
    },
    "1": {
        "P_0": [-2.376371583333333, -13.749357381666668],
        "P_d": [-0.566469992644628, -27.297582133636364],
        "P_f": [-7.7486732, -21.271947543333333],
        "yaw_0": 90 - (88.97628786666667),
        "yaw_d": 90 - (62.168829599999995),
        "yaw_f": 90 - (-1.9554259149999922),
        "forward paths": process_exist_path(data_left2right[:40]),
        "backward paths": process_exist_path(data_left2right[40:]),
    },
    "2": {
        "P_0": [-2.284308814, -31.734065629999996],
        "P_d": [0.409550333177305, -18.459612079588652],
        "P_f": [-7.449310874, -21.459648512],
        "yaw_0": 90 - (-88.9748993),
        "yaw_d": 90 - (-38.76474382600003),
        "yaw_f": 90 - (1.296984835999993),
        "forward paths": process_exist_path(data_right2left[:46]),
        "backward paths": process_exist_path(data_right2left[46:]),
    },
    "3": {
        "P_0": [],
        "P_d": [1.035116, -18.325821],
        "P_f": [-7.685114, -21.214608],
        "yaw_0": None,
        "yaw_d": 90 - (-50),
        "yaw_f": 90 - (0),
        "forward paths": [],
        "backward paths": process_exist_path(data_hitachi),
    },
}
# idx = "1"
# path, trajectory, yaw_angles_deg = simulation(
#     path=predefined_path[idx]["backward paths"]["path"],
#     param=predefined_path[idx]["backward paths"],
#     i_points=predefined_path[idx]["P_d"][::-1],
#     f_points=predefined_path[idx]["P_f"][::-1],
#     i_yaw=180 - predefined_path[idx]["yaw_d"],
#     speed=-1 * speed,
#     init_steering_angle=-1 * init_steering_angle,
#     vehicle_config=vehicle_config,
#     n_steps=n_steps,
# )
# plot_trajectory(
#     paths=[path],
#     trajectories=[trajectory],
#     yaw_angles_deg=[yaw_angles_deg],
#     recorded_path=None,
# )
