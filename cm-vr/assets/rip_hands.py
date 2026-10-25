"""Makes the headset's hands and pistol (hands.bin) from a Flycast rip of The House of the Dead 2.

The models come from your own copy of the game, as the emulator drew them: run the PC
build to the game over scene with a rip request (cm-vr/rip-hands.ps1 does it all), then

    py rip_hands.py <rip dir> <hands.bin> [--pass N] [--preview preview.png]

Nothing of the game is in this repository, and hands.bin is not to be shared either.

What it takes from that frame (the agent on his knees, his pistol in his right hand): the
pistol, the right hand around its grip and the open left hand, with their textures. They
are lifted back into 3D with the game's focal length (from the rip), the pistol is put
upright along its barrel, its slide is cut free along the line painted on its sides (with
a cap and a barrel underneath, for when it moves back), and everything is written in
metres for xr_hands.cpp:

    gun space    the pistol upright, barrel along -z, y up, origin in the fist (the hand
                 around the grip): that point goes where the controller's grip is
    hand space   the open left hand, fingers along -z, palm towards +x, thumb up, origin
                 just off the palm (where the controller's grip is)

Copyright 2026 mikermak. Part of the hotd2-vr fork of Flycast, GPL v2 or later.
"""
import argparse
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from riplib import (Canvas, Pass, TSP_CLAMP_U, TSP_CLAMP_V, TSP_FLIP_U, TSP_FLIP_V, TSP_IGNORE_TEX_A,
                    decode_texture, strip_triangles)

# HOTD2 (PAL, MK-5100250), the game over scene: textures (VRAM addresses) of the agent's parts.
HAND_TEXTURES = (0x5fa380, 0x5fc380)
CUFF_TEXTURES = (0x601380,)
GUN_TEXTURES = (0x5fe380,)

# Metres per game unit for the models: their open hand comes out 16 cm from wrist to
# fingertip and the pistol 21 cm long (the game's pistol is large next to its hands).
METRES = 0.095
SLIDE_TRAVEL = 0.16     # of the pistol's length
PALM_OFFSET = 0.2       # game units: the controller's grip is this far off the open palm
GRAB_SHIFT = (-0.17, 0.02)  # the open hand on the slide: left and up, of the pistol's length

MESH_FRAME, MESH_SLIDE, MESH_GUN_HAND, MESH_OPEN_HAND = range(4)
MESH_NAMES = ('frame', 'slide', 'gun hand', 'open hand')


class Tri:
    """A triangle: positions (3,3), uvs (3,2), colour (rgba 0..255), texture key."""
    __slots__ = ('p', 'uv', 'col', 'tex')

    def __init__(self, p, uv, col, tex):
        self.p, self.uv, self.col, self.tex = p, uv, col, tex


def texture_key(poly):
    tsp = poly.tsp
    flags = ((tsp & TSP_CLAMP_U) and 1) | ((tsp & TSP_CLAMP_V) and 2) | ((tsp & TSP_FLIP_U) and 4) | ((tsp & TSP_FLIP_V) and 8)
    if not tsp & TSP_IGNORE_TEX_A:
        flags |= 16     # alpha test
    return (poly.tcw, tsp & 0x3F, flags)


def triangles(ps, polys):
    """Triangles of the polygons in game eye space, each wound so its normal faces out:
    by the winding the game culled with (ISP cull mode 2/3), the sign settled by a vote
    over all of them (outwards from the middle), else outwards from the middle."""
    out, votes = [], []
    centre = np.concatenate([ps.eye(p) for p in polys]).mean(0)
    for poly in polys:
        e = ps.eye(poly)
        sx, sy = poly.v['x'], poly.v['y']
        key = texture_key(poly) if poly.textured else None
        for a, b, c in strip_triangles(len(e)):
            idx = [a, b, c]
            n = np.cross(e[b] - e[a], e[c] - e[a])
            if np.linalg.norm(n) < 1e-12:
                continue
            area = (sx[b] - sx[a]) * (sy[c] - sy[a]) - (sx[c] - sx[a]) * (sy[b] - sy[a])
            sign = (1 if area > 0 else -1) * {2: 1, 3: -1}.get(poly.cull, 0)
            outward = 1 if n @ (e[idx].mean(0) - centre) > 0 else -1
            if sign:
                votes.append(sign * outward)
            t = Tri(e[idx], np.stack([poly.v['u'][idx], poly.v['v'][idx]], 1), (255, 255, 255, 255), key)
            out.append((t, sign, outward))
    convention = 1 if sum(votes) >= 0 else -1
    tris = []
    for t, sign, outward in out:
        if (sign * convention if sign else outward) < 0:
            t.p, t.uv = t.p[[0, 2, 1]], t.uv[[0, 2, 1]]
        tris.append(t)
    return tris


def clusters(ps, polys):
    """Polygons grouped into pieces that share vertices."""
    parent = list(range(len(polys)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    owner = {}
    for i, p in enumerate(polys):
        for e in ps.eye(p):
            k = tuple(np.round(e, 3))
            if k in owner:
                parent[find(i)] = find(owner[k])
            else:
                owner[k] = i
    groups = {}
    for i in range(len(polys)):
        groups.setdefault(find(i), []).append(polys[i])
    return list(groups.values())


def centre(ps, polys):
    return np.concatenate([ps.eye(p) for p in polys]).mean(0)


def surface_centre(tris):
    a = np.array([np.linalg.norm(np.cross(t.p[1] - t.p[0], t.p[2] - t.p[0])) for t in tris])
    c = np.array([t.p.mean(0) for t in tris])
    return (c * a[:, None]).sum(0) / a.sum()


def find_parts(ps):
    pick = lambda texs: [p for p in ps.polys if len(p.v) >= 3 and p.tex in texs and np.isfinite(p.v['z']).all() and (p.v['z'] > 0).all()]
    gun = pick(GUN_TEXTURES)
    hands = clusters(ps, pick(HAND_TEXTURES))
    if not gun or len(hands) < 2:
        raise SystemExit(f'pass {ps.n}: no pistol and two hands here')
    gc = centre(ps, gun)
    hands.sort(key=lambda c: np.linalg.norm(centre(ps, c) - gc))
    gun_hand, open_hand = hands[0], hands[1]
    gun_cuff, open_cuff = [], []
    for c in clusters(ps, pick(CUFF_TEXTURES)):
        cc = centre(ps, c)
        (gun_cuff if np.linalg.norm(cc - centre(ps, gun_hand)) < np.linalg.norm(cc - centre(ps, open_hand)) else open_cuff).extend(c)
    return gun, gun_hand, gun_cuff, open_hand, open_cuff


def gun_axes(gun_tris):
    """Rows x, y, z of gun space in eye space: z from the muzzle back along the barrel,
    y up (away from the grip)."""
    pts = np.concatenate([t.p for t in gun_tris])
    mid = pts.mean(0)
    _, _, ax = np.linalg.svd(pts - mid)
    # the barrel: the direction of the longest edges (the slide's)
    edges = []
    for t in gun_tris:
        for i, j in ((0, 1), (1, 2), (2, 0)):
            d = t.p[j] - t.p[i]
            edges.append(d if d @ ax[0] >= 0 else -d)
    edges = np.array(edges)
    lengths = np.linalg.norm(edges, axis=1)
    long = edges[lengths > 0.7 * lengths.max()]
    z = long.sum(0)
    z /= np.linalg.norm(z)
    # down: the grip, the far side of the second axis
    y = ax[1] - (ax[1] @ z) * z
    y /= np.linalg.norm(y)
    side = (pts - mid) @ y
    if side.max() > -side.min():
        y = -y
    # back: where the bottom of the grip is
    low = pts[((pts - mid) @ y) < ((pts - mid) @ y).min() * 0.8]
    if ((low - mid) @ z).mean() < 0:
        z = -z
    x = np.cross(y, z)
    return np.stack([x, y, z])


def open_hand_axes(hand_tris, cuff_tris):
    """Rows x, y, z of hand space in eye space (fingers -z, palm +x, thumb +y, for a left
    hand), and the wrist."""
    hp = np.concatenate([t.p for t in hand_tris])
    cp = np.concatenate([t.p for t in cuff_tris])
    hc = hp.mean(0)
    d = np.linalg.norm(cp - hc, axis=1)
    wrist = cp[d <= np.median(d)].mean(0)      # the cuff's hand-side ring
    f = hc - wrist
    f /= np.linalg.norm(f)
    _, _, ax = np.linalg.svd(hp - hc)
    n = ax[2] - (ax[2] @ f) * f
    n /= np.linalg.norm(n)
    # the fingers curl towards the palm: their tips are on the palm side of their middles
    along = (hp - wrist) @ f
    length = along.max()
    tips = hp[along > 0.9 * length]
    middles = hp[(along > 0.55 * length) & (along < 0.75 * length)]
    palm = n if (tips @ n).mean() > (middles @ n).mean() else -n
    thumb = np.cross(palm, f)    # left hand: thumb = palm x fingers
    return np.stack([palm, thumb, -f]), wrist


def to_space(tris, origin, axes, scale):
    out = []
    for t in tris:
        out.append(Tri((t.p - origin) @ axes.T * scale, t.uv, t.col, t.tex))
    return out


def clip(tris, y):
    """Splits triangles at the plane at height y: (below, above)."""
    below, above = [], []
    for t in tris:
        d = t.p[:, 1] - y
        if (d >= -1e-6).all():
            above.append(t)
            continue
        if (d <= 1e-6).all():
            below.append(t)
            continue
        for keep, out in ((lambda s: s <= 0, below), (lambda s: s >= 0, above)):
            poly = []
            for i in range(3):
                j = (i + 1) % 3
                if keep(d[i]):
                    poly.append((t.p[i], t.uv[i]))
                if (d[i] < 0) != (d[j] < 0) and d[i] != 0 and d[j] != 0:
                    s = d[i] / (d[i] - d[j])
                    poly.append((t.p[i] + s * (t.p[j] - t.p[i]), t.uv[i] + s * (t.uv[j] - t.uv[i])))
            for k in range(1, len(poly) - 1):
                tri = [poly[0], poly[k], poly[k + 1]]
                p = np.array([v[0] for v in tri])
                if np.linalg.norm(np.cross(p[1] - p[0], p[2] - p[0])) > 1e-12:
                    out.append(Tri(p, np.array([v[1] for v in tri]), t.col, t.tex))
    return below, above


def box(lo, hi, col, faces='all'):
    """An untextured box, outward wound."""
    x0, y0, z0 = lo
    x1, y1, z1 = hi
    quads = {
        'top': [(x0, y1, z0), (x0, y1, z1), (x1, y1, z1), (x1, y1, z0)],
        'bottom': [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)],
        'front': [(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)],
        'back': [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)],
        'left': [(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)],
        'right': [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)],
    }
    out = []
    for name, q in quads.items():
        if faces != 'all' and name not in faces:
            continue
        q = np.array(q, np.float64)
        for a, b, c in ((0, 1, 2), (0, 2, 3)):
            out.append(Tri(q[[a, b, c]], np.zeros((3, 2)), col, None))
    return out


def smooth_normals(tris, crease=50.0):
    """Per corner normals: the faces around a point within `crease` degrees of each other."""
    fn = []
    for t in tris:
        n = np.cross(t.p[1] - t.p[0], t.p[2] - t.p[0])
        fn.append(n / np.linalg.norm(n))
    around = {}
    for i, t in enumerate(tris):
        for p in t.p:
            around.setdefault(tuple(np.round(p, 5)), []).append(i)
    cos = np.cos(np.radians(crease))
    normals = []
    for i, t in enumerate(tris):
        corner = []
        for p in t.p:
            n = sum(fn[j] for j in around[tuple(np.round(p, 5))] if fn[j] @ fn[i] >= cos)
            corner.append(n / np.linalg.norm(n))
        normals.append(np.array(corner))
    return normals


def bounds(tris):
    p = np.concatenate([t.p for t in tris])
    return p.min(0), p.max(0)


def build(ps):
    gun, gun_hand, gun_cuff, open_hand, open_cuff = find_parts(ps)
    gun_t = triangles(ps, gun)
    gun_hand_t = triangles(ps, gun_hand + gun_cuff)
    open_t = triangles(ps, open_hand + open_cuff)

    # gun space: upright along the barrel, origin in the fist around the grip
    axes = gun_axes(gun_t)
    fist = surface_centre(triangles(ps, gun_hand))
    gun_m = to_space(gun_t, fist, axes, METRES)
    hand_m = to_space(gun_hand_t, fist, axes, METRES)
    lo, hi = bounds(gun_m)
    length = hi[2] - lo[2]
    # the muzzle: the middle of the front-most face (the slide's nose)
    front = np.concatenate([t.p for t in gun_m])
    front = front[front[:, 2] < lo[2] + 0.002]
    muzzle = np.array([(front[:, 0].min() + front[:, 0].max()) / 2, (front[:, 1].min() + front[:, 1].max()) / 2, lo[2]])
    # the slide: above the bottom of its nose, where the line on its sides is
    cut = front[:, 1].min()
    frame_m, slide_m = clip(gun_m, cut)
    s_lo, s_hi = bounds(slide_m)
    f_lo, f_hi = bounds([t for t in frame_m if t.p[:, 1].min() > cut - 0.01 * length] or frame_m)
    body_front = np.concatenate([t.p for t in slide_m])
    body_front = body_front[body_front[:, 2] > lo[2] + 0.002][:, 2].min()   # behind the nose
    dark, darker = (70, 70, 74, 255), (38, 38, 42, 255)
    gap = 0.0004
    inner = (s_hi[0] - s_lo[0]) * 0.03
    # under the slide and on top of the frame: closed off, seen once the slide moves
    slide_m += box((s_lo[0] + inner, cut + gap, body_front), (s_hi[0] - inner, cut + gap, s_hi[2]), dark, ('bottom',))
    frame_m += box((s_lo[0] + inner, cut - gap, body_front), (s_hi[0] - inner, cut - gap, s_hi[2] - 0.15 * length), dark, ('top',))
    # the barrel, which stays when the slide goes back
    r = (s_hi[0] - s_lo[0]) * 0.22
    frame_m += box((muzzle[0] - r, muzzle[1] - r, lo[2] + 0.001), (muzzle[0] + r, muzzle[1] + r, lo[2] + 0.35 * length), darker)
    travel = SLIDE_TRAVEL * length
    grab = np.array([muzzle[0], (cut + s_hi[1]) / 2, s_hi[2] - 0.12 * length])

    # hand space: the open left hand, origin just off its palm
    h_axes, wrist = open_hand_axes(triangles(ps, open_hand), triangles(ps, open_cuff))
    palm = surface_centre(triangles(ps, open_hand)) + h_axes[0] * PALM_OFFSET
    open_m = to_space(open_t, palm, h_axes, METRES)

    # the open hand on the slide (gun space, slide at rest): palm down on its rear, fingers
    # over to the right side, thumb back towards the shooter
    on_slide = np.stack([[0, -1, 0], [0, 0, 1], [-1, 0, 0]], 0).astype(np.float64)   # rows: hand x, y, z in gun space
    grab_pose = np.eye(4)
    grab_pose[:3, :3] = on_slide.T
    # the palm (PALM_OFFSET above the origin, the hand's +x being down) on the slide's top,
    # the hand to the left so the fingers reach over
    grab_pose[:3, 3] = (grab[0] + GRAB_SHIFT[0] * length, s_hi[1] + GRAB_SHIFT[1] * length - PALM_OFFSET * METRES, grab[2])

    meshes = [frame_m, slide_m, hand_m, open_m]
    return dict(meshes=meshes, muzzle=muzzle, grab=grab, travel=travel, grab_pose=grab_pose,
                length=length, hand=np.ptp(np.concatenate([t.p for t in open_m]), axis=0))


def write(path, ps, model):
    keys = []
    for mesh in model['meshes']:
        for t in mesh:
            if t.tex is not None and t.tex not in keys:
                keys.append(t.tex)
    images = [decode_texture(ps, tcw, tsp) for tcw, tsp, _ in keys]
    with open(path, 'wb') as f:
        f.write(struct.pack('<4sI', b'HND1', 1))
        f.write(struct.pack('<3f', *model['muzzle']))
        f.write(struct.pack('<3f', *model['grab']))
        f.write(struct.pack('<f', model['travel']))
        f.write(struct.pack('<16f', *model['grab_pose'].T.flatten()))   # column major
        # textures: index 0 is plain white (untextured parts)
        f.write(struct.pack('<I', len(keys) + 1))
        f.write(struct.pack('<3I', 1, 1, 0) + bytes([255, 255, 255, 255]))
        for (tcw, tsp, flags), img in zip(keys, images):
            if not flags & 16:
                img = img.copy()
                img[..., 3] = 255
            f.write(struct.pack('<3I', img.shape[1], img.shape[0], flags))
            f.write(np.ascontiguousarray(img).tobytes())
        f.write(struct.pack('<I', len(model['meshes'])))
        for mesh in model['meshes']:
            normals = smooth_normals(mesh)
            groups = {}
            for t, n in zip(mesh, normals):
                groups.setdefault(0 if t.tex is None else keys.index(t.tex) + 1, []).append((t, n))
            f.write(struct.pack('<I', len(groups)))
            for tex, items in sorted(groups.items()):
                verts = bytearray()
                for t, n in items:
                    for k in range(3):
                        verts += struct.pack('<8f', *t.p[k], *n[k], *t.uv[k]) + bytes(t.col)
                f.write(struct.pack('<2I', tex, len(items) * 3))
                f.write(verts)
    return keys, images


def preview(path, ps, model, keys, images):
    """Views of the assembled model: the pistol in the hand with the slide at rest and back,
    the open hand, and the open hand racking the slide."""
    from PIL import Image, ImageDraw
    tex = {k: img.astype(np.float32) / 255 for k, img in zip(keys, images)}
    frame_m, slide_m, hand_m, open_m = model['meshes']
    gp = model['grab_pose']
    on_slide = [Tri(t.p @ gp[:3, :3].T + gp[:3, 3], t.uv, t.col, t.tex) for t in open_m]
    back = lambda tris, d: [Tri(t.p + np.array([0, 0, d]), t.uv, t.col, t.tex) for t in tris]
    travel = model['travel']
    scenes = [
        ('in the hand', frame_m + slide_m + hand_m, (35, 15)),
        ('in the hand', frame_m + slide_m + hand_m, (-120, 10)),
        ('slide back', frame_m + back(slide_m, travel) + hand_m + back(on_slide, travel), (40, 25)),
        ('slide back', frame_m + back(slide_m, travel) + back(on_slide, travel), (-60, 30)),
        ('racking, from the shooter', frame_m + back(slide_m, travel * 0.5) + hand_m + back(on_slide, travel * 0.5), (160, 25)),
        ('open left hand', open_m, (60, 10)),
    ]
    size = 360
    sheet = Image.new('RGB', (size * 3, size * 2))
    for i, (name, tris, (yaw, pitch)) in enumerate(scenes):
        cy, sy = np.cos(np.radians(yaw)), np.sin(np.radians(yaw))
        cp, sp = np.cos(np.radians(pitch)), np.sin(np.radians(pitch))
        rot = np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]]) @ np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
        pts = np.concatenate([t.p for t in tris])
        mid = (pts.min(0) + pts.max(0)) / 2
        radius = np.linalg.norm(pts - mid, axis=1).max()
        dist = radius * 3
        focal = size * 0.5 / (radius * 1.1 / dist)
        light = np.array([0.32, 0.86, 0.40])
        cv = Canvas(size, size)
        normals = smooth_normals(tris)
        for t, n in zip(tris, normals):
            p = (t.p - mid) @ rot.T
            z = dist - p[:, 2]
            s = np.stack([size / 2 + p[:, 0] * focal / z, size / 2 - p[:, 1] * focal / z], 1)
            nn = n @ rot.T
            view = -(p - np.array([0, 0, dist]))
            flip = np.sign(np.einsum('ij,ij->i', nn, view))
            shade = 0.3 + 0.8 * np.clip((nn * flip[:, None]) @ (rot @ light), 0, 1)
            col = np.array(t.col, np.float32) / 255 * np.concatenate([shade[:, None].repeat(3, 1), np.ones((3, 1))], 1)
            img = tex.get(t.tex) if t.tex is not None else None
            cv.triangle(s, 1 / z, t.uv, col.astype(np.float32), img, t.tex[2] if t.tex else 0, bool(t.tex and t.tex[2] & 16))
        im = cv.image()
        ImageDraw.Draw(im).text((6, 6), name, fill='yellow')
        sheet.paste(im, ((i % 3) * size, (i // 3) * size))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('ripdir')
    ap.add_argument('out')
    ap.add_argument('--pass', dest='pass_', type=int, help='the pass to use (default: the one with the most of the pistol)')
    ap.add_argument('--preview', help='also draw the model to this PNG')
    args = ap.parse_args()
    numbers = Pass.numbers(args.ripdir)
    if not numbers:
        raise SystemExit(f'no rip in {args.ripdir}')
    if args.pass_ is None:
        def gun_polys(n):
            return sum(p.tex in GUN_TEXTURES for p in Pass(args.ripdir, n).polys)
        args.pass_ = max(numbers, key=gun_polys)
    ps = Pass(args.ripdir, args.pass_)
    model = build(ps)
    keys, images = write(args.out, ps, model)
    print(f'{args.out}: pass {ps.n}, pistol {model["length"] * 100:.1f} cm, open hand {model["hand"][2] * 100:.1f} cm long, '
          f'{len(keys)} textures, triangles ' + ', '.join(f'{name} {len(m)}' for name, m in zip(MESH_NAMES, model['meshes'])))
    if args.preview:
        try:
            preview(args.preview, ps, model, keys, images)
            print(f'{args.preview}: preview')
        except ImportError:
            print('(no preview: needs pillow)')


if __name__ == '__main__':
    main()
