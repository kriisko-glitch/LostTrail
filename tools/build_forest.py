"""
Lost Trail Forest Builder
Places trees, rocks, water sources, and POIs in the level via UE5 RC API.
Also creates a NavMeshBoundsVolume for AI navigation.

Usage: python build_forest.py
Requires: UE5 editor running with LostTrail project open
"""

import sys
import os
import random
import math

sys.path.insert(0, 'C:/Users/Kris/Kriisko-Studio/tools')

# Use the RC API
try:
    from ue5_scene_ops._rc import rc_python
except ImportError:
    print("ERROR: Cannot import ue5_scene_ops. Make sure studio tools are available.")
    sys.exit(1)


def rc(code):
    """Execute Python in UE5 editor."""
    ok, result = rc_python(code)
    if not ok:
        print(f"RC FAILED: {result}")
        return False
    return True


def build_forest():
    """Place forest elements in the level."""

    print("=== Lost Trail Forest Builder ===")
    print()

    # Step 1: Create a ground plane (large flat landscape)
    print("[1/6] Creating forest ground...")
    rc('''
import unreal

# Spawn a large floor plane
editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
location = unreal.Vector(0, 0, -50)
rotation = unreal.Rotator(0, 0, 0)

floor = editor.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
if floor:
    mesh_comp = floor.static_mesh_component
    plane_mesh = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Plane')
    if plane_mesh:
        mesh_comp.set_static_mesh(plane_mesh)
        mesh_comp.set_world_scale3d(unreal.Vector(100, 100, 1))  # 10000x10000 ground
        # Dark green ground
        mat = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/BasicShapeMaterial')
        if mat:
            mesh_comp.set_material(0, mat)
    floor.set_actor_label('ForestGround')
    floor.tags.append('ground')
    print(f"Ground placed at {location}")
else:
    print("FAILED to create ground")
''')

    # Step 2: Place trees (cylinders + spheres for trunk + canopy)
    print("[2/6] Placing trees...")
    tree_count = 80
    rc(f'''
import unreal
import random

editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cylinder = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cylinder')
sphere = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Sphere')

placed = 0
for i in range({tree_count}):
    # Random position in a 8000x8000 area, avoiding center (player spawn)
    x = random.uniform(-4000, 4000)
    y = random.uniform(-4000, 4000)

    # Keep a clear area around the origin (player spawn)
    if abs(x) < 500 and abs(y) < 500:
        continue

    # Trunk
    loc = unreal.Vector(x, y, 100)
    rot = unreal.Rotator(0, random.uniform(0, 360), 0)
    trunk = editor.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if trunk and cylinder:
        trunk.static_mesh_component.set_static_mesh(cylinder)
        scale_y = random.uniform(0.3, 0.6)
        trunk.set_actor_scale3d(unreal.Vector(scale_y, scale_y, random.uniform(1.5, 3.5)))
        trunk.set_actor_label(f'Tree_Trunk_{{i:03d}}')
        trunk.tags.append('tree')

    # Canopy (sphere on top)
    canopy_loc = unreal.Vector(x, y, 300 + random.uniform(0, 150))
    canopy = editor.spawn_actor_from_class(unreal.StaticMeshActor, canopy_loc, rot)
    if canopy and sphere:
        canopy.static_mesh_component.set_static_mesh(sphere)
        s = random.uniform(1.5, 3.0)
        canopy.set_actor_scale3d(unreal.Vector(s, s, s * 0.7))
        canopy.set_actor_label(f'Tree_Canopy_{{i:03d}}')
        canopy.tags.append('tree')

    placed += 1

print(f"Placed {{placed}} trees")
''')

    # Step 3: Place rocks
    print("[3/6] Placing rocks...")
    rc('''
import unreal
import random

editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sphere = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Sphere')

for i in range(30):
    x = random.uniform(-4000, 4000)
    y = random.uniform(-4000, 4000)

    loc = unreal.Vector(x, y, random.uniform(-20, 30))
    rot = unreal.Rotator(random.uniform(-15, 15), random.uniform(0, 360), random.uniform(-15, 15))
    rock = editor.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if rock and sphere:
        rock.static_mesh_component.set_static_mesh(sphere)
        sx = random.uniform(0.5, 2.0)
        sy = random.uniform(0.5, 2.0)
        sz = random.uniform(0.3, 1.0)
        rock.set_actor_scale3d(unreal.Vector(sx, sy, sz))
        rock.set_actor_label(f'Rock_{i:03d}')
        rock.tags.append('rock')

print("Placed 30 rocks")
''')

    # Step 4: Place water sources (tagged for dog detection)
    print("[4/6] Placing water sources and food POIs...")
    rc('''
import unreal
import random

editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sphere = unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Sphere')

# Water sources (3-4 around the map)
water_positions = [
    (1500, 2000), (-2000, 1500), (2500, -1800), (-1000, -2500)
]
for i, (wx, wy) in enumerate(water_positions):
    loc = unreal.Vector(wx, wy, -30)
    water = editor.spawn_actor_from_class(unreal.StaticMeshActor, loc)
    if water and sphere:
        water.static_mesh_component.set_static_mesh(sphere)
        water.set_actor_scale3d(unreal.Vector(3.0, 3.0, 0.15))
        water.set_actor_label(f'WaterSource_{i}')
        water.tags.append('poi.water')

# Berry bushes (8 scattered)
for i in range(8):
    x = random.uniform(-3500, 3500)
    y = random.uniform(-3500, 3500)
    loc = unreal.Vector(x, y, 20)
    bush = editor.spawn_actor_from_class(unreal.StaticMeshActor, loc)
    if bush and sphere:
        bush.static_mesh_component.set_static_mesh(sphere)
        bush.set_actor_scale3d(unreal.Vector(0.5, 0.5, 0.4))
        bush.set_actor_label(f'BerryBush_{i}')
        bush.tags.append('poi.food')

# Shelter spots (3 — near rock clusters)
shelter_positions = [(-1500, 1000), (2000, -1000), (0, -3000)]
for i, (sx, sy) in enumerate(shelter_positions):
    loc = unreal.Vector(sx, sy, 50)
    shelter = editor.spawn_actor_from_class(unreal.StaticMeshActor, loc)
    if shelter and sphere:
        shelter.static_mesh_component.set_static_mesh(sphere)
        shelter.set_actor_scale3d(unreal.Vector(2.0, 2.0, 1.5))
        shelter.set_actor_label(f'Shelter_{i}')
        shelter.tags.append('poi.shelter')

print("Placed water, food, and shelter POIs")
''')

    # Step 5: Place NavMeshBoundsVolume
    print("[5/6] Creating NavMesh bounds volume...")
    rc('''
import unreal

editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# NavMeshBoundsVolume covering the forest area
loc = unreal.Vector(0, 0, 200)
nav_vol = editor.spawn_actor_from_class(unreal.NavMeshBoundsVolume, loc)
if nav_vol:
    nav_vol.set_actor_scale3d(unreal.Vector(100, 100, 20))  # 10000x10000x2000
    nav_vol.set_actor_label('ForestNavMesh')
    print("NavMeshBoundsVolume placed")
else:
    print("FAILED to create NavMeshBoundsVolume")
''')

    # Step 6: Add lighting
    print("[6/6] Adding forest lighting...")
    rc('''
import unreal

editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Directional light (sun)
sun_loc = unreal.Vector(0, 0, 1000)
sun_rot = unreal.Rotator(-45, 30, 0)
sun = editor.spawn_actor_from_class(unreal.DirectionalLight, sun_loc, sun_rot)
if sun:
    sun.set_actor_label('ForestSun')
    print("Directional light placed")

# Sky light for ambient
sky_loc = unreal.Vector(0, 0, 500)
sky = editor.spawn_actor_from_class(unreal.SkyLight, sky_loc)
if sky:
    sky.set_actor_label('ForestSkyLight')
    print("Sky light placed")

# Exponential height fog for atmosphere
fog_loc = unreal.Vector(0, 0, 0)
fog = editor.spawn_actor_from_class(unreal.ExponentialHeightFog, fog_loc)
if fog:
    fog.set_actor_label('ForestFog')
    print("Height fog placed")

# Sky atmosphere
atm = editor.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
if atm:
    atm.set_actor_label('ForestAtmosphere')
    print("Sky atmosphere placed")

print("Forest lighting complete")
''')

    print()
    print("=== Forest build complete! ===")
    print("Save the level, then hit Play to test.")


if __name__ == "__main__":
    build_forest()
