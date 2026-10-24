r"""
Test fixtures for the player's own models (Task 14b; accept.ps1 cm-own-gun, cm-own-controllers): self-made
boxes written at test time, so no third-party model is ever in the repo.

  python -I make-own-models.py <root> [--guns] [--controllers] [--no-sidecar boxgun] [--only-right]

<root>\guns\boxgun.glb + boxgun.json: a 3-part box gun (grip, body, barrel), each its own base colour (red,
green, blue), the barrel (a muzzle brake, wider and taller than the body: it shows from behind) also textured
(a 4x4 PNG, light checks), in made-up units (2 long) with the barrel
along +x and up +z (so the app must turn and scale it). <root>\guns\zz-big.glb + zz-big.json: a grid of
204,800 triangles, over the app's 200,000 (refused). <root>\controllers\left.glb, right.glb + .json: a box
controller in cm (two parts, dark grey and orange), its aim node 2 cm up, 6 cm forward, tilted 30 degrees.

Prints the box gun's muzzle in gun space (barrel -z, y up, metres, the grip's top the origin) as the app
should find it from the sidecar: "muzzle <x> <y> <z>".
"""
import argparse
import json
import math
import os
import struct
import zlib


def png(width, height, pixel):
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


def write_guns(root, sidecar):
    os.makedirs(os.path.join(root, 'guns'), exist_ok=True)
    g = Glb()
    g.primitive(box([-0.3, -0.15, -1.2], [0.1, 0.15, 0.0]), RED)
    g.primitive(box([-0.5, -0.17, 0.0], [1.0, 0.17, 0.45]), GREEN)
    checks = png(4, 4, lambda x, y: (255, 255, 255, 255) if (x + y) % 2 else (215, 215, 215, 255))
    g.primitive(box([1.0, -0.25, -0.05], [1.5, 0.25, 0.55]), BLUE, checks)
    g.write(os.path.join(root, 'guns', 'boxgun.glb'))
    if sidecar:
        with open(os.path.join(root, 'guns', 'boxgun.json'), 'w') as f:
            json.dump(GUN, f, indent=1)
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


def write_controllers(root, hands):
    os.makedirs(os.path.join(root, 'controllers'), exist_ok=True)
    half = math.radians(30) / 2
    for hand in hands:
        c = Glb()
        c.primitive(box([-3.0, -2.0, -6.0], [3.0, 2.0, 6.0]), (0.18, 0.18, 0.2))
        c.primitive(box([-3.5, 2.0, -7.0], [3.5, 3.0, -2.0]), (1.0, 0.45, 0.05))
        c.write(os.path.join(root, 'controllers', hand + '.glb'))
        with open(os.path.join(root, 'controllers', hand + '.json'), 'w') as f:
            json.dump({'aim': {'position': [0.0, 2.0, -6.0], 'rotation': [math.sin(half), 0.0, 0.0, math.cos(half)]},
                       'scale': 0.01}, f, indent=1)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('root')
    ap.add_argument('--guns', action='store_true')
    ap.add_argument('--controllers', action='store_true')
    ap.add_argument('--no-sidecar', action='store_true', help="the box gun without its boxgun.json")
    ap.add_argument('--only-right', action='store_true', help='controllers: right.glb only')
    a = ap.parse_args()
    if a.guns:
        write_guns(a.root, not a.no_sidecar)
    if a.controllers:
        write_controllers(a.root, ['right'] if a.only_right else ['left', 'right'])
    print('muzzle %.4f %.4f %.4f' % tuple(gun_muzzle(GUN)))


if __name__ == '__main__':
    main()
