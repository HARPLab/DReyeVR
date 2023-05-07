import time
import argparse
from numpy import random
from DReyeVR_utils import find_ego_vehicle

import carla


def set_DReyeVR_autopilot(world, traffic_manager):
    DReyeVR_vehicle = find_ego_vehicle(world)
    if DReyeVR_vehicle is not None:
        DReyeVR_vehicle.set_autopilot(True, traffic_manager.get_port())
        print("Successfully set autopilot on ego vehicle")
    return DReyeVR_vehicle


def spawn_other_vehicles(client, max_vehicles, world, traffic_manager):
    spawn_points = world.get_map().get_spawn_points()

    blueprints = world.get_blueprint_library().filter("vehicle.*")
    blueprints = sorted(blueprints, key=lambda bp: bp.id)

    SpawnActor = carla.command.SpawnActor
    SetAutopilot = carla.command.SetAutopilot
    FutureActor = carla.command.FutureActor

    vehicle_list = []
    batch = []
    for n, transform in enumerate(spawn_points):
        if n >= max_vehicles:
            break
        blueprint = random.choice(blueprints)
        if blueprint.has_attribute("color"):
            color = random.choice(blueprint.get_attribute("color").recommended_values)
            blueprint.set_attribute("color", color)
        if blueprint.has_attribute("driver_id"):
            driver_id = random.choice(
                blueprint.get_attribute("driver_id").recommended_values
            )
            blueprint.set_attribute("driver_id", driver_id)
        try:
            blueprint.set_attribute("role_name", "autopilot")
        except IndexError:
            pass

        batch.append(
            SpawnActor(blueprint, transform).then(
                SetAutopilot(FutureActor, True, traffic_manager.get_port())
            )
        )
    synchronous_master = False
    for response in client.apply_batch_sync(batch, synchronous_master):
        if response.error:
            print(f"ERROR: {response.error}")
        else:
            vehicle_list.append(response.actor_id)
    print(f"successfully spawned {len(vehicle_list)} vehicles")
    return vehicle_list


def main():
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        "--host",
        metavar="H",
        default="127.0.0.1",
        help="IP of the host server (default: 127.0.0.1)",
    )
    argparser.add_argument(
        "-p",
        "--port",
        metavar="P",
        default=2000,
        type=int,
        help="TCP port to listen to (default: 2000)",
    )
    argparser.add_argument(
        "-n",
        "--number-of-vehicles",
        metavar="N",
        default=10,
        type=int,
        help="number of vehicles (default: 10)",
    )
    argparser.add_argument(
        "--tm-port",
        metavar="P",
        default=8000,
        type=int,
        help="port to communicate with TM (default: 8000)",
    )
    argparser.add_argument(
        "-s", "--seed", metavar="S", type=int, help="Random device seed"
    )
    args = argparser.parse_args()

    client = carla.Client(args.host, args.port)
    client.set_timeout(10.0)
    random.seed(args.seed if args.seed is not None else int(time.time()))

    other_vehicles = []
    ego_vehicle = None
    try:
        world = client.get_world()

        traffic_manager = client.get_trafficmanager(args.tm_port)
        traffic_manager.set_global_distance_to_leading_vehicle(1.0)
        if args.seed is not None:
            traffic_manager.set_random_device_seed(args.seed)

        ego_vehicle = set_DReyeVR_autopilot(world, traffic_manager)

        # spawn other vehicles
        other_vehicles = spawn_other_vehicles(
            client, args.number_of_vehicles, world, traffic_manager
        )

        while True:
            world.wait_for_tick()
    finally:
        if ego_vehicle is not None:
            ego_vehicle.set_autopilot(False, traffic_manager.get_port())
        print("\ndestroying %d vehicles" % len(other_vehicles))
        client.apply_batch([carla.command.DestroyActor(x) for x in other_vehicles])

        time.sleep(0.5)


if __name__ == "__main__":

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        print("\ndone.")
