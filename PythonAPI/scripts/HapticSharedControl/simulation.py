import matplotlib.pyplot as plt
import numpy as np

from .haptic_algo import *
from .path_planning import *
from .utils import *

plt.style.use("default")


# cspell: ignore bezier arctan xlabel figsize ylabel
def simulation(
    path, param, i_points, f_points, i_yaw, speed, init_steering_angle, vehicle_config, n_steps
):
    print("============================\nSimulation running\n============================")
    current_position = i_points
    # Haptic Shared Control Initialization
    haptic_control = HapticSharedControl(
        Cs=0.5,
        Kc=0.5,
        T=2,
        tp=1,
        speed=speed,
        desired_trajectory_params=param,
        vehicle_config=vehicle_config,
    )

    # Simulation Loop
    print("Starting Simulation")

    s_angleL = init_steering_angle
    s_angleR = init_steering_angle

    trajectory = [current_position]
    torques = []
    steering_angles_deg_list = [init_steering_angle]

    yaw_angles_deg = [i_yaw]  # if change sign of swa, change sign here

    # Calculate the torque at each step
    step = 0
    distances = [np.float("inf")]
    while True:
        print("=====================================")
        print("\n>>> STEP:", step)

        torque, coef, desired_steering_angle_deg = haptic_control.calculate_torque(
            current_position=current_position,
            steering_angles_deg=[s_angleL, s_angleR],
            current_yaw_angle_deg=i_yaw,
        )

        next_position = haptic_control.rtp

        torques.append(torque)
        trajectory.append(next_position)

        steering_angles_deg_list.append(desired_steering_angle_deg)
        yaw_angles_deg.append(haptic_control.predict_yaw_angle_deg)

        s_angleL = np.clip(desired_steering_angle_deg, -47.95, 69.99)
        s_angleR = np.clip(desired_steering_angle_deg, -69.99, 47.95)

        i_yaw = haptic_control.predict_yaw_angle_deg
        current_position = next_position

        step += 1
        if n_steps is not None and step >= n_steps:
            break
        elif n_steps is None:
            # consider the point as reached if the distance is less than 0.1
            dist_t = dist(current_position, f_points)
            if dist_t > distances[-1]:
                trajectory.pop(-1)
                torques.pop(-1)
                steering_angles_deg_list.pop(-1)
                yaw_angles_deg.pop(-1)
                print("Distance increased so stop the simulation")
                break
            distances.append(dist_t)

    return path, trajectory, yaw_angles_deg


def plot_trajectory(paths, trajectories, yaw_angles_deg, recorded_path=None):
    # Extract trajectory points
    # paths.remove(None)
    # trajectories.remove(None)
    # yaw_angles_deg.remove(None)

    colors = ["red", "green", "orange"]
    # Plot the trajectory
    plt.figure(figsize=(8, 8))
    for i in range(len(paths)):
        trajectory = np.array(trajectories[i])
        x_points = trajectory[:, 0]
        y_points = trajectory[:, 1]

        plt.plot(paths[i][:, 0], paths[i][:, 1], "--", label=f"Bezier Path {i}", color="blue")
        for idx, point in enumerate(zip(paths[i][:, 0], paths[i][:, 1])):
            plt.plot(point[0], point[1], "o", color="blue")

        plt.plot(paths[i][0, 0], paths[i][0, 1], "o", label=f"Start point path {i}", color="Black")
        plt.plot(
            paths[i][-1, 0], paths[i][-1, 1], "o", label=f"End point path {i}", color="purple"
        )

        # Plot the vehicle trajectory
        plt.plot(x_points, y_points, label=f"Predicted Path {i}", color=colors[i])
        for idx, point in enumerate(zip(x_points, y_points)):
            plt.plot(point[0], point[1], "o", color=colors[i])
            plot_arrow(point[0], point[1], np.radians(yaw_angles_deg[i][idx]), 0.3, width=0.2)

    if recorded_path is not None:
        plt.plot(recorded_path[1], recorded_path[0], label="Recorded Path", color="orange", marker=".")

    plt.xlabel("X (m)")
    plt.ylabel("Y (m)")
    plt.title("Vehicle Path and Bezier Trajectory")

    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.axis("square")
    plt.show()
    plt.close()
