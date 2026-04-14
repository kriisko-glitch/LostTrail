"""Import Kenney Nature Kit FBX files into UE5 via Python remote execution."""
import sys
import time
import os

sys.path.insert(0, r'C:\Program Files\Epic Games\UE_5.6\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python')
from remote_execution import RemoteExecution

# The Python code to run inside UE5 editor
UE_SCRIPT = r'''
import unreal
import os

fbx_dir = r"D:\Games\assets\kenney_nature\Models\FBX format"
dest_base = "/Game/KenneyNature"

files_to_import = [
    "tree_default.fbx",
    "tree_detailed.fbx",
    "tree_oak.fbx",
    "tree_tall.fbx",
    "tree_thin.fbx",
    "tree_fat.fbx",
    "tree_pineDefaultA.fbx",
    "tree_pineTallA_detailed.fbx",
    "tree_pineRoundA.fbx",
    "tree_cone.fbx",
    "rock_largeA.fbx",
    "rock_largeB.fbx",
    "rock_largeC.fbx",
    "rock_tallA.fbx",
    "rock_smallA.fbx",
    "rock_smallB.fbx",
    "stone_largeA.fbx",
    "plant_bush.fbx",
    "plant_bushLarge.fbx",
    "plant_bushSmall.fbx",
    "plant_bushDetailed.fbx",
    "grass.fbx",
    "grass_large.fbx",
    "grass_leafs.fbx",
    "campfire_stones.fbx",
    "log.fbx",
    "log_stack.fbx",
    "flower_redA.fbx",
    "flower_yellowA.fbx",
]

tasks = []
for fname in files_to_import:
    full_path = os.path.join(fbx_dir, fname)
    if not os.path.exists(full_path):
        print(f"SKIP (not found): {fname}")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", full_path)
    task.set_editor_property("destination_path", dest_base)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_animations", False)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    task.set_editor_property("options", options)

    tasks.append(task)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks(tasks)

imported = 0
for task in tasks:
    paths = task.get_editor_property("imported_object_paths")
    if paths:
        imported += 1

print(f"IMPORT_RESULT: {imported}/{len(tasks)} assets imported to {dest_base}")
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

    print(f"Connected to editor node: {nodes[0]['node_id']}")
    re.open_command_connection(nodes[0]['node_id'])
    time.sleep(1)

    print("Starting FBX import (this may take 30-60 seconds)...")
    result = re.run_command(UE_SCRIPT, unattended=True)

    print(f"Success: {result['success']}")
    if result['output']:
        for line in result['output']:
            print(f"  [{line['type']}] {line['output'].strip()}")
    if result['result'] and result['result'] != 'None':
        print(f"  Result: {result['result']}")

    re.close_command_connection()
    re.stop()

if __name__ == '__main__':
    main()
