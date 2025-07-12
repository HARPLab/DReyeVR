import json
from pathlib import Path
from pprint import pprint

import bezier as B
import matplotlib.pyplot as plt
import numpy as np
import scipy.special
from scipy.interpolate import CubicSpline

try:
    from utils import *
except ImportError:
    from HapticSharedControl.utils import *


def calc_control_points_bezier_path(sx, sy, s_yaw, ex, ey, e_yaw, l_d, l_f, n_points=100):
    """
    Compute control points and path given start and end position.

    :param sx: (float) x-coordinate of the starting point
    :param sy: (float) y-coordinate of the starting point
    :param s_yaw: (float) yaw angle at start
    :param ex: (float) x-coordinate of the ending point
    :param ey: (float) y-coordinate of the ending point
    :param e_yaw: (float) yaw angle at the end

    :return: (numpy array, numpy array)
    """

    control_points = np.array(
        [
            [sx, sy],
            [sx - l_d * np.cos(s_yaw), sy + l_d * np.sin(s_yaw)],
            [ex + l_f * np.cos(e_yaw), ey - l_f * np.sin(e_yaw)],
            [ex, ey],
        ]
    )

    paths = calc_bezier_path(control_points, n_points=n_points)

    return paths, control_points


def calc_bezier_path(control_points, n_points=100):
    """
    Compute bezier path (trajectory) given control points.

    :param control_points: (numpy array)
    :param n_points: (int) number of points in the trajectory
    :return: (numpy array)
    """
    trajectory = []
    d_trajectory = []
    dd_trajectory = []

    for u in np.linspace(0, 1, n_points):
        point = bezier(u, control_points)

        derivatives_cp = bezier_derivatives_control_points(control_points, 2)
        dt = bezier(u, derivatives_cp[1])
        ddt = bezier(u, derivatives_cp[2])

        trajectory.append(point)
        d_trajectory.append(dt)
        dd_trajectory.append(ddt)

    return [np.array(trajectory), np.array(d_trajectory), np.array(dd_trajectory)]


def bernstein_poly(n, i, u):
    """
    Bernstein polynomials.

    :param n: (int) polynomial degree
    :param i: (int)
    :param u: (float)
    :return: (float)
    """
    return scipy.special.comb(n, i) * (u**i) * ((1 - u) ** (n - i))


def bezier(u, control_points):
    """
    Return one point on the bezier curve.

    :param u: (float) number in [0, 1]
    :param control_points: (numpy array)
    :return: (numpy array) Coordinates of the point
    """
    n = len(control_points) - 1
    return np.sum([bernstein_poly(n, i, u) * control_points[i] for i in range(n + 1)], axis=0)


def bezier_derivatives_control_points(control_points, n_derivatives):
    """
    Compute control points of the successive derivatives of a given bezier curve.

    A derivative of a bezier curve is a bezier curve.
    See https://pomax.github.io/bezierinfo/#derivatives
    for detailed explanations

    :param control_points: (numpy array)
    :param n_derivatives: (int)
    e.g., n_derivatives=2 -> compute control points for first and second derivatives
    :return: ([numpy array])
    """
    w = {0: control_points}
    for i in range(n_derivatives):
        n = len(w[i])
        w[i + 1] = np.array([(n - 1) * (w[i][j + 1] - w[i][j]) for j in range(n - 1)])
    return w


def calc_curvature(dx, dy, ddx, ddy):
    """k_max
    Compute curvature at one point given first and second derivatives.

    :param dx: (float) First derivative along x axis
    :param dy: (float)
    :param ddx: (float) Second derivative along x axis
    :param ddy: (float)
    :return: (float)
    """
    return (dx * ddy - dy * ddx) / (dx**2 + dy**2) ** (3 / 2)


def plot_arrow(x, y, yaw, length=1.0, width=0.5, fc="r", ec="k"):  # pragma: no cover
    """Plot arrow."""
    if not isinstance(x, float):
        for ix, iy, iyaw in zip(x, y, yaw):
            plot_arrow(ix, iy, iyaw)
    else:
        plt.arrow(
            x,
            y,
            length * np.cos(yaw),
            length * np.sin(yaw),
            fc=fc,
            ec=ec,
            head_width=width,
            head_length=width,
        )
        plt.plot(x, y)


def calculate_bezier_trajectory(
    start_pos,
    end_pos,
    start_yaw,
    end_yaw,
    n_points,
    turning_radius,
    max_res=10,
    show_animation=True,
):
    start_x, start_y = start_pos
    end_x, end_y = end_pos
    start_yaw = np.radians(start_yaw)
    end_yaw = np.radians(end_yaw)

    dist = np.hypot(end_x - start_x, end_y - start_y)  # distance between start and end

    trajectories = {}
    # Find the optimal l_d and l_f: assume that ld and lf in range of [dist/4, ]
    for l_d in np.linspace(dist / 5, dist, max_res):
        for l_f in np.linspace(dist / 5, dist, max_res):

            paths, control_points = calc_control_points_bezier_path(
                start_x, start_y, start_yaw, end_x, end_y, end_yaw, l_d, l_f, n_points=n_points
            )
            path, d_path, dd_path = paths
            control_points = np.asfortranarray(control_points)

            curve = B.Curve(control_points.T, degree=3)
            length = curve.length

            # Display the tangent, normal and radius of curvature at a given point
            curvatures = []
            for i, u in enumerate(np.linspace(0, 1, n_points)):
                curvatures.append(
                    calc_curvature(d_path[i][0], d_path[i][1], dd_path[i][0], dd_path[i][1])
                )

            max_curvature = max(curvatures)

            if abs(1 / max_curvature) >= turning_radius:
                trajectories[(l_d, l_f)] = length
                # print(f"l_d: {l_d}, l_f: {l_f}, length: {length}, radius: {abs(1 / max_curvature)}")

    l_f, l_d = min(trajectories, key=trajectories.get)

    paths, control_points = calc_control_points_bezier_path(
        start_x, start_y, start_yaw, end_x, end_y, end_yaw, l_d, l_f, n_points=n_points
    )
    path, d_path, dd_path = paths

    # Display the tangent, normal and radius of curvature at a given point
    radius_list = []
    tangent_list = []
    normal_list = []
    curvature_center_list = []

    for i, u in enumerate(np.linspace(0, 1, n_points)):
        x_target, y_target = bezier(u, control_points)
        derivatives_cp = bezier_derivatives_control_points(control_points, 2)
        point = bezier(u, control_points)
        dt = bezier(u, derivatives_cp[1])
        ddt = bezier(u, derivatives_cp[2])

        # Radius of curvature
        radius = 1 / calc_curvature(dt[0], dt[1], ddt[0], ddt[1])

        # Normalize derivative
        dt /= np.linalg.norm(dt, 2)
        tangent = np.array([point, point + dt])
        normal = np.array([point, point + [-dt[1], dt[0]]])
        curvature_center = point + np.array([-dt[1], dt[0]]) * radius

        radius_list.append(radius)
        tangent_list.append(tangent)
        normal_list.append(normal)
        curvature_center_list.append(curvature_center)

    circle = plt.Circle(
        tuple(curvature_center_list[-1]),
        radius_list[-1],
        color=(0, 0.8, 0.8),
        fill=False,
        linewidth=1,
    )

    assert path.T[0][0] == start_x, "path is invalid"
    assert path.T[1][0] == start_y, "path is invalid"
    assert path.T[0][-1] == end_x, "path is invalid"
    assert path.T[1][-1] == end_y, "path is invalid"

    path_params = {
        "path": path,
        "control_points": control_points,
        "start_x": start_x,
        "start_y": start_y,
        "end_x": end_x,
        "end_y": end_y,
        "start_yaw": start_yaw,
        "end_yaw": end_yaw,
        "x_target": x_target,
        "y_target": y_target,
        "dist": dist,
        "radius": radius_list,
        "tangent": tangent_list,
        "normal": normal_list,
        "curvature_center": curvature_center_list,
        "circle": circle,
    }

    if show_animation:  # pragma: no cover
        fig, ax = plt.subplots()
        ax.plot(path.T[0], path.T[1], label="Bezier Path")
        ax.plot(control_points.T[0], control_points.T[1], "--o", label="Control Points")
        ax.plot(x_target, y_target)
        ax.plot(tangent[:, 0], tangent[:, 1], label="Tangent")
        ax.plot(normal[:, 0], normal[:, 1], label="Normal")
        ax.add_artist(circle)
        plot_arrow(start_x, start_y, np.pi - start_yaw, length=0.1 * dist, width=0.05 * dist)
        plot_arrow(end_x, end_y, np.pi - end_yaw, length=0.1 * dist, width=0.05 * dist)
        ax.legend()
        ax.axis("equal")
        ax.grid(True)
        plt.show()

    return path, control_points, path_params


def compute_tangents(points):
    tangents = []
    n = len(points)

    for i in range(n):
        if i == 0:
            dx, dy = points[i + 1][0] - points[i][0], points[i + 1][1] - points[i][1]
        elif i == n - 1:
            dx, dy = points[i][0] - points[i - 1][0], points[i][1] - points[i - 1][1]
        else:
            dx, dy = points[i + 1][0] - points[i - 1][0], points[i + 1][1] - points[i - 1][1]

        scale = 1
        p1 = (points[i][0] - scale * dx, points[i][1] - scale * dy)
        p2 = (points[i][0] + scale * dx, points[i][1] + scale * dy)

        tangents.append((p1, p2))

    return tangents


def process_exist_path(path):
    """
    Process an existing path consisting of a series of points and compute
    tangent vectors at each point along the path.

    Args:
        path: A list or numpy array of [x,y] coordinates that form a path

    Returns:
        param: Dictionary containing the original path and computed tangent vectors
    """
    path = np.array(path)
    param = {
        "path": path,
        "start_x": path[0][0],
        "start_y": path[0][1],
        "end_x": path[-1][0],
        "end_y": path[-1][1],
        "tangent": compute_tangents(path),
    }

    return param


if __name__ == "__main__":
    import matplotlib.pyplot as plt

    plt.style.use("default")

    __file_path__ = Path(__file__).resolve().parent

    with open(f"{__file_path__}/wheel_setting.json", "r") as f:
        vehicle_config = json.load(f)
    vehicle = Vehicle(vehicle_config=vehicle_config)
    R = vehicle.minimum_turning_radius

    P_0 = [-1.47066772, -13.22415039]  # [x, y] in carla -> [y, x] in matplotlib
    P_d = [-0.37066772, -29.02415039]
    P_f = [-6.87066772, -21.62415039]

    yaw_0 =  (-90)
    yaw_d =  (-110)
    yaw_f =  (180)

    n_points = 10
    path1, control_points1, params1 = calculate_bezier_trajectory(
        start_pos=P_0,
        end_pos=P_d,
        start_yaw=yaw_0,
        end_yaw=yaw_d,
        n_points=n_points,
        turning_radius=R,
        show_animation=False,
    )

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
    print(path2)
    # show 2 path in same plot
    plt.figure()
    plt.plot(path1.T[0], path1.T[1], label="Bezier Path 1")
    plt.plot(control_points1.T[0], control_points1.T[1], "--o", label="Control Points 1")
    plt.plot(params1["x_target"], params1["y_target"])
    plt.plot(params1["tangent"][-1][:, 0], params1["tangent"][-1][:, 1], label="Tangent 1")
    plt.plot(params1["normal"][-1][:, 0], params1["normal"][-1][:, 1], label="Normal 1")
    plt.gca().add_artist(params1["circle"])
    plot_arrow(
        params1["start_x"],
        params1["start_y"],
        np.pi - params1["start_yaw"],
        length=0.1 * params1["dist"],
        width=0.02 * params1["dist"],
    )
    plot_arrow(
        params1["end_x"],
        params1["end_y"],
        np.pi - params1["end_yaw"],
        length=0.1 * params1["dist"],
        width=0.02 * params1["dist"],
    )

    plt.plot(path2.T[0], path2.T[1], label="Bezier Path 2")
    plt.plot(control_points2.T[0], control_points2.T[1], "--o", label="Control Points 2")
    plt.plot(params2["x_target"], params2["y_target"])
    plt.plot(params2["tangent"][-1][:, 0], params2["tangent"][-1][:, 1], label="Tangent 2")
    plt.plot(params2["normal"][-1][:, 0], params2["normal"][-1][:, 1], label="Normal 2")
    plt.gca().add_artist(params2["circle"])
    plot_arrow(
        params2["start_x"],
        params2["start_y"],
        np.pi - params2["start_yaw"],
        length=0.1 * params2["dist"],
        width=0.02 * params2["dist"],
    )
    plot_arrow(
        params2["end_x"],
        params2["end_y"],
        np.pi - params2["end_yaw"],
        length=0.1 * params2["dist"],
        width=0.02 * params2["dist"],
    )

    plt.xlabel("X (m)")
    plt.ylabel("Y (m)")
    plt.axis("square")
    plt.title("Bezier Path with Control Points")
    plt.legend()
    plt.grid(True)
    plt.show()
    plt.close()
