"""
Draws the launcher icon of Confidential Mission VR (the Quest build, "vr" build type): the
initials CM inside the four corner brackets of a target lock, with a small VR under them, from
plain shapes only. No font, no game or company art (nothing of Sega's logo or the game's). Writes
the five legacy densities into the vr source set, where they override the main source set's
ic_launcher.png at the same sizes:

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
BACK = (20, 20, 24, 255)      # charcoal
EDGE = (120, 18, 24, 255)     # a dark red rim, so the tile stands out on a dark home screen
LOCK = (226, 36, 42, 255)     # crimson: the brackets and the VR
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


def letter_c(d, x0, y0, w, h, t):
    """A C in the box (x0, y0, w, h) with strokes t wide: a ring open on the right."""
    d.ellipse([x0, y0, x0 + w, y0 + h], fill=255)
    d.ellipse([x0 + t, y0 + t, x0 + w - t, y0 + h - t], fill=0)
    d.pieslice([x0 - t, y0 - t, x0 + w + t, y0 + h + t], -42, 42, fill=0)


def letter_m(d, x0, y0, w, h, t):
    """An M in the box (x0, y0, w, h) with strokes t wide: two stems and a V between them."""
    d.rectangle([x0, y0, x0 + t, y0 + h], fill=255)
    d.rectangle([x0 + w - t, y0, x0 + w, y0 + h], fill=255)
    letter_v(d, x0, y0, w, h * 0.68, t)


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
    d.rounded_rectangle([m, m, BIG - 1 - m, BIG - 1 - m], radius=u(0.18), fill=EDGE)
    rim = u(0.025)
    d.rounded_rectangle([m + rim, m + rim, BIG - 1 - m - rim, BIG - 1 - m - rim],
                        radius=u(0.18) - rim, fill=BACK)

    # the target lock: four corner brackets round the middle
    s0, s1, arm, t = u(0.18), u(0.82), u(0.15), u(0.045)
    for x, sx in ((s0, 1), (s1, -1)):
        for y, sy in ((s0, 1), (s1, -1)):
            d.rectangle(sorted_box(x, y, x + sx * arm, y + sy * t), fill=LOCK)
            d.rectangle(sorted_box(x, y, x + sx * t, y + sy * arm), fill=LOCK)

    # CM, as big as the brackets allow, a little above the middle
    mask = Image.new("L", (BIG, BIG), 0)
    md = ImageDraw.Draw(mask)
    lh, cw, mw, gap, stroke = u(0.27), u(0.22), u(0.25), u(0.035), u(0.065)
    x = BIG / 2 - (cw + gap + mw) / 2
    y = u(0.30)
    letter_c(md, x, y, cw, lh, stroke)
    letter_m(md, x + cw + gap, y, mw, lh, stroke)
    img.paste(Image.new("RGBA", (BIG, BIG), LETTERS), (0, 0), mask)

    # a small VR under it
    mask = Image.new("L", (BIG, BIG), 0)
    md = ImageDraw.Draw(mask)
    vh, vw, vgap, vstroke = u(0.115), u(0.085), u(0.014), u(0.024)
    x = BIG / 2 - (2 * vw + vgap) / 2
    y = u(0.64)
    letter_v(md, x, y, vw, vh, vstroke)
    letter_r(md, x + vw + vgap, y, vw, vh, vstroke)
    img.paste(Image.new("RGBA", (BIG, BIG), LOCK), (0, 0), mask)
    return img


def sorted_box(x0, y0, x1, y1):
    return [min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1)]


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
