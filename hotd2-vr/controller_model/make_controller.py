"""The built-in controller model (hotd2-vr): a low-poly, Quest-3-like Touch controller, made here
from a few surfaces of revolution, baked into core/rend/vr/controller_model.h.

    python -I hotd2-vr/controller_model/make_controller.py core/rend/vr/controller_model.h

Self-made geometry, no Meta data: a handle, a head with a flat face plate, a thumbstick, two
face buttons, a menu button, a trigger and a grip button, each its own part (a range of the
indices) with its centre, where xr_controllers.cpp points a button's label (anchor()).

It is the right controller, in the OpenXR grip space (metres): the origin in the middle of the
handle, -z up the handle towards the thumb, -y forward (the fingers' side, the trigger), +x to
the right. The left one is its mirror in x (drawn so by xr_controllers.cpp), so the right's A
and B are the left's X and Y, and the menu button, where the right controller has its Meta
button, is on the left the menu button (=). The face plate is tilted 30 degrees back from
square to the handle: level with the aim ray, which leaves the grip 60 degrees from the handle.

Standard library only; the output is the same byte for byte on every run.
"""
import math
import sys

POS_SCALE = 1e-05	# metres per position unit, as gun_model.h


def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    length = math.sqrt(dot(a, a))
    return mul(a, 1.0 / length) if length > 1e-12 else (0.0, 0.0, 0.0)


class Frame:
    """An origin and right-handed axes: local (a, b, c) is origin + a ex + b ey + c ez."""
    def __init__(self, origin, ex, ez):
        self.origin, self.ex, self.ez = origin, norm(ex), norm(ez)
        self.ey = cross(self.ez, self.ex)
        assert abs(dot(self.ex, self.ez)) < 1e-9

    def at(self, a, b, c):
        return add(self.origin, add(mul(self.ex, a), add(mul(self.ey, b), mul(self.ez, c))))

    def moved(self, a, b, c):
        return Frame(self.at(a, b, c), self.ex, self.ez)


class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.tri = [], [], []

    def lathe(self, frame, segments, sx, sy, *profiles):
        """Surfaces of revolution about the frame's z: each profile a list of (c, r) from its
        bottom to its top, r across times sx in x and sy in y (an elliptic section). Profiles
        meet in a crease (no shared normals); an r of 0 is a pole."""
        for profile in profiles:
            base = len(self.pos)
            index = []		# per profile point, per segment
            for c, r in profile:
                if r == 0.0:
                    index.append([len(self.pos)] * segments)
                    self.pos.append(frame.at(0.0, 0.0, c))
                    continue
                row = []
                for i in range(segments):
                    t = 2.0 * math.pi * i / segments
                    row.append(len(self.pos))
                    self.pos.append(frame.at(r * sx * math.cos(t), r * sy * math.sin(t), c))
                index.append(row)
            acc = [(0.0, 0.0, 0.0)] * (len(self.pos) - base)
            for j in range(len(profile) - 1):
                for i in range(segments):
                    k = (i + 1) % segments
                    for tri in ((index[j][i], index[j][k], index[j + 1][i]), (index[j][k], index[j + 1][k], index[j + 1][i])):
                        if len(set(tri)) < 3:
                            continue
                        a, b, c = (self.pos[v] for v in tri)
                        n = cross(sub(b, a), sub(c, a))		# outward, area-weighted
                        if dot(n, n) < 1e-24:
                            continue
                        self.tri.append(tri)
                        for v in tri:
                            acc[v - base] = add(acc[v - base], n)
            self.nrm += [norm(n) for n in acc]

    def ellipsoid(self, frame, rx, ry, rz, segments, rings):
        profile = [(-math.cos(math.pi * j / rings) * rz, math.sin(math.pi * j / rings)) for j in range(rings + 1)]
        profile[0], profile[-1] = (-rz, 0.0), (rz, 0.0)
        self.lathe(frame, segments, rx, ry, profile)


def build():
    """The parts, in drawing order: (name, Mesh)."""
    up = (0.0, 0.0, -1.0)
    right = (1.0, 0.0, 0.0)
    tilt = math.radians(30.0)
    face_n = (0.0, math.sin(tilt), -math.cos(tilt))		# the face plate's normal: up the handle, tilted back

    body = Mesh()
    # the handle: 10.4 cm, a bit deeper (y) than wide, its top inside the head
    handle = Frame((0.0, 0.0, 0.0), right, up)
    body.lathe(handle, 18, 0.9, 1.0,
               [(-0.050, 0.0), (-0.050, 0.009)],
               [(-0.050, 0.009), (-0.0485, 0.0135), (-0.045, 0.0160), (-0.035, 0.0172), (-0.010, 0.0180),
                (0.015, 0.0178), (0.035, 0.0170), (0.052, 0.0160), (0.054, 0.0)])
    # the head: a rounded slab 5.8 x 7.2 cm, 2.6 cm thick, over the handle and forward of it
    h = 0.013
    head = Frame((0.0, -0.016, -0.068), right, face_n)
    body.lathe(head, 24, 0.029, 0.036,
               [(-h, 0.0), (-h, 0.55), (-0.85 * h, 0.85), (-0.5 * h, 0.97), (0.0, 1.0), (0.5 * h, 0.98),
                (0.85 * h, 0.93), (h, 0.85)],
               [(h, 0.85), (h, 0.0)])

    # the face plate on the head's flat top; the buttons stand on it (a: across, to the right;
    # b: forward, level with the aim ray)
    top = head.moved(0.0, 0.0, h)
    face = Mesh()
    face.lathe(top, 24, 0.029, 0.036, [(0.0004, 0.82), (0.0004, 0.0)])

    def button(a, b, radius, height, segments):
        m = Mesh()
        m.lathe(top.moved(a, b, 0.0), segments, 1.0, 1.0,
                [(0.0, radius), (height * 0.75, radius), (height * 0.95, radius * 0.85)],
                [(height * 0.95, radius * 0.85), (height, 0.0)])
        return m

    stick = Mesh()
    stick.lathe(top.moved(-0.008, 0.010, 0.0), 14, 1.0, 1.0,
                [(0.0, 0.0036), (0.0050, 0.0036)],
                [(0.0050, 0.0036), (0.0050, 0.0085), (0.0062, 0.0093), (0.0082, 0.0094), (0.0094, 0.0088), (0.0098, 0.0075)],
                [(0.0098, 0.0075), (0.0090, 0.0040), (0.0088, 0.0)])

    # the trigger under the front of the head, for the index finger; the grip button on the
    # handle's inner side, for the middle finger
    trigger = Mesh()
    trigger.ellipsoid(Frame((0.0, -0.026, -0.052), right, (0.0, -0.30, -0.95)), 0.0075, 0.0065, 0.014, 12, 8)
    grip = Mesh()
    grip.ellipsoid(Frame((-0.0150, -0.006, -0.008), right, up), 0.0050, 0.0085, 0.017, 12, 8)

    return [('body', body), ('face', face), ('a', button(0.015, -0.004, 0.0055, 0.0030, 14)),
            ('b', button(0.013, 0.012, 0.0055, 0.0030, 14)), ('menu', button(0.004, -0.018, 0.0036, 0.0022, 12)),
            ('stick', stick), ('trigger', trigger), ('grip', grip)]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    parts = build()
    qpos, qnrm, indices, ranges = [], [], [], []
    for name, mesh in parts:
        base = len(qpos)
        q = [tuple(int(round(v / POS_SCALE)) for v in p) for p in mesh.pos]
        qpos += q
        qnrm += [tuple(max(-127, min(127, int(round(v * 127)))) for v in n) for n in mesh.nrm]
        lo = [min(p[k] for p in q) for k in range(3)]
        hi = [max(p[k] for p in q) for k in range(3)]
        centre = [(lo[k] + hi[k]) * 0.5 * POS_SCALE for k in range(3)]
        ranges.append((name, len(indices), len(mesh.tri) * 3, centre))
        for t in mesh.tri:
            indices += [base + v for v in t]
        print(f'{name}: {len(mesh.tri)} triangles, centre {centre[0]:.4f} {centre[1]:.4f} {centre[2]:.4f}')
    assert len(qpos) < 65536
    assert max(abs(v) for p in qpos for v in p) < 32767
    triangles = len(indices) // 3
    print(f'{len(qpos)} vertices, {triangles} triangles a hand')
    assert triangles <= 1500, 'over the 1500 triangles a hand'

    def rows(values, per_line):
        return ',\n'.join(','.join(str(v) for v in values[i:i + per_line]) for i in range(0, len(values), per_line))

    with open(sys.argv[1], 'w', newline='\n') as f:
        f.write('// Generated by hotd2-vr/controller_model/make_controller.py - do not edit.\n')
        f.write('//\n')
        f.write('// A low-poly controller like a Quest 3 Touch Plus, the right one, in the OpenXR grip space\n')
        f.write('// (metres; -z up the handle, -y forward): self-made, no Meta data. The left one is its\n')
        f.write('// mirror in x. Copyright 2026 mikermak. This file is part of Flycast and is distributed\n')
        f.write('// under the GNU GPL v2 or later.\n')
        f.write('#pragma once\n\nnamespace vr::xr::controllermodel\n{\n\n')
        f.write(f'constexpr float PositionScale = {POS_SCALE}f;\t// metres per position unit\n\n')
        f.write('// ranges of Indices, and the middle of each (its bounds), where its label points\n')
        f.write('struct Part { const char *name; unsigned first, count; float center[3]; };\n')
        f.write('constexpr Part Parts[] = {\n')
        for name, first, count, c in ranges:
            f.write(f'\t{{ "{name}", {first}u, {count}u, {{ {c[0]:.5f}f, {c[1]:.5f}f, {c[2]:.5f}f }} }},\n')
        f.write('};\n\n')
        f.write(f'constexpr unsigned VertexCount = {len(qpos)};\n')
        f.write(f'constexpr unsigned TriangleCount = {triangles};\n')
        f.write('// x, y, z\n')
        f.write('constexpr short Positions[] = {\n' + rows([v for p in qpos for v in p], 30) + '\n};\n')
        f.write('// x, y, z, times 127\n')
        f.write('constexpr signed char Normals[] = {\n' + rows([v for n in qnrm for v in n], 48) + '\n};\n')
        f.write('constexpr unsigned short Indices[] = {\n' + rows(indices, 36) + '\n};\n\n}\n')


if __name__ == '__main__':
    main()
