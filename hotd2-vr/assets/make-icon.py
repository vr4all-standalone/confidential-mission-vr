"""
Draws the launcher icon of VR Light Gun Hub (the Quest build, "vr" build type): a gun sight
(a ring with four ticks) around the initials VR, from plain shapes only. No font, no game or
company art. Writes the five legacy densities into the vr source set, where they override the
main source set's ic_launcher.png at the same sizes:

    python hotd2-vr\\assets\\make-icon.py [preview.png]

Needs Pillow (requirements.txt). With an argument it also saves all five side by side,
upscaled without smoothing, to look at.
"""
import math
import pathlib
import sys

from PIL import Image, ImageDraw

ROOT = pathlib.Path(__file__).resolve().parents[2]
RES = ROOT / "shell" / "android-studio" / "flycast" / "src" / "vr" / "res"
# as src/main/res/drawable-*/ic_launcher.png
SIZES = {"ldpi": 36, "mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144}

BIG = 1152                    # drawn once at this size, then scaled down (a multiple of 36, 48, 72 and 144)
BACK = (16, 26, 46, 255)      # deep navy
EDGE = (44, 64, 98, 255)      # a lighter rim, so the tile stands out on a dark home screen
SIGHT = (255, 128, 24, 255)   # amber
LETTERS = (255, 255, 255, 255)


def u(f):
    """A fraction of the icon's size in pixels."""
    return round(f * BIG)


def letter_v(d, x0, y0, w, h, t):
    """A V in the box (x0, y0, w, h) with strokes t wide (measured across the arm)."""
    cx, y1 = x0 + w / 2, y0 + h
    # the arms' horizontal width for a stroke t thick at their slant
    slant = math.atan2(w / 2, h)
    a = t / math.cos(slant)
    c = a / 2                                   # half the flat foot at the bottom
    # inner edges: the outer ones shifted right (left arm) and left (right arm) by a
    s = (cx - (x0 + a)) / ((cx - c) - x0)
    notch = y0 + s * h
    d.polygon([(x0, y0), (x0 + a, y0), (cx, notch), (x0 + w - a, y0), (x0 + w, y0),
               (cx + c, y1), (cx - c, y1)], fill=255)


def letter_r(d, x0, y0, w, h, t):
    """An R in the box (x0, y0, w, h) with strokes t wide: stem, round bowl, straight leg."""
    y1 = y0 + h
    bowl_h = h * 0.56                           # the bowl's outer height, from the top
    r = bowl_h / 2                              # its round right end
    xr = x0 + w * 0.92                          # its rightmost point
    d.rectangle([x0, y0, x0 + t, y1], fill=255)
    # the bowl: a stadium's right half, its hole cut out
    d.rectangle([x0, y0, xr - r, y0 + bowl_h], fill=255)
    d.ellipse([xr - 2 * r, y0, xr, y0 + bowl_h], fill=255)
    ri = r - t
    d.rectangle([x0 + t, y0 + t, xr - r, y0 + bowl_h - t], fill=0)
    d.ellipse([xr - r - ri, y0 + t, xr - r + ri, y0 + bowl_h - t], fill=0)
    # the leg, from the bowl's lower middle down to the right
    lx = x0 + w * 0.40
    ly = y0 + bowl_h - t
    d.polygon([(lx, ly), (lx + t * 1.25, ly), (x0 + w, y1), (x0 + w - t * 1.3, y1)], fill=255)


def draw():
    img = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    m = u(0.03)                                 # transparent margin round the tile
    d.rounded_rectangle([m, m, BIG - 1 - m, BIG - 1 - m], radius=u(0.2), fill=EDGE)
    rim = u(0.025)
    d.rounded_rectangle([m + rim, m + rim, BIG - 1 - m - rim, BIG - 1 - m - rim],
                        radius=u(0.2) - rim, fill=BACK)

    c = BIG / 2
    ro, ring = u(0.33), u(0.075)                # the sight's ring: outer radius, thickness
    d.ellipse([c - ro, c - ro, c + ro, c + ro], fill=SIGHT)
    d.ellipse([c - ro + ring, c - ro + ring, c + ro - ring, c + ro - ring], fill=BACK)
    # four ticks through the ring, from just inside it out towards the tile's edge
    tw, t_in, t_out = u(0.075), ro - ring - u(0.03), u(0.445)
    for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):
        if dx == 0:
            box = [c - tw / 2, c + dy * t_in, c + tw / 2, c + dy * t_out]
        else:
            box = [c + dx * t_in, c - tw / 2, c + dx * t_out, c + tw / 2]
        d.rectangle([min(box[0], box[2]), min(box[1], box[3]), max(box[0], box[2]), max(box[1], box[3])],
                    fill=SIGHT)

    # VR in the middle, as big as the ring's opening allows
    mask = Image.new("L", (BIG, BIG), 0)
    md = ImageDraw.Draw(mask)
    lh, lw, gap, stroke = u(0.25), u(0.17), u(0.02), u(0.06)
    x = c - (2 * lw + gap) / 2
    y = c - lh / 2
    letter_v(md, x, y, lw, lh, stroke)
    letter_r(md, x + lw + gap, y, lw, lh, stroke)
    img.paste(Image.new("RGBA", (BIG, BIG), LETTERS), (0, 0), mask)
    return img


def main():
    big = draw()
    shots = []
    for density, size in SIZES.items():
        out = RES / f"drawable-{density}" / "ic_launcher.png"
        out.parent.mkdir(parents=True, exist_ok=True)
        small = big.resize((size, size), Image.LANCZOS)
        small.save(out, optimize=True)
        shots.append(small)
        print(out.relative_to(ROOT), size)
    if len(sys.argv) > 1:
        scale = 4
        sheet = Image.new("RGBA", (sum(s.width * scale + 16 for s in shots) + 16, 144 * scale + 32), (60, 60, 60, 255))
        x = 16
        for s in shots:
            sheet.alpha_composite(s.resize((s.width * scale, s.height * scale), Image.NEAREST), (x, 16))
            x += s.width * scale + 16
        sheet.save(sys.argv[1])
        print("preview:", sys.argv[1])


if __name__ == "__main__":
    main()
