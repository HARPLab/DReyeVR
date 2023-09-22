#!/usr/bin/env python

# Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma de
# Barcelona (UAB).
#
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT>.

import glob
import os
import sys
import time

try:
    sys.path.append(glob.glob('../carla/dist/carla-*%d.%d-%s.egg' % (
        sys.version_info.major,
        sys.version_info.minor,
        'win-amd64' if os.name == 'nt' else 'linux-x86_64'))[0])
except IndexError:
    pass

import carla

import argparse

try:
    import queue
except ImportError:
    import Queue as queue

from DReyeVR_utils import DReyeVRSensor, find_ego_vehicle, find_ego_sensor

def ReplayStat(s):
    idx = s.find("ReplayStatus")
    if idx == -1:
        return 0
    print(s[idx + len("ReplayStatus=")])
    return (s[idx + len("ReplayStatus=")] == '1')


def main():

    argparser = argparse.ArgumentParser(
        description=__doc__)
    argparser.add_argument(
        '--host',
        metavar='H',
        default='127.0.0.1',
        help='IP of the host server (default: 127.0.0.1)')
    argparser.add_argument(
        '-p', '--port',
        metavar='P',
        default=2000,
        type=int,
        help='TCP port to listen to (default: 2000)')
    argparser.add_argument(
        '-s', '--start',
        metavar='S',
        default=0.0,
        type=float,
        help='starting time (default: 0.0)')
    argparser.add_argument(
        '-d', '--duration',
        metavar='D',
        default=0.0,
        type=float,
        help='duration (default: 0.0)')
    argparser.add_argument(
        '-f', '--recorder-filename',
        metavar='F',
        default="test1.log",
        help='recorder filename (test1.log)')
    argparser.add_argument(
        '-c', '--camera',
        metavar='C',
        default=0,
        type=int,
        help='camera follows an actor (ex: 82)')
    argparser.add_argument(
        '-x', '--time-factor',
        metavar='X',
        default=1.0,
        type=float,
        help='time factor (default 1.0)')
    argparser.add_argument(
        '-i', '--ignore-hero',
        action='store_true',
        help='ignore hero vehicles')
    argparser.add_argument(
        '--spawn-sensors',
        action='store_true',
        help='spawn sensors in the replayed world')
    argparser.add_argument(
        '-n', '--frame-number',
        default=0,
        type=int,
        help='number of frames in the recording')
    args = argparser.parse_args()
    egosensor = None
    instseg_camera = None

    try:

        client = carla.Client(args.host, args.port)
        client.set_timeout(60.0)

        world = client.get_world()

        blueprint_library = world.get_blueprint_library()

        ego_vehicle = find_ego_vehicle(world)

        
        client.set_timeout(60.0)

        # set the time factor for the replayer
        client.set_replayer_time_factor(args.time_factor)

        # set to ignore the hero vehicles or not
        client.set_replayer_ignore_hero(args.ignore_hero)


        egosensor = find_ego_sensor(world)
        ego_queue = queue.Queue()
        egosensor.listen(ego_queue.put)
        ReplayStatus = 0

        instseg_camera = world.spawn_actor(
            blueprint_library.find('sensor.camera.instance_segmentation'),
            carla.Transform(carla.Location(x=-5.5, z=2.8), carla.Rotation(pitch=-15)),
            attach_to=ego_vehicle)

        instseg_queue = queue.Queue()
        instseg_camera.listen(instseg_queue.put)

        rgb_camera = world.spawn_actor(
            blueprint_library.find('sensor.camera.rgb'),
            carla.Transform(carla.Location(x=-5.5, z=2.8), carla.Rotation(pitch=-15)),
            attach_to=ego_vehicle)

        rgb_queue = queue.Queue()
        rgb_camera.listen(rgb_queue.put)




        # replay the session
        print(client.replay_file(args.recorder_filename, args.start, args.duration, args.camera, args.spawn_sensors))


        replay_started = 0
        replaying = 0
     
        while (not replay_started) or replaying:
            replaying = ego_queue.queue[-1].replaystatus
            if replaying:
                replay_started = 1
            if (not replay_started) and ego_queue.qsize() > 200:
                break

        if egosensor:
            egosensor.destroy()
        if instseg_camera:
            instseg_camera.destroy()
        if rgb_camera:
            rgb_camera.destroy()
           
        if not replay_started:
            print("replay never started")
        else:
            replay_frames = set()
            while not ego_queue.empty():
                el = ego_queue.get(2.0)
                if el.replaystatus:
                    replay_frames.add(el.frame)
            first_frame = min(replay_frames)

            while not instseg_queue.empty():
                image = instseg_queue.get(2.0)
                if image.frame in replay_frames:
                    image.save_to_disk('instance_segmentation_output/%.6d.jpg' % (image.frame - first_frame + 1))
                
            while not rgb_queue.empty():
                image = rgb_queue.get(2.0)
                if image.frame in replay_frames:
                    image.save_to_disk('rgb_output/%.6d.jpg' % (image.frame - first_frame + 1))
            

    finally:
        pass
       

if __name__ == '__main__':

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        print('\ndone.')
