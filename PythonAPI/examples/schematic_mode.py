from typing import Any, Dict

from DReyeVR_utils import DReyeVRSensor, find_ego_vehicle
import no_rendering_mode
from no_rendering_mode import (
    COLOR_SCARLET_RED_1,
    World,
    main,
    game_loop,
)

import carla
import numpy as np
from scipy.spatial.transform import Rotation


try:
    import pygame
except ImportError:
    raise RuntimeError("cannot import pygame, make sure pygame package is installed")


class DReyeVRWorld(World):
    """Class that contains all World information from no_rendering_mode but with overrided DReyeVR functions"""

    def select_hero_actor(self):
        print("Initializing DReyeVR ego vehicle as hero_actor")
        world = self.client.get_world()
        self.hero_actor = find_ego_vehicle(world)
        self.hero_transform = self.hero_actor.get_transform()
        self.sensor = DReyeVRSensor(world)
        self.sensor.ego_sensor.listen(self.sensor.update)  # subscribe to readout

    def render_ego_sensor(self, surface, world_to_pixel, ray_length=20):
        data: Dict[str, Any] = self.sensor.data
        if len(data) == 0:
            return
        rot = Rotation.from_euler("yxz", data["camera_rotation"], degrees=True)
        ray_start = data["camera_location"] / 100 + rot.apply(data["gaze_origin"] / 100)
        # NOTE: ray_length is in carla metres
        ray_end_locn = ray_start + ray_length * rot.apply(data["gaze_dir"])
        gaze_ray_line = [
            carla.Location(ray_start[0], ray_start[1], ray_start[2]),
            carla.Location(ray_end_locn[0], ray_end_locn[1], ray_end_locn[2]),
        ]
        color = COLOR_SCARLET_RED_1
        # attach to ego-vehicle
        self.hero_actor.get_transform().transform(gaze_ray_line)
        # map gaze data to pixels
        gaze_ray_line = [world_to_pixel(p) for p in gaze_ray_line]
        # draw lines on schematic
        pygame.draw.lines(
            surface,
            color,
            False,
            gaze_ray_line,
            width=int(np.ceil(4.0 * self.map_image.scale)),
        )

    def render_actors(self, surface, vehicles, traffic_lights, speed_limits, walkers):
        super(DReyeVRWorld, self).render_actors(
            surface, vehicles, traffic_lights, speed_limits, walkers
        )
        # render additional eye tracker components
        self.render_ego_sensor(surface, self.map_image.world_to_pixel)


def schematic_run(args):
    # used when isolated and run in a script such as run_experiment.py
    no_rendering_mode.World = DReyeVRWorld  # hack to make the no_rendering_mode game_loop use the new DReyeVRWorld
    game_loop(args)


if __name__ == "__main__":
    no_rendering_mode.World = DReyeVRWorld  # hack to make the no_rendering_mode game_loop use the new DReyeVRWorld
    main()
