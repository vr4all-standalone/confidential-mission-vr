"""A small test GLB for the controller models (hotd2-vr, accept.ps1 cm-controllers-rt).

    python -I make-test-glb.py <out.glb>

Writes a glTF 2.0 binary with two meshes, each in its own node: an 8 cm box (12 triangles)
at the origin and a 64-segment open cylinder (128 triangles), 2 cm across and 12 cm long
along -z, moved 6 cm forward by its node's translation (so a loader that skips node
transforms draws it in the wrong place). Both use one material whose base colour is a 2x2
PNG (red, yellow, cyan, magenta). 140 triangles in all. Standard library only.
"""
import json
import math
import struct
import sys
import zlib


def png_2x2():
    pixels = [(255, 40, 40, 255), (255, 230, 40, 255), (40, 230, 255, 255), (255, 40, 230, 255)]
    raw = b''
    for row in range(2):
        raw += b'\x00' + b''.join(bytes(p) for p in pixels[row * 2:row * 2 + 2])

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 2, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def box(half):
    pos, nrm, uv, idx = [], [], [], []
    for axis in range(3):
        for sign in (-1.0, 1.0):
            n = [0.0, 0.0, 0.0]
            n[axis] = sign
            a, b = (axis + 1) % 3, (axis + 2) % 3
            base = len(pos)
            for i, (s, t) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
                p = [0.0, 0.0, 0.0]
                p[axis] = sign * half
                p[a] = s * half
                p[b] = t * half
                pos.append(p)
                nrm.append(n)
                uv.append([(s + 1) / 2, (t + 1) / 2])
            quad = [0, 1, 2, 0, 2, 3] if sign > 0 else [0, 2, 1, 0, 3, 2]
            idx += [base + k for k in quad]
    return pos, nrm, uv, idx


def cylinder(radius, length, segments):
    pos, nrm, uv, idx = [], [], [], []
    for i in range(segments + 1):
        a = 2 * math.pi * i / segments
        c, s = math.cos(a), math.sin(a)
        for k, z in enumerate((0.0, -length)):
            pos.append([radius * c, radius * s, z])
            nrm.append([c, s, 0.0])
            uv.append([i / segments, float(k)])
    for i in range(segments):
        a, b, c, d = 2 * i, 2 * i + 1, 2 * i + 2, 2 * i + 3
        idx += [a, c, b, b, c, d]
    return pos, nrm, uv, idx


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    meshes = [box(0.04), cylinder(0.01, 0.12, 64)]
    binary = bytearray()
    views, accessors, gltf_meshes = [], [], []

    def add_view(data, target):
        while len(binary) % 4:
            binary.append(0)
        views.append({'buffer': 0, 'byteOffset': len(binary), 'byteLength': len(data), 'target': target})
        binary.extend(data)
        return len(views) - 1

    def add_floats(rows, kind, with_bounds=False):
        data = b''.join(struct.pack('<%df' % len(r), *r) for r in rows)
        acc = {'bufferView': add_view(data, 34962), 'componentType': 5126, 'count': len(rows), 'type': kind}
        if with_bounds:
            acc['min'] = [min(r[i] for r in rows) for i in range(3)]
            acc['max'] = [max(r[i] for r in rows) for i in range(3)]
        accessors.append(acc)
        return len(accessors) - 1

    for pos, nrm, uv, idx in meshes:
        attributes = {'POSITION': add_floats(pos, 'VEC3', True), 'NORMAL': add_floats(nrm, 'VEC3'),
                      'TEXCOORD_0': add_floats(uv, 'VEC2')}
        accessors.append({'bufferView': add_view(struct.pack('<%dH' % len(idx), *idx), 34963),
                          'componentType': 5123, 'count': len(idx), 'type': 'SCALAR'})
        gltf_meshes.append({'primitives': [{'attributes': attributes, 'indices': len(accessors) - 1, 'material': 0}]})
    png = png_2x2()
    image_view = add_view(png, None)
    del views[image_view]['target']
    while len(binary) % 4:
        binary.append(0)

    doc = {
        'asset': {'version': '2.0', 'generator': 'hotd2-vr make-test-glb.py'},
        'scene': 0,
        'scenes': [{'nodes': [0, 1]}],
        'nodes': [{'mesh': 0}, {'mesh': 1, 'translation': [0.0, 0.0, -0.06]}],
        'meshes': gltf_meshes,
        'materials': [{'pbrMetallicRoughness': {'baseColorTexture': {'index': 0}, 'metallicFactor': 0.0}}],
        'textures': [{'source': 0, 'sampler': 0}],
        'samplers': [{'magFilter': 9728, 'minFilter': 9728}],
        'images': [{'bufferView': image_view, 'mimeType': 'image/png'}],
        'buffers': [{'byteLength': len(binary)}],
        'bufferViews': views,
        'accessors': accessors,
    }
    text = json.dumps(doc, separators=(',', ':')).encode('utf-8')
    text += b' ' * (-len(text) % 4)
    body = struct.pack('<II', len(text), 0x4E4F534A) + text + struct.pack('<II', len(binary), 0x004E4942) + bytes(binary)
    with open(sys.argv[1], 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + len(body)) + body)
    triangles = sum(len(m[3]) for m in meshes) // 3
    print(f'{sys.argv[1]}: {triangles} triangles, {len(png)}-byte PNG, {12 + len(body)} bytes')


if __name__ == '__main__':
    main()
