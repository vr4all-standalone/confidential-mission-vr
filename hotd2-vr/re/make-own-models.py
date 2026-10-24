r"""
Test fixtures for the player's own models and the bundled ones (Task 14b, S37; accept.ps1 cm-own-gun,
cm-own-controllers, cm-bundled): self-made boxes written at test time, so no third-party model is ever in the repo.

  python -I make-own-models.py <root> [--guns] [--controllers] [--no-sidecar] [--only-right] [--early] [--only-box]
                                [--no-buttons] [--bad-button]

<root>\guns\boxgun.glb + boxgun.json: a box gun (grip, body, barrel), each its own base colour (red, green,
blue), the body 8 slabs of the same green (10 primitives that make 3 parts), the barrel (a muzzle brake, wider
and taller than the body: it shows from behind) also textured (a 4x4 PNG, light checks), in made-up units (2
long) with the barrel along +x and up +z (so the app must turn and scale it). <root>\guns\zz-big.glb + .json: a
grid of 204,800 triangles, over the app's 200,000 (refused). bad-inf, bad-up, bad-far (the box gun's GLB):
sidecars with a grip of 1e39, an up of 1e30, a muzzle 500 units away (each refused). tex-a, tex-b, tex-c: 8
boxes, each with its own 2048 x 2048 PNG (128 MB of RGBA a gun, 384 MB in all: each is still offered, there is
no total across the guns). --early: aa-early.glb + .json ("Early gun", the box gun's GLB), sorting before boxgun.
--only-box: the box gun alone (and aa-early with --early), none of the others. <root>\controllers\left.glb,
right.glb + .json: a box controller in cm (3 primitives, 2 parts: dark grey in two halves, orange), its aim node
2 cm up, 6 cm forward, tilted 30 degrees; its sidecar's "buttons" (the how-to-play labels' points, Task 15b) at
known spots on the orange plate's top and the body's front (left: x, y, menu, stick, trigger; right: a, b, stick,
trigger). --no-buttons: the sidecars without "buttons". --bad-button: the left one's "menu" 5 m off (refused: its
label keeps the built-in place).

Prints the box gun's muzzle in gun space (barrel -z, y up, metres, the grip's top the origin) as the app
should find it from the sidecar: "muzzle <x> <y> <z>"; for each gun the app should offer by its checks
alone, its textures' RGBA bytes: "textures <stem> <bytes>"; and for each controller "buttons" point the app
should take, where the label's line ends in the hand's aim pose (metres: scale * inverse(aim node) * point):
"label <hand> <button> <x> <y> <z>".
"""
import argparse
import json
import math
import os
import struct
import zlib


def png(width, height, pixel=None, raw=None):
    if raw is None:
        raw = b''.join(b'\x00' + b''.join(bytes(pixel(x, y)) for x in range(width)) for y in range(height))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def box(lo, hi):
    """24 vertices (a normal per face), 12 triangles: positions, normals, uvs, indices"""
    pos, nrm, uv, idx = [], [], [], []
    for axis in range(3):
        for sign in (-1, 1):
            u, v = [(1, 2), (2, 0), (0, 1)][axis]
            base = len(pos)
            for a, b in ((0, 0), (1, 0), (1, 1), (0, 1)):
                p = [0.0, 0.0, 0.0]
                p[axis] = hi[axis] if sign > 0 else lo[axis]
                p[u] = hi[u] if a else lo[u]
                p[v] = hi[v] if b else lo[v]
                pos.append(p)
                n = [0.0, 0.0, 0.0]
                n[axis] = float(sign)
                nrm.append(n)
                uv.append([float(a), float(b)])
            quad = [0, 1, 2, 0, 2, 3] if sign > 0 else [0, 2, 1, 0, 3, 2]
            idx += [base + k for k in quad]
    return pos, nrm, uv, idx


class Glb:
    def __init__(self):
        self.bin = bytearray()
        self.doc = {'asset': {'version': '2.0', 'generator': 'hotd2-vr make-own-models.py'}, 'buffers': [],
                    'bufferViews': [], 'accessors': [], 'materials': [], 'meshes': [{'primitives': []}],
                    'nodes': [{'mesh': 0}], 'scenes': [{'nodes': [0]}], 'scene': 0}

    def view(self, data, target=None):
        while len(self.bin) % 4:
            self.bin.append(0)
        v = {'buffer': 0, 'byteOffset': len(self.bin), 'byteLength': len(data)}
        if target:
            v['target'] = target
        self.bin += data
        self.doc['bufferViews'].append(v)
        return len(self.doc['bufferViews']) - 1

    def floats(self, rows, kind):
        data = b''.join(struct.pack('<%df' % len(r), *r) for r in rows)
        a = {'bufferView': self.view(data, 34962), 'componentType': 5126, 'count': len(rows), 'type': kind}
        if kind == 'VEC3':
            a['min'] = [min(r[k] for r in rows) for k in range(3)]
            a['max'] = [max(r[k] for r in rows) for k in range(3)]
        self.doc['accessors'].append(a)
        return len(self.doc['accessors']) - 1

    def primitive(self, mesh, color, texture=None):
        pos, nrm, uv, idx = mesh
        big = len(pos) > 65535
        data = struct.pack('<%d%s' % (len(idx), 'I' if big else 'H'), *idx)
        self.doc['accessors'].append({'bufferView': self.view(data, 34963), 'componentType': 5125 if big else 5123,
                                      'count': len(idx), 'type': 'SCALAR'})
        indices = len(self.doc['accessors']) - 1
        material = {'pbrMetallicRoughness': {'baseColorFactor': list(color) + [1.0], 'metallicFactor': 0.0}}
        if texture is not None:
            image = self.view(texture)
            self.doc.setdefault('images', []).append({'bufferView': image, 'mimeType': 'image/png'})
            self.doc.setdefault('samplers', [{}])
            self.doc.setdefault('textures', []).append({'source': len(self.doc['images']) - 1, 'sampler': 0})
            material['pbrMetallicRoughness']['baseColorTexture'] = {'index': len(self.doc['textures']) - 1}
        self.doc['materials'].append(material)
        attributes = {'POSITION': self.floats(pos, 'VEC3'), 'NORMAL': self.floats(nrm, 'VEC3'), 'TEXCOORD_0': self.floats(uv, 'VEC2')}
        self.doc['meshes'][0]['primitives'].append({'attributes': attributes, 'indices': indices,
                                                    'material': len(self.doc['materials']) - 1})

    def write(self, path):
        while len(self.bin) % 4:
            self.bin.append(0)
        self.doc['buffers'] = [{'byteLength': len(self.bin)}]
        text = json.dumps(self.doc, separators=(',', ':')).encode()
        text += b' ' * (-len(text) % 4)
        body = struct.pack('<II', len(text), 0x4E4F534A) + text + struct.pack('<II', len(self.bin), 0x004E4942) + bytes(self.bin)
        with open(path, 'wb') as f:
            f.write(struct.pack('<III', 0x46546C67, 2, 12 + len(body)) + body)


# The box gun: the grip's top at the origin, the barrel along +x, up +z; 2 units from the body's back to the muzzle.
GUN = {'name': 'Box gun', 'grip': [0.0, 0.0, 0.0], 'muzzle': [1.5, 0.0, 0.25], 'up': [0.0, 0.0, 1.0], 'length': 0.25}
RED, GREEN, BLUE = (0.85, 0.08, 0.08), (0.10, 0.75, 0.12), (0.15, 0.25, 1.0)


def gun_muzzle(side):
    """the muzzle in gun space as the README says the app finds it: the barrel (muzzle - grip, without its part
    along up) is -z, up is +y, scaled so the model's extent along the barrel is length"""
    g, m, up = side['grip'], side['muzzle'], side['up']
    d = [m[k] - g[k] for k in range(3)]
    ul = math.sqrt(sum(c * c for c in up))
    u = [c / ul for c in up]
    du = sum(d[k] * u[k] for k in range(3))
    f = [d[k] - du * u[k] for k in range(3)]
    fl = math.sqrt(sum(c * c for c in f))
    f = [c / fl for c in f]
    x = [u[1] * -f[2] - u[2] * -f[1], u[2] * -f[0] - u[0] * -f[2], u[0] * -f[1] - u[1] * -f[0]]
    extent = 1.5 - (-0.5)   # the body's back (x -0.5) to the barrel's end (x 1.5)
    s = side['length'] / extent
    return [s * sum(d[k] * a[k] for k in range(3)) for a in (x, u, [-c for c in f])]


def box_gun():
    g = Glb()
    g.primitive(box([-0.3, -0.15, -1.2], [0.1, 0.15, 0.0]), RED)
    # the body in 8 slabs of one colour: merged into one part by the app
    for k in range(8):
        g.primitive(box([-0.5 + 1.5 * k / 8, -0.17, 0.0], [-0.5 + 1.5 * (k + 1) / 8, 0.17, 0.45]), GREEN)
    checks = png(4, 4, lambda x, y: (255, 255, 255, 255) if (x + y) % 2 else (215, 215, 215, 255))
    g.primitive(box([1.0, -0.25, -0.05], [1.5, 0.25, 0.55]), BLUE, checks)
    return g


def write_guns(root, sidecar, early, only_box=False):
    os.makedirs(os.path.join(root, 'guns'), exist_ok=True)
    g = box_gun()
    g.write(os.path.join(root, 'guns', 'boxgun.glb'))
    if sidecar:
        with open(os.path.join(root, 'guns', 'boxgun.json'), 'w') as f:
            json.dump(GUN, f, indent=1)
        print('textures boxgun %d' % (4 * 4 * 4))
    if early:
        g.write(os.path.join(root, 'guns', 'aa-early.glb'))
        with open(os.path.join(root, 'guns', 'aa-early.json'), 'w') as f:
            json.dump(dict(GUN, name='Early gun'), f, indent=1)
        print('textures aa-early %d' % (4 * 4 * 4))
    if only_box:
        return
    # over the cap: 320 x 320 quads
    n = 320
    pos = [[x / n, y / n, 0.0] for y in range(n + 1) for x in range(n + 1)]
    idx = []
    for y in range(n):
        for x in range(n):
            a = y * (n + 1) + x
            idx += [a, a + 1, a + n + 2, a, a + n + 2, a + n + 1]
    big = Glb()
    big.primitive((pos, [[0.0, 0.0, 1.0]] * len(pos), [[0.0, 0.0]] * len(pos), idx), GREEN)
    big.write(os.path.join(root, 'guns', 'zz-big.glb'))
    with open(os.path.join(root, 'guns', 'zz-big.json'), 'w') as f:
        json.dump({'name': 'Big grid', 'grip': [0, 0, 0], 'muzzle': [1, 0, 0], 'length': 0.3}, f)
    # sidecars the app must refuse (the box gun's GLB): a grip past a float, an up that overflows, a muzzle far off
    bad = {'bad-inf': dict(GUN, name='Inf grip', grip=[1e39, 0.0, 0.0]),
           'bad-up': dict(GUN, name='Huge up', up=[0.0, 0.0, 1e30]),
           'bad-far': dict(GUN, name='Far muzzle', muzzle=[500.0, 0.0, 0.25])}
    for stem, side in bad.items():
        g.write(os.path.join(root, 'guns', stem + '.glb'))
        with open(os.path.join(root, 'guns', stem + '.json'), 'w') as f:
            json.dump(side, f, indent=1)
    # 128 MB of textures each (8 x 2048 x 2048 RGBA), 384 MB in all: more than the folder total there once was
    side = 2048
    images = [png(side, side, raw=(b'\x00' + bytes((40 + 25 * k, 200 - 20 * k, 90, 255)) * side) * side) for k in range(8)]
    for stem in 'tex-a', 'tex-b', 'tex-c':
        t = Glb()
        for k in range(8):
            t.primitive(box([0.2 * k, 0.0, 0.0], [0.2 * k + 0.2, 0.2, 0.2]), (1.0, 1.0, 1.0), images[k])
        t.write(os.path.join(root, 'guns', stem + '.glb'))
        with open(os.path.join(root, 'guns', stem + '.json'), 'w') as f:
            json.dump({'name': stem.replace('-', ' ').title(), 'grip': [0.1, 0.0, 0.1], 'muzzle': [1.6, 0.1, 0.1],
                       'length': 0.3}, f)
        print('textures %s %d' % (stem, 8 * side * side * 4))


# The box controller's label points (cm, GLB space): the face buttons and the stick on the orange plate's top (y 3),
# the trigger on the body's front (z -6); the right hand's mirrored in x.
BUTTONS = {'left': {'x': [-1.5, 3.0, -3.0], 'y': [-1.5, 3.0, -5.5], 'menu': [1.5, 3.0, -2.5], 'stick': [1.5, 3.0, -5.5],
                    'trigger': [0.0, -1.0, -6.0]},
           'right': {'a': [1.5, 3.0, -3.0], 'b': [1.5, 3.0, -5.5], 'stick': [-1.5, 3.0, -5.5], 'trigger': [0.0, -1.0, -6.0]}}


def to_aim(point, aim, scale):
    """a GLB point in the aim pose, as the app places the model there: scale * inverse(aim node) * point"""
    qx, qy, qz, qw = aim['rotation']
    v = [point[k] - aim['position'][k] for k in range(3)]
    # the node's rotation undone: v rotated by the conjugate quaternion
    x, y, z = v
    cx, cy, cz = -qx, -qy, -qz
    tx, ty, tz = 2 * (cy * z - cz * y), 2 * (cz * x - cx * z), 2 * (cx * y - cy * x)
    r = (x + qw * tx + cy * tz - cz * ty, y + qw * ty + cz * tx - cx * tz, z + qw * tz + cx * ty - cy * tx)
    return [scale * c for c in r]


def write_controllers(root, hands, buttons=True, bad_button=False):
    os.makedirs(os.path.join(root, 'controllers'), exist_ok=True)
    half = math.radians(30) / 2
    for hand in hands:
        c = Glb()
        # the body in two halves of one colour: merged into one part by the app
        c.primitive(box([-3.0, -2.0, -6.0], [3.0, 2.0, 0.0]), (0.18, 0.18, 0.2))
        c.primitive(box([-3.0, -2.0, 0.0], [3.0, 2.0, 6.0]), (0.18, 0.18, 0.2))
        c.primitive(box([-3.5, 2.0, -7.0], [3.5, 3.0, -2.0]), (1.0, 0.45, 0.05))
        c.write(os.path.join(root, 'controllers', hand + '.glb'))
        side = {'aim': {'position': [0.0, 2.0, -6.0], 'rotation': [math.sin(half), 0.0, 0.0, math.cos(half)]}, 'scale': 0.01}
        if buttons:
            points = dict(BUTTONS[hand])
            if bad_button and hand == 'left':
                points['menu'] = [500.0, 0.0, 0.0]
            side['buttons'] = points
            for name, p in points.items():
                if max(abs(c) for c in p) < 100:
                    print('label %s %s %.5f %.5f %.5f' % ((hand, name) + tuple(to_aim(p, side['aim'], side['scale']))))
        with open(os.path.join(root, 'controllers', hand + '.json'), 'w') as f:
            json.dump(side, f, indent=1)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('root')
    ap.add_argument('--guns', action='store_true')
    ap.add_argument('--controllers', action='store_true')
    ap.add_argument('--no-sidecar', action='store_true', help="the box gun without its boxgun.json")
    ap.add_argument('--only-right', action='store_true', help='controllers: right.glb only')
    ap.add_argument('--early', action='store_true', help='guns: aa-early.glb too, sorting before boxgun.glb')
    ap.add_argument('--only-box', action='store_true', help='guns: the box gun alone (and aa-early with --early)')
    ap.add_argument('--no-buttons', action='store_true', help='controllers: the sidecars without "buttons"')
    ap.add_argument('--bad-button', action='store_true', help='controllers: the left one\'s "menu" point 5 m off')
    a = ap.parse_args()
    if a.guns:
        write_guns(a.root, not a.no_sidecar, a.early, a.only_box)
    if a.controllers:
        write_controllers(a.root, ['right'] if a.only_right else ['left', 'right'], not a.no_buttons, a.bad_button)
    print('muzzle %.4f %.4f %.4f' % tuple(gun_muzzle(GUN)))


if __name__ == '__main__':
    main()
