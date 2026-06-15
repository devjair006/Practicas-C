"""Clean RetroWeaponPack viewmodels and embed their albedo textures.

Run with Blender:
  blender --background --python tools/export_clean_weapon_viewmodels.py -- \
    --source-dir assets/armas --texture-root PATH_TO_RetroWeaponsPack
"""

import argparse
import os
import sys

import bpy


WEAPONS = {
    "FP_Arms_Pistol_01_Anims.glb": {
        "meshes": {"FPS_Arms_Mesh", "Pistol_Mesh"},
        "texture": "Guns/Pistol_01/Textures/Pistol_01_Albedo.png",
    },
    "FP_Arms_rifle_01_Anims.glb": {
        "meshes": {"FPS_Arms_Mesh", "ChargeHandle_Mesh"},
        "texture": "Guns/Rifle_01/Textures/Rifle_01_Albedo.png",
    },
    "FP_Arms_Shotgun_01_Anims.glb": {
        "meshes": {
            "FPS_Arms_Mesh",
            "Forearm_Mesh",
            "LoadingPort_Mesh",
            "MagazineLoadingPort_Mesh",
            "Main_Mesh",
            "Trigger_Mesh",
        },
        "texture": "Guns/Shotgun_01/Textures/Shotgun_01_Albedo.png",
    },
}

ARMS_TEXTURE = "FP_Arms/Texture/FPS_Arms_Albedo.png"


def script_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--texture-root", required=True)
    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    return parser.parse_args(args)


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
        for datablock in list(datablocks):
            datablocks.remove(datablock)


def ancestors(obj):
    result = set()
    parent = obj.parent
    while parent:
        result.add(parent)
        parent = parent.parent
    return result


def assign_albedo(obj, texture_path):
    image = bpy.data.images.load(texture_path, check_existing=True)
    for slot in obj.material_slots:
        material = slot.material
        if not material:
            continue
        material.use_nodes = True
        nodes = material.node_tree.nodes
        links = material.node_tree.links
        principled = next(
            (node for node in nodes if node.type == "BSDF_PRINCIPLED"), None
        )
        if not principled:
            continue
        for node in list(nodes):
            if node.type == "TEX_IMAGE":
                nodes.remove(node)
        texture_node = nodes.new("ShaderNodeTexImage")
        texture_node.image = image
        texture_node.interpolation = "Closest"
        links.new(texture_node.outputs["Color"], principled.inputs["Base Color"])
        if "Alpha" in texture_node.outputs and "Alpha" in principled.inputs:
            links.new(texture_node.outputs["Alpha"], principled.inputs["Alpha"])


def clean_weapon(path, texture_root, config):
    reset_scene()
    bpy.ops.import_scene.gltf(filepath=path)

    desired_meshes = {
        obj for obj in bpy.context.scene.objects
        if obj.type == "MESH" and obj.name in config["meshes"]
    }
    keep = set(desired_meshes)
    keep.update(obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE")
    for obj in list(keep):
        keep.update(ancestors(obj))

    for obj in list(bpy.context.scene.objects):
        if obj not in keep:
            bpy.data.objects.remove(obj, do_unlink=True)

    # Custom bone shapes are editor-only helpers. Blender otherwise exports
    # their Icosphere meshes even after the helper objects are removed.
    for armature in (obj for obj in keep if obj.type == "ARMATURE"):
        for pose_bone in armature.pose.bones:
            pose_bone.custom_shape = None

    arms_path = os.path.join(texture_root, ARMS_TEXTURE)
    weapon_path = os.path.join(texture_root, config["texture"])
    for obj in desired_meshes:
        assign_albedo(
            obj,
            arms_path if obj.name == "FPS_Arms_Mesh" else weapon_path,
        )

    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLB",
        export_animations=True,
        export_materials="EXPORT",
    )
    print(
        f"[VIEWMODEL] {os.path.basename(path)}: "
        f"{len(desired_meshes)} meshes limpios"
    )


def main():
    args = script_args()
    source_dir = os.path.abspath(args.source_dir)
    texture_root = os.path.abspath(args.texture_root)
    for filename, config in WEAPONS.items():
        clean_weapon(os.path.join(source_dir, filename), texture_root, config)


if __name__ == "__main__":
    main()
