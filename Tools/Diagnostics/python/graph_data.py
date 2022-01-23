import numpy as np
import os
import matplotlib.pyplot as plt
import matplotlib as mpl

mpl.use("Agg")

"""IMPORTANT"""
# NOTE: this script assumes it is run in DReyeVR/Diagnostics/python


def plot_versus(
    data_x,
    data_y,
    units="",
    name_x="X",
    name_y="Y",
    trim=(0, 0),
    lines=False,
    colour="r",
    dir_path="results",
):
    # trim the starts and end of data
    trim_start, trim_end = trim
    max_size = min(len(data_x), len(data_y))
    data_x = data_x[trim_start : max_size - trim_end]
    data_y = data_y[trim_start : max_size - trim_end]

    # create a figure that is 6in x 6in
    fig = plt.figure()

    # the axis limits and grid lines
    plt.grid(True)

    units_str = " (" + units + ")" if units != "" else ""
    trim_str = " [" + str(trim_start) + ", " + str(trim_end) + "]"

    # label your graph, axes, and ticks on each axis
    plt.xlabel(name_x + units_str, fontsize=16)
    plt.ylabel(name_y + units_str, fontsize=16)
    plt.xticks()
    plt.yticks()
    plt.tick_params(labelsize=15)
    if name_x == "":
        plt.title(name_y + trim_str, fontsize=18)
    else:
        plt.title(name_x + " versus " + name_y + trim_str, fontsize=18)

    # plot dots
    plt.plot(data_x, data_y, colour + "o")
    if lines:
        plt.plot(data_x, data_y, color=colour, linewidth=1)

    # complete the layout, save figure, and show the figure for you to see
    plt.tight_layout()
    # make file and save to disk
    if not os.path.exists(os.path.join(os.getcwd(), dir_path)):
        os.mkdir(dir_path)
    filename = name_x + "_vs_" + name_y + ".png" if name_x != "" else name_y + ".png"
    fig.savefig(os.path.join(dir_path, filename))
    plt.close(fig)


def plot_diff(
    subA,
    subB,
    units="",
    name_A="A",
    name_B="B",
    trim=(0, 0),
    lines=False,
    colour="r",
    dir_path="results",
):
    # trim the starts and end of data
    trim_start, trim_end = trim
    max_size = min(len(subA), len(subB))
    subA = subA[trim_start : max_size - trim_end]
    subB = subB[trim_start : max_size - trim_end]

    # create a figure that is 6in x 6in
    fig = plt.figure()

    # the axis limits and grid lines
    plt.grid(True)

    units_str = " (" + units + ")" if units != "" else ""
    trim_str = " [" + str(trim_start) + ", " + str(trim_end) + "]"

    # label your graph, axes, and ticks on each axis
    plt.xlabel("Points", fontsize=16)
    plt.ylabel("Difference" + units_str, fontsize=16)
    plt.xticks()
    plt.yticks()
    plt.tick_params(labelsize=15)
    plt.title("Difference (" + name_A + " - " + name_B + ")" + trim_str, fontsize=18)

    # generate data
    x_data = np.arange(len(subA))
    y_data = subA - subB
    plt.plot(x_data, y_data, colour + "o")
    if lines:
        plt.plot(x_data, y_data, color=colour, linewidth=1)

    # complete the layout, save figure, and show the figure for you to see
    plt.tight_layout()

    # make file and save to disk
    if not os.path.exists(os.path.join(os.getcwd(), dir_path)):
        os.mkdir(dir_path)
    filename = name_A + "_minus_" + name_B + ".png"
    fig.savefig(os.path.join(dir_path, filename))
    plt.close(fig)


def plot_alone(
    data, units="", name="Y", trim=(0, 0), lines=False, colour="r", dir_path="results"
):
    x_values = np.arange(len(data))
    plot_versus(
        x_values,
        data,
        units=units,
        name_x="",
        name_y=name,
        trim=trim,
        lines=lines,
        colour=colour,
        dir_path=dir_path,
    )


def read_from_file(datadir, name, delimiter=" "):
    filename = name + ".txt"  # TODO: arbitrary filetypes
    if not os.path.exists(os.path.join(os.getcwd(), datadir)):
        print("ERROR: could not find", datadir, "in cwd")
        os._exit(1)
    filepath = os.path.join(os.getcwd(), datadir, filename)
    with open(filepath, "r") as file:
        return np.loadtxt(file, delimiter=delimiter)


def framerates(data, name="", trim=(0, 0), dir_path="results"):
    trim_start, trim_end = trim
    diffs = np.diff(data[trim_start : len(data) - trim_end])
    avg_frametime = np.average(diffs)
    median_frametime = np.median(diffs)
    avg_framerate = 1 / avg_frametime
    median_framerate = 1 / median_frametime
    print("Average " + name + " framerate:", avg_framerate, "fps")
    print("Median " + name + " framerate:", median_framerate, "fps")
    plot_alone(diffs, name="diff_" + name, trim=trim, dir_path=dir_path)


def sr_no_carla(folder="without_carla", delimiter=" "):
    output_path = os.path.join("results", folder)
    datadir = os.path.join(os.getcwd(), "data", "no_carla")
    ue4 = read_from_file(datadir, "ue4", delimiter=delimiter)
    sranipal = read_from_file(datadir, "sranipal", delimiter=delimiter)
    frameseq = read_from_file(datadir, "frameseq", delimiter=delimiter)
    assert len(ue4) == len(sranipal) and len(sranipal) == len(frameseq)
    # plot ue4 (x) vs sranipal (y)
    plot_versus(
        ue4, sranipal, units="s", name_x="ue4", name_y="sranipal", dir_path=output_path
    )
    # plot ue4 - sranipal on the y
    plot_diff(
        ue4,
        sranipal,
        units="s",
        name_A="ue4",
        name_B="sranipal",
        trim=(5, 0),
        dir_path=output_path,
    )
    # plot framseq on the y
    plot_alone(frameseq, name="frameseq", dir_path=output_path)
    # plot ue4 (x) vs frameseq (y)
    plot_versus(
        ue4,
        frameseq,
        name_x="ue4",
        name_y="frameseq",
        trim=(5, 0),
        dir_path=output_path,
    )
    # plot framerate of frameseq over time
    frameseq_fps = np.diff(frameseq) / np.diff(ue4)
    plot_alone(frameseq_fps, name="frameseq_fps", dir_path=output_path)

    """Derivatives"""
    # plot differences between sranipal points
    framerates(sranipal, name="sranipal", trim=(0, 10), dir_path=output_path)
    # plot differences between ue4 points
    framerates(ue4, name="ue4", trim=(10, 10), dir_path=output_path)
    # plot differences between frameseq points
    framerates(frameseq, name="frameseq", trim=(10, 10), dir_path=output_path)
    """Metadata"""
    total_time = ue4[-1] - ue4[0]
    num_sr_frames = frameseq[-1] - frameseq[0]
    frameseq_framerate = num_sr_frames / total_time
    print("Total test duration (s):", total_time)
    print("Frameseq framerates:", frameseq_framerate)


def sr_with_carla(data_folders=[], output_folder="with_carla", delimiter=", "):
    output_path = os.path.join("results", output_folder)
    if not os.path.exists(os.path.join(os.getcwd(), output_path)):
        os.mkdir(output_path)
    for folder in data_folders:
        print("\nFound data folder", folder)
        specific_output_path = os.path.join(output_path, folder)
        datadir = os.path.join(os.getcwd(), "..", "data", folder)
        carla = read_from_file(datadir, "carla", delimiter=delimiter)
        sranipal = read_from_file(datadir, "sranipal", delimiter=delimiter)
        carla_stream = read_from_file(datadir, "carla_stream", delimiter=delimiter)
        frameseq = read_from_file(datadir, "frame_seq", delimiter=delimiter)
        # ensure all lengths are the same
        max_len = min(len(carla), len(sranipal), len(carla_stream), len(frameseq))
        carla = carla[:max_len]
        sranipal = sranipal[:max_len]
        carla_stream = carla_stream[:max_len]
        frameseq = frameseq[:max_len]

        # plot carla (x) vs sranipal (y)
        plot_versus(
            carla,
            sranipal,
            units="s",
            name_x="carla",
            name_y="sranipal",
            dir_path=specific_output_path,
        )
        # plot carla - sranipal on the y
        plot_diff(
            carla,
            sranipal,
            units="s",
            name_A="carla",
            name_B="sranipal",
            trim=(5, 0),
            dir_path=specific_output_path,
        )
        # plot carla stream on the y
        plot_alone(carla_stream, name="carla_stream", dir_path=specific_output_path)
        # plot carla (x) vs carla_stream (y)
        plot_versus(
            carla,
            carla_stream,
            name_x="carla",
            name_y="carla_stream",
            trim=(5, 0),
            dir_path=specific_output_path,
        )
        # plot framseq on the y
        plot_alone(frameseq, name="frame_seq", dir_path=specific_output_path)
        # plot ue4 (x) vs frameseq (y)
        plot_versus(
            carla,
            frameseq,
            name_x="carla",
            name_y="frame_seq",
            trim=(5, 0),
            dir_path=specific_output_path,
        )
        # plot framerate of frameseq over time
        frameseq_fps = np.diff(frameseq) / np.diff(carla)
        plot_alone(frameseq_fps, name="frameseq_fps", dir_path=specific_output_path)
        """Derivatives"""
        # plot differences between sranipal points
        framerates(
            sranipal, name="sranipal", trim=(0, 10), dir_path=specific_output_path
        )
        # plot differences between carla points
        framerates(carla, name="carla", trim=(10, 10), dir_path=specific_output_path)
        # plot differences between carla_stream points
        framerates(
            carla_stream,
            name="carla_stream",
            trim=(10, 10),
            dir_path=specific_output_path,
        )
        # plot differences between frameseq points
        framerates(
            frameseq, name="frame_seq", trim=(10, 10), dir_path=specific_output_path
        )
        """Metadata"""
        # plot framerates wrt carla_stream (time between tick and stream)
        plot_alone(
            1 / np.diff(carla_stream),
            name="fps_carla_stream",
            dir_path=specific_output_path,
        )
        # plot framerates wrt sranipal time (time between tick and stream)
        plot_alone(
            1 / np.diff(sranipal), name="fps_sranipal", dir_path=specific_output_path
        )
        # plot framerates wrt carla time (time between tick and stream)
        plot_alone(1 / np.diff(carla), name="fps_carla", dir_path=specific_output_path)

        total_time = carla[-1] - carla[0]  # same as sranipal time
        num_sr_frames = frameseq[-1] - frameseq[0]
        frameseq_framerate = num_sr_frames / total_time
        print("Total test duration (s):", total_time)
        print("Frameseq framerates:", frameseq_framerate)

