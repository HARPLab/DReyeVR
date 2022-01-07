import argparse
import numpy as np
import os
import matplotlib.pyplot as plt
import matplotlib as mpl
import pandas
mpl.use('Agg')

"""IMPORTANT"""
# NOTE: this script assumes it is run in DReyeVR/Diagnostics/
assert("Diagnostics" in os.getcwd())


def plot_many_versus(data_x, data_ys, units="", name_x="X", name_y="Y",
                     trim=(0, 0), points=True, lines=False,
                     colours=["r", "g", "b", "c", "m", "y", "k"], xbounds=None,
                     ybounds=None, dir_path="results", t_carla=(None, None)):
    # trim the starts and end of data
    trim_start, trim_end = trim

    max_len = min(len(data_x), len(data_ys[0]))
    data_x = data_x[trim_start:max_len - trim_end]
    for data_y in data_ys:
        data_y = data_y[trim_start:max_len - trim_end]

    # create a figure that is 6in x 6in
    fig = plt.figure()

    # the axis limits and grid lines
    plt.grid(True)

    units_str = " (" + units + ")" if units != "" else ""
    trim_str = " [" + str(trim_start) + ", " + str(trim_end) + "]"

    # label your graph, axes, and ticks on each axis
    plt.xlabel(name_x + units_str, fontsize=16)
    plt.ylabel(name_y + units_str, fontsize=16)
    if ybounds:
        plt.ylim(ybounds)
    if xbounds:
        plt.xlim(xbounds)
    plt.xticks()
    plt.yticks()
    plt.tick_params(labelsize=15)
    if(name_x == ""):
        plt.title(name_y + trim_str, fontsize=18)
    else:
        plt.title(name_x + " versus " + name_y + trim_str, fontsize=18)

    # plot data
    for i in range(len(data_ys)):
        data_y = data_ys[i]
        colour = colours[i % len(colours)]
        if points:
            plt.plot(data_x, data_y, colour + "o")
        if lines:
            plt.plot(data_x, data_y, color=colour, linewidth=1)

    # add lines for carla starting/ending
    t_carla_start, t_carla_end = t_carla

    # plot time when carla starts
    if(t_carla_start is not None):
        y = np.arange(int(np.max(data_ys)) + 5)
        x = np.ones_like(y) * t_carla_start
        plt.plot(x, y, color='c', linewidth=1)

    # plot time when carla starts
    if(t_carla_end is not None):
        y = np.arange(np.max(data_ys) + 5)
        x = np.ones_like(y) * t_carla_end
        plt.plot(x, y, color='c', linewidth=1)

    # complete the layout, save figure, and show the figure for you to see
    plt.tight_layout()
    # make file and save to disk
    if not os.path.exists(os.path.join(os.getcwd(), dir_path)):
        os.makedirs(dir_path, exist_ok=True)
    filename = name_x + "_vs_" + name_y + '.png' if name_x != "" else name_y + ".png"
    fig.savefig(os.path.join(dir_path, filename))
    plt.close(fig)
    print("Plotted", filename)


def read_from_file(datadir, filename):
    print("Looking for", filename)
    if not os.path.exists(os.path.join(os.getcwd(), datadir)):
        print("ERROR: could not find", datadir, "in cwd")
        os._exit(1)
    filepath = os.path.join(os.getcwd(), datadir, filename)
    with open(filepath, "r") as file:
        # the command 'collectl -sCm -oT -P -i 0.5 --sep ","' prints
        # and if closed via ctrl+C, outputs "Ouch!" at the end
        # simple fix: skip first line (header) and last line (footer)
        df = pandas.read_csv(file, skipfooter=1, engine='python')
        print("Successfully found and parsed", filename)
        return df


def convert_to_seconds(data_str):
    arr = []
    for x in data_str:
        (h, m, s) = x.split(':')
        t = int(h) * (60*60) + int(m) * 60 + float(s)
        arr.append(t)
    return np.array(arr)


def get_cpu_data_from(df, bounds):
    start, end = bounds
    cpus = []
    for i in range(start, end, 1):
        cpu_str = "[CPU:" + str(i) + "]Totl%"
        cpus.append(df[cpu_str].to_numpy())
    return cpus


def main(sys_data_df, dir_path):
    # get system time
    sys_time = sys_data_df["Time"].to_numpy()
    sys_time = convert_to_seconds(sys_time)

    # get cpu usages
    cpu_usages = get_cpu_data_from(sys_data_df, (0, 7))

    time = sys_time - sys_time[0]  # zero out (relative time)

    # time when carla started/ended, the run_collectl.sh makes entire columns for these
    t_start_str = sys_data_df["[CARLA]t_start"].to_numpy()[0]
    t_end_str = sys_data_df["[CARLA]t_end"].to_numpy()[0]

    t_carla_start = convert_to_seconds([t_start_str])[0] - sys_time[0]
    t_carla_end = convert_to_seconds([t_end_str])[0] - sys_time[0]

    t_carla = (t_carla_start, t_carla_end)

    average_cpu = np.mean(cpu_usages, axis=0)

    # plot carla fps
    carla_fps = sys_data_df["[CARLA]Fps"].to_numpy()
    carla_fps = carla_fps[~np.isnan(carla_fps)]  # strip away the np.nan's

    # pad the carla fps data to start when the t_carla_end occurs
    start_pads = len(time) - len(carla_fps) - len(time[time > t_carla_end])
    carla_fps = np.pad(carla_fps, (start_pads, 0),
                       'constant', constant_values=0)
    plot_many_versus(time, [carla_fps], name_x="time",
                     name_y="fps", dir_path=dir_path,
                     lines=True, points=False,
                     t_carla=t_carla)

    # plot individual core usage
    plot_many_versus(time, cpu_usages, name_x="time",
                     name_y="cores", dir_path=dir_path,
                     lines=True, points=False,
                     t_carla=t_carla)

    # plot cpu average
    plot_many_versus(time, [average_cpu], name_x="time",
                     name_y="avg_cpu", dir_path=dir_path,
                     lines=True, points=False, t_carla=t_carla)

    # plot memory usage
    mem_usage = sys_data_df["[MEM]Used"].to_numpy() / 1000000
    #sys_data_df["[MEM]Tot"].to_numpy() - sys_data_df["[MEM]Free"].to_numpy()
    plot_many_versus(time, [mem_usage], name_x="time",
                     name_y="mem_usage", dir_path=dir_path,
                     lines=True, points=False, t_carla=t_carla)

    # plot gpu usage
    gpu_usage = sys_data_df["[NVIDIA]Gpu"].to_numpy()
    plot_many_versus(time, [gpu_usage], name_x="time",
                     name_y="gpu_usage", dir_path=dir_path,
                     lines=True, points=False, t_carla=t_carla)

    # plot gpu memory
    gpu_memory = sys_data_df["[NVIDIA]Mem"].to_numpy()
    plot_many_versus(time, [gpu_memory], name_x="time",
                     name_y="gpu_memory", dir_path=dir_path,
                     lines=True, points=False, t_carla=t_carla)
    print("Successfully plotted everything to", dir_path)


def get_from_data(data_folder, filename):
    datadir = os.path.join(os.getcwd(), "data", data_folder)

    # output directory
    dir_path = os.path.join(os.getcwd(), "results", data_folder)

    # read from csv
    sys_data_df = read_from_file(datadir, filename)
    return sys_data_df, dir_path


if __name__ == '__main__':
    print("\nPlotting data:\n")
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        '-d', '--dir',
        metavar='D',
        default="collectl",  # cwd
        type=str,
        help='data directory name for outputs')
    args = argparser.parse_args()
    data_folder = args.dir
    output_path = os.path.join("results")

    sys_data_df, dir_path = get_from_data(data_folder=data_folder,
                                          filename="combined.csv")

    main(sys_data_df, dir_path)
