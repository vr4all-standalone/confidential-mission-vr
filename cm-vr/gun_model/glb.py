"""Minimal glTF 2.0 (.glb) reader for the gun model: meshes with world transforms."""
import json, struct
import numpy as np

def load(path):
    d = open(path, 'rb').read()
    jl = struct.unpack_from('<I', d, 12)[0]
    j = json.loads(d[20:20 + jl])
    bin_off = 20 + jl + 8

    def acc(i):
        a = j['accessors'][i]
        bv = j['bufferViews'][a['bufferView']]
        comp = {5126: 'f4', 5123: 'u2', 5125: 'u4', 5121: 'u1'}[a['componentType']]
        n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        off = bin_off + bv.get('byteOffset', 0) + a.get('byteOffset', 0)
        dt = np.dtype(comp)
        stride = bv.get('byteStride', 0) or dt.itemsize * n
        raw = np.frombuffer(d, dtype=np.uint8, offset=off, count=stride * (a['count'] - 1) + dt.itemsize * n)
        rows = np.lib.stride_tricks.as_strided(raw, shape=(a['count'], dt.itemsize * n), strides=(stride, 1))
        return np.frombuffer(rows.copy().tobytes(), dtype=comp).reshape(a['count'], n)

    def local(node):
        if 'matrix' in node:
            return np.array(node['matrix'], dtype=np.float64).reshape(4, 4).T
        m = np.eye(4)
        if 'scale' in node:
            m = np.diag(list(node['scale']) + [1]) @ m
        if 'rotation' in node:
            x, y, z, w = node['rotation']
            r = np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                          [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                          [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])
            rm = np.eye(4); rm[:3, :3] = r; m = rm @ m
        if 'translation' in node:
            t = np.eye(4); t[:3, 3] = node['translation']; m = t @ m
        return m

    parts = []
    def walk(ni, parent):
        node = j['nodes'][ni]
        world = parent @ local(node)
        if 'mesh' in node:
            for p in j['meshes'][node['mesh']]['primitives']:
                pos = acc(p['attributes']['POSITION']).astype(np.float64)
                nrm = acc(p['attributes']['NORMAL']).astype(np.float64)
                idx = acc(p['indices']).reshape(-1).astype(np.int64)
                pos = (world[:3, :3] @ pos.T).T + world[:3, 3]
                nm = np.linalg.inv(world[:3, :3]).T
                nrm = (nm @ nrm.T).T
                nrm /= np.linalg.norm(nrm, axis=1, keepdims=True) + 1e-12
                parts.append({'name': j['nodes'][parent_names.get(ni, ni)]['name'] if False else node.get('name'),
                              'mesh': node['mesh'], 'pos': pos, 'nrm': nrm, 'tri': idx.reshape(-1, 3)})
        for c in node.get('children', []):
            walk(c, world)
    parent_names = {}
    for root in j['scenes'][j.get('scene', 0)]['nodes']:
        walk(root, np.eye(4))
    # name each part after its parent group (softsurfaces, hardsurfaces, screws, lens)
    names = {}
    for i, n in enumerate(j['nodes']):
        for c in n.get('children', []):
            names[c] = n.get('name', '')
    for p in parts:
        for i, n in enumerate(j['nodes']):
            if n.get('mesh') == p['mesh']:
                p['name'] = names.get(i, n.get('name'))
    return parts, j
