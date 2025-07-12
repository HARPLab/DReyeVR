import carla

client = carla.Client("127.0.0.1", 2000)
client.set_timeout(10.0)

try:
    world = client.get_world()
    print("✅ 接続成功: マップ =", world.get_map().name)
except RuntimeError as e:
    print("❌ 接続失敗:", e)
