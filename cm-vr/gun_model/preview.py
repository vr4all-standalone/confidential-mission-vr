"""Draw the gun parts from three sides (painter's algorithm) to check parts and orientation."""
import sys
import numpy as np
from PIL import Image, ImageDraw
from glb import load

def render(parts, axes, size=700, colors=None, title=''):
    a, b, depth = axes  # screen x, screen y (up), depth (towards viewer)
    allp = np.vstack([p['pos'] for p in parts])
    lo, hi = allp.min(0), allp.max(0)
    span = max(hi[a] - lo[a], hi[b] - lo[b]) * 1.1
    cx, cy = (hi[a] + lo[a]) / 2, (hi[b] + lo[b]) / 2
    img = Image.new('RGB', (size, size), (30, 30, 34))
    dr = ImageDraw.Draw(img)
    tris = []
    light = np.zeros(3); light[depth] = 0.6; light[b] = 0.8; light /= np.linalg.norm(light)
    for k, p in enumerate(parts):
        col = np.array(colors[k] if colors else (200, 200, 200), dtype=float)
        v = p['pos'][p['tri']]
        n = np.cross(v[:, 1] - v[:, 0], v[:, 2] - v[:, 0])
        n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12
        shade = 0.35 + 0.65 * np.abs(n @ light)
        z = v[:, :, depth].mean(1)
        for t in range(len(v)):
            tris.append((z[t], v[t], col * shade[t]))
    tris.sort(key=lambda t: t[0])
    for z, v, c in tris:
        pts = [((q[a] - cx) / span * size + size / 2, size / 2 - (q[b] - cy) / span * size) for q in v]
        dr.polygon(pts, fill=tuple(int(x) for x in c))
    dr.text((8, 8), title, fill=(255, 255, 255))
    return img

if __name__ == '__main__':
    parts, j = load(sys.argv[1])
    for p in parts:
        print(p['name'], len(p['tri']), 'tris', p['pos'].min(0).round(3), p['pos'].max(0).round(3))
    colors = [(220, 60, 50), (80, 160, 230), (240, 220, 60), (60, 220, 120)]
    views = {'side_xy': (0, 1, 2), 'top_xz': (0, 2, 1), 'front_zy': (2, 1, 0)}
    imgs = [render(parts, ax, colors=colors, title=name) for name, ax in views.items()]
    sheet = Image.new('RGB', (700 * 3, 700))
    for i, im in enumerate(imgs):
        sheet.paste(im, (700 * i, 0))
    sheet.save(sys.argv[2])
    print('legend: ' + ', '.join(f"{p['name']}={c}" for p, c in zip(parts, colors)))
