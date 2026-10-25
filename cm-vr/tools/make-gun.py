r"""
make-gun.py: a gun model of your own for Confidential Mission VR (hotd2-vr, Task 14b), made with
Blender (5.2) run headless:

  blender -b -P make-gun.py -- <in.fbx|in.glb|in.gltf> <out-dir> --name "My gun" --length 0.22
          [--texture-dir <folder>]

It imports the model, binds textures the file names but can't find (by file name, any case, in
--texture-dir and the model's own folder, also with another image extension), scales textures to
at most 1024 pixels a side, joins nothing, and exports <out-dir>/<in's name>.glb with the textures
inside, plus <in's name>.json, the sidecar with a first guess:
  muzzle  the far end of the longest axis (the end away from the grip), at the middle of what is there
  grip    the top of the lowest part (the mesh object lowest down; one object: its lowest 40 %)
  up      +y (Blender's +z), length and name as given
Check and correct the guess in the simulator (README, "Your own models"), then copy both files to
guns/ (on the Quest: Download/guns/; on the PC: next to flycast.exe or next to the game's zip).

Blender cannot read ASCII FBX (only binary FBX): convert an ASCII one first (e.g. with Autodesk's
FBX Converter, or open and export it as binary FBX or GLB in another program).

The app ships no model of anyone else's: you answer for the rights to the files you add.
"""
import argparse
import json
import os
import re
import sys

import bpy
from mathutils import Vector

IMAGE_EXTENSIONS = ('.png', '.jpg', '.jpeg', '.tga', '.bmp', '.tif', '.tiff', '.webp')
MAX_SIDE = 1024
MAX_TRIANGLES = 200000


def fail(message):
    print('make-gun: ' + message, file=sys.stderr)
    sys.exit(1)


def parse():
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    ap = argparse.ArgumentParser(prog='blender -b -P make-gun.py --', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('model', help='the model: .fbx (binary; Blender cannot read ASCII FBX), .glb or .gltf')
    ap.add_argument('out_dir', help='where the .glb and its .json sidecar go')
    ap.add_argument('--name', required=True, help="the VR menu's label, at most 24 characters")
    ap.add_argument('--length', type=float, required=True, help='metres along the barrel, 0.05 to 1.5 (a pistol: about 0.2)')
    ap.add_argument('--texture-dir', help='where to look for textures the model names but are not where it says')
    a = ap.parse_args(argv)
    if not 0.05 <= a.length <= 1.5:
        fail('--length %g is outside 0.05-1.5 m' % a.length)
    if len(a.name) > 24:
        fail('--name is %d characters: at most 24' % len(a.name))
    return a


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ext = os.path.splitext(path)[1].lower()
    if ext == '.fbx':
        with open(path, 'rb') as f:
            if not f.read(20).startswith(b'Kaydara FBX Binary'):
                fail(path + ' is an ASCII FBX: Blender cannot read it; convert it to binary FBX or GLB first')
        bpy.ops.import_scene.fbx(filepath=path)
    elif ext in ('.glb', '.gltf'):
        bpy.ops.import_scene.gltf(filepath=path)
    else:
        fail('not a .fbx, .glb or .gltf: ' + path)


def bind_textures(dirs):
    """images whose files aren't where the model says: the same file name in dirs (any case,
    also with another image extension)"""
    found = {}
    for d in dirs:
        for root, _, files in os.walk(d):
            for name in files:
                found.setdefault(name.lower(), os.path.join(root, name))
    for image in bpy.data.images:
        if image.source != 'FILE' or image.packed_file is not None:
            continue
        path = bpy.path.abspath(image.filepath)
        if os.path.isfile(path):
            continue
        base = os.path.basename(path.replace('\\', '/')).lower()
        stem = os.path.splitext(base)[0]
        hit = found.get(base) or next((found[stem + e] for e in IMAGE_EXTENSIONS if stem + e in found), None)
        if hit:
            image.filepath = hit
            image.reload()
            print('make-gun: texture %s bound to %s' % (base, hit))
        else:
            print('make-gun: texture %s not found (that part keeps its colour)' % base)


def fit_textures():
    for image in bpy.data.images:
        w, h = image.size[0], image.size[1]
        if w > MAX_SIDE or h > MAX_SIDE:
            f = MAX_SIDE / max(w, h)
            image.scale(max(1, int(w * f)), max(1, int(h * f)))
            image.pack()
            print('make-gun: texture %s %dx%d -> %dx%d' % (image.name, w, h, image.size[0], image.size[1]))


def to_gltf(v):
    """Blender (z up) -> glTF (y up), as the exporter turns it"""
    return Vector((v.x, v.z, -v.y))


def guess():
    """grip and muzzle in the GLB's space, and the triangle count"""
    deps = bpy.context.evaluated_depsgraph_get()
    parts, triangles = [], 0
    for obj in bpy.context.scene.objects:
        if obj.type != 'MESH':
            continue
        mesh = obj.evaluated_get(deps).to_mesh()
        mesh.calc_loop_triangles()
        triangles += len(mesh.loop_triangles)
        points = [to_gltf(obj.matrix_world @ v.co) for v in mesh.vertices]
        obj.evaluated_get(deps).to_mesh_clear()
        if points:
            parts.append(points)
    if not parts:
        fail('no mesh in the model')
    every = [p for part in parts for p in part]
    lo = Vector([min(p[k] for p in every) for k in range(3)])
    hi = Vector([max(p[k] for p in every) for k in range(3)])
    # the grip: the top of the lowest part (one part: its lowest 40 %)
    if len(parts) > 1:
        low = min(parts, key=lambda part: min(p.y for p in part))
    else:
        cut = lo.y + 0.4 * (hi.y - lo.y)
        low = [p for p in every if p.y <= cut]
    grip = Vector((sum(p.x for p in low) / len(low), max(p.y for p in low), sum(p.z for p in low) / len(low)))
    # the muzzle: the end of the longest axis away from the grip, the middle of what is there
    axis = max(range(3), key=lambda k: hi[k] - lo[k])
    end = hi[axis] if hi[axis] - grip[axis] >= grip[axis] - lo[axis] else lo[axis]
    near = [p for p in every if abs(p[axis] - end) <= 0.05 * (hi[axis] - lo[axis])]
    muzzle = Vector([sum(p[k] for p in near) / len(near) for k in range(3)])
    muzzle[axis] = end
    return grip, muzzle, triangles


def export(path):
    try:
        bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', export_apply=True, export_animations=False)
    except TypeError:
        bpy.ops.export_scene.gltf(filepath=path, export_format='GLB')


def main():
    a = parse()
    model = os.path.abspath(a.model)
    if not os.path.isfile(model):
        fail('no file ' + model)
    load(model)
    bind_textures([d for d in (a.texture_dir, os.path.dirname(model)) if d])
    fit_textures()
    grip, muzzle, triangles = guess()
    os.makedirs(a.out_dir, exist_ok=True)
    stem = re.sub(r'[^\w.-]+', '-', os.path.splitext(os.path.basename(model))[0]).strip('-') or 'gun'
    out = os.path.join(os.path.abspath(a.out_dir), stem + '.glb')
    export(out)
    side = {'name': a.name, 'grip': [round(c, 5) for c in grip], 'muzzle': [round(c, 5) for c in muzzle],
            'up': [0, 1, 0], 'length': a.length}
    with open(os.path.splitext(out)[0] + '.json', 'w') as f:
        lines = ['  %s: %s' % (json.dumps(k), json.dumps(v)) for k, v in side.items()]
        f.write('{' + chr(10) + (',' + chr(10)).join(lines) + chr(10) + '}' + chr(10))
    print('make-gun: %s (%d triangles%s, %d texture%s)' % (out, triangles,
          ': over %d, the app refuses it' % MAX_TRIANGLES if triangles > MAX_TRIANGLES else '',
          len(bpy.data.images), '' if len(bpy.data.images) == 1 else 's'))
    print('make-gun: sidecar %s: grip %s, muzzle %s (a first guess: check it in the simulator)' % (
          os.path.splitext(out)[0] + '.json', side['grip'], side['muzzle']))


main()
