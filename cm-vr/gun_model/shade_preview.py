"""Preview of the baked gun with the headset's plastic shading (approximation, per triangle)."""
import re, sys
import numpy as np
from PIL import Image, ImageDraw

src = open(sys.argv[1]).read()
def arr(name, dt):
    body = re.search(name + r'\[\] = \{(.*?)\};', src, re.S).group(1)
    return np.array([int(x) for x in body.replace('\n', '').split(',') if x.strip()], dtype=dt)
pos = arr('Positions', np.float64).reshape(-1, 3) * 1e-5
nrm = arr('Normals', np.float64).reshape(-1, 3) / 127
idx = arr('Indices', np.int64).reshape(-1, 3)
parts = re.findall(r'\{ "(\w+)", (\d+)u, (\d+)u \}', src)
mats = {'hardsurfaces': ((0.78, 0.05, 0.04), 0.55, 60), 'softsurfaces': ((0.56, 0.03, 0.03), 0.35, 28),
        'screws': ((0.16, 0.16, 0.18), 0.8, 80), 'lens': ((0.06, 0.01, 0.01), 1.0, 140)}
key = np.array([0.32, 0.86, 0.40]); key /= np.linalg.norm(key)
fill = np.array([-0.45, -0.75, -0.48]); fill /= np.linalg.norm(fill)

def view(yaw, pitch, size=640):
    cy, sy, cp, sp = np.cos(yaw), np.sin(yaw), np.cos(pitch), np.sin(pitch)
    R = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]) @ np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]])
    eye_dir = R @ np.array([0, 0, 1.0])   # from the model towards the eye
    eye = eye_dir * 0.6
    img = Image.new('RGB', (size, size), (24, 22, 26)); dr = ImageDraw.Draw(img)
    tris = []
    for name, first, count in parts:
        col, spec, shin = mats[name]
        t = idx[int(first) // 3:(int(first) + int(count)) // 3]
        v = pos[t]; n = nrm[t].mean(1); n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
        c = v.mean(1)
        vv = eye - c; vv /= np.linalg.norm(vv, axis=1, keepdims=True)
        flip = (n * vv).sum(1) < 0; n[flip] *= -1
        dif = np.clip(n @ key, 0, None); back = np.clip(n @ fill, 0, None)
        h = key + vv; h /= np.linalg.norm(h, axis=1, keepdims=True)
        hl = np.clip((n * h).sum(1), 0, None) ** shin * spec
        rim = (1 - np.clip((n * vv).sum(1), 0, None)) ** 3
        rgb = np.array(col)[None] * (0.22 + 0.78 * dif + 0.25 * back)[:, None] + hl[:, None] + rim[:, None] * (0.18 * np.array(col)[None] + 0.1 * spec)
        cam = (v - 0) @ R   # into view space
        for k in range(len(t)):
            tris.append((cam[k, :, 2].mean(), cam[k], np.clip(rgb[k], 0, 1)))
    tris.sort(key=lambda x: x[0])
    for z, q, c in tris:
        pts = [(p[0] / 0.3 * size + size / 2, size / 2 - (p[1] + 0.01) / 0.3 * size) for p in q]
        dr.polygon(pts, fill=tuple(int(255 * x) for x in c))
    return img

imgs = [view(np.radians(-70), np.radians(-12)), view(np.radians(-120), np.radians(-20)), view(np.radians(170), np.radians(-25))]
sheet = Image.new('RGB', (640 * 3, 640))
for i, im in enumerate(imgs): sheet.paste(im, (640 * i, 0))
sheet.save(sys.argv[2])
