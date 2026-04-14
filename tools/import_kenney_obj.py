"""Import Kenney Nature Kit OBJ files into UE5 via Python remote execution."""
import sys
import time

sys.path.insert(0, r'C:\Program Files\Epic Games\UE_5.6\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python')
from remote_execution import RemoteExecution

UE_SCRIPT = r'''
import unreal
import os

obj_dir = r"D:\Games\assets\kenney_nature\Models\OBJ format"
dest_base = "/Game/KenneyNature"

files_to_import = [
    "tree_default.obj",
    "tree_detailed.obj",
    "tree_oak.obj",
    "tree_tall.obj",
    "tree_thin.obj",
    "tree_fat.obj",
    "tree_pineDefaultA.obj",
    "tree_pineTallA_detailed.obj",
    "tree_pineRoundA.obj",
    "tree_cone.obj",
    "rock_largeA.obj",
    "rock_largeB.obj",
    "rock_largeC.obj",
    "rock_tallA.obj",
    "rock_smallA.obj",
    "rock_smallB.obj",
    "stone_largeA.obj",
    "plant_bush.obj",
    "plant_bushLarge.obj",
    "plant_bushSmall.obj",
    "plant_bushDetailed.obj",
    "grass.obj",
    "grass_large.obj",
    "grass_leafs.obj",
    "campfire_stones.obj",
    "log.obj",
    "log_stack.obj",
    "flower_redA.obj",
    "flower_yellowA.obj",
]

tasks = []
for fname in files_to_import:
    full_path = os.path.join(obj_dir, fname)
    if not os.path.exists(full_path):
        print(f"SKIP: {fname}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", full_path)
    task.set_editor_property("destination_path", dest_base)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    tasks.append(task)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks(tasks)

imported = 0
failed = []
for i, task in enumerate(tasks):
    paths = task.get_editor_property("imported_object_paths")
    if paths:
        imported += 1
    else:
        failed.append(files_to_import[i])

print(f"IMPORT_RESULT: {imported}/{len(tasks)} assets imported")
if failed:
    print(f"FAILED: {', '.join(failed[:5])}")

# Verify a tree mesh actually has geometry
test_mesh = unreal.load_asset("/Game/KenneyNature/tree_default")
if test_mesh:
    bounds = test_mesh.get_bounds()
    box_extent = bounds.box_extent
    print(f"VERIFY tree_default bounds: {box_extent.x:.1f} x {box_extent.y:.1f} x {box_extent.z:.1f}")
else:
    print("VERIFY: tree_default not found after import")
'''

def main():
    re = RemoteExecution()
    re.start()
    time.sleep(2)
    nodes = re.remote_nodes
    if not nodes:
        print("No UE5 editor found")
        re.stop()
        return

    print(f"Connected to editor")
    re.open_command_connection(nodes[0]['node_id'])
    time.sleep(1)

    print("Importing OBJ assets...")
    result = re.run_command(UE_SCRIPT, unattended=True)
    print(f"Success: {result['success']}")
    for line in result.get('output', []):
        print(f"  {line['output'].strip()}")
    if not result['success']:
        print(f"  Error: {result['result']}")

    # Save all
    print("Saving assets to disk...")
    save_result = re.run_command('''
import unreal
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(save_map_packages=True, save_content_packages=True)
print("All dirty packages saved")
''', unattended=True)
    for line in save_result.get('output', []):
        print(f"  {line['output'].strip()}")

    re.close_command_connection()
    re.stop()

if __name__ == '__main__':
    main()
