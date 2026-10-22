"""
Checks for adding a game to hotd2-vr on the PC: finds the game's camera matrices and values
in a RAM dump (rip\\ram0000.bin), sums up a rip pass, and reads the VR lines of a run's
flycast.log (vr-test.ps1 writes both under build-win\\vrtest\\<name>).

    python -I vrcheck.py matrices <ram.bin>
    python -I vrcheck.py find-float <ram.bin> <value> [--tol 0.01]
    python -I vrcheck.py find-u16 <ram.bin> <v1,v2,...>
    python -I vrcheck.py whist <ripdir> <n> [--max-w 3]
    python -I vrcheck.py border <ripdir> <n> <stockFocal> <liveFocal> [--min-ratio 0.5] [--max-w 3]
    python -I vrcheck.py shots <flycast.log> [--tex t1,t2] [--max-off N] [--each-gun N] [--from S] [--to S] [--union]
    python -I vrcheck.py focal <flycast.log> [--expect F --tol 0.02] [--from S] [--to S]
    python -I vrcheck.py selftest

RAM offsets are printed as the profile takes them (address & 0x01FFFFFF: the dump starts at
main RAM's base). Exit 1 when a check fails. Needs numpy (pip install numpy).
"""
import argparse
import contextlib
import io
import os
import re
import site
import struct
import sys
import tempfile
from collections import Counter

# python -I leaves out the user's site-packages, where pip install --user puts numpy: that one
# back (still nothing from the current or the script's directory), and riplib next door.
if site.getusersitepackages() not in sys.path:
    sys.path.append(site.getusersitepackages())
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets'))
import numpy as np  # noqa: E402


#
# RAM dumps
#

def _floats(path):
    data = open(path, 'rb').read()
    return np.frombuffer(data, '<f4', len(data) // 4)


def matrices(args):
    """4x4 float matrices: viewports (diag 320, -240 or +240) and projections ([1..4] zero,
    [0] and [5] the same focal over 320 and 240), the matrices readGameFocal multiplies."""
    a = _floats(args.ram)
    n = len(a) - 15
    finite = lambda i: np.isfinite(a[i:i + 16]).all()
    for i in np.nonzero((a[:n] == 320.0) & (np.abs(a[5:n + 5]) == 240.0))[0]:
        if finite(i):
            print(f'viewport 0x{i * 4:06X} {a[i]:.1f} {a[i + 5]:.1f}')
    with np.errstate(all='ignore'):
        fx = np.abs(a[:n].astype(np.float64)) * 320
        fy = np.abs(a[5:n + 5].astype(np.float64)) * 240
        hit = (a[1:n + 1] == 0) & (a[2:n + 2] == 0) & (a[3:n + 3] == 0) & (a[4:n + 4] == 0)
        hit &= (fx > 1) & (fx < 1e5) & (fy > 1) & (fy < 1e5) & (np.abs(fx - fy) <= 0.005 * np.maximum(fx, fy))
    for i in np.nonzero(hit)[0]:
        if finite(i):
            print(f'proj 0x{i * 4:06X} focal {fx[i]:.1f} {fy[i]:.1f}')
    return 0


def find_float(args):
    a = _floats(args.ram)
    with np.errstate(all='ignore'):
        hit = np.abs(a - args.value) <= args.tol * abs(args.value)
    for i in np.nonzero(hit)[0]:
        print(f'0x{i * 4:06X} {a[i]:.6g}')
    return 0


def find_u16(args):
    data = open(args.ram, 'rb').read()
    h = np.frombuffer(data, '<u2', len(data) // 2)
    seq = [int(v, 0) for v in args.values.split(',')]
    n = len(h) - len(seq) + 1
    hit = np.ones(max(n, 0), bool)
    for k, v in enumerate(seq):
        hit &= h[k:k + n] == v
    for i in np.nonzero(hit)[0]:
        print(f'0x{i * 2:06X}')
    return 0


#
# rips (riplib.Pass: vertices in framebuffer pixels, z = 1/W)
#

LISTS = ('op', 'pt', 'tr')


def _corners_w(p):
    """W of each corner, or None for a polygon with a hidden or invalid corner."""
    z = p.v['z'].astype(np.float64)
    if not (np.isfinite(p.v['x']).all() and np.isfinite(p.v['y']).all() and np.isfinite(z).all() and (z > 0).all()):
        return None
    return 1.0 / z


def _flat_w(w):
    """The polygon's one W (rounded to 0.001), or None when its corners differ."""
    return round(float(w.mean()), 3) if w.max() - w.min() <= 0.0005 else None


def whist(args):
    from riplib import Pass
    ps = Pass(args.ripdir, args.n)
    layers = {}
    for p in ps.polys:
        w = _corners_w(p)
        flat = _flat_w(w) if w is not None else None
        if flat is None or flat > args.max_w:
            continue
        layers.setdefault(flat, []).append(p)
    for w in sorted(layers):
        polys = layers[w]
        lists = ','.join(LISTS[k] for k in sorted({p.lst for p in polys}))
        tex = sorted({p.tex for p in polys})
        texs = ','.join('none' if t == 0 else f'{t:06x}' for t in tex[:8]) + (f',+{len(tex) - 8} more' if len(tex) > 8 else '')
        x = np.concatenate([p.v['x'] for p in polys])
        y = np.concatenate([p.v['y'] for p in polys])
        print(f'W {w:.3f} polys {len(polys)} list {lists} tex {texs} '
              f'box {x.min():.0f},{y.min():.0f}..{x.max():.0f},{y.max():.0f}')
    return 0


BANDS = ((0.0, 0.8), (0.8, 1.0), (1.0, 1.2), (1.2, 1.5))


def border(args):
    """Where the game stops drawing in a widened view: 3D polygon centres by r, the distance
    from the centre in units of the stock frame's half width (r 1 = its edge). The 2D layers
    (one W up to --max-w, as whist lists them) don't count; a 3D sprite at one W does."""
    from riplib import Pass
    ps = Pass(args.ripdir, args.n)
    edge = ps.fbw * 0.5 * args.live_focal / args.stock_focal
    count = [0] * len(BANDS)
    for p in ps.polys:
        w = _corners_w(p)
        if w is None:
            continue
        flat = _flat_w(w)
        if flat is not None and flat <= args.max_w:
            continue
        r = abs((p.v['x'].min() + p.v['x'].max()) * 0.5 - ps.fbw * 0.5) / edge
        for k, (lo, hi) in enumerate(BANDS):
            if lo <= r < hi:
                count[k] += 1
    density = [c / (hi - lo) for c, (lo, hi) in zip(count, BANDS)]
    for c, d, (lo, hi) in zip(count, density, BANDS):
        print(f'band {lo:.1f}-{hi:.1f} polys {c} density {d:.1f}')
    if density[1] == 0:
        print('cliff ratio unknown (no polygons at 0.8-1.0)')
        return 1
    ratio = density[2] / density[1]
    print(f'cliff ratio {ratio:.2f}')
    return 1 if ratio < args.min_ratio else 0


#
# flycast.log
#

def _log_ms(line):
    """Time of a log line (mm:ss:ms from process start), or None."""
    m = re.match(r'(\d+):(\d\d):(\d{3}) ', line)
    return (int(m[1]) * 60 + int(m[2])) * 1000 + int(m[3]) if m else None


SHOT = re.compile(r'VR: shot \+(-?\d+) ms gun (-?\d+),(-?\d+)(?: \(2D\))?: (\w+) (\d+) \(\d+ verts\) \d+x\d+, '
                  r'(\d+) px off, W (\S+?)\.\.(\S+?)(?: all-2D)?, tex ([0-9a-f]+) ')
SHOT_FRAME = re.compile(r'VR diag: shot \+(-?\d+) ms gun (-?\d+),(-?\d+)(?: \(2D\))?, fb ')
# (newer builds) a VR: shot line's box and where the game draws the shot (shotDrawnAt)
SHOT_AT = re.compile(r', at (-?\d+),(-?\d+)\.\.(-?\d+),(-?\d+), shot drawn at (-?\d+),(-?\d+)$')
NEW = re.compile(r'VR new after shot \+(-?\d+) ms gun (-?\d+),(-?\d+): (\w+) (\d+) tex ([0-9a-f]+) .*?, \d+ verts at '
                 r'(-?\d+),(-?\d+)\.\.(-?\d+),(-?\d+) \(centre \d+ px from the gun\) W \S+ col [0-9a-f]+, '
                 r'shot drawn at (-?\d+),(-?\d+)$')
MARKER_MAX_OFFSET = 120  # px: Flycast logs a shot's small polygons only this close to the gun (vr_reproject.cpp)


def _union_marker(frames):
    """--union: the first pass's matched polygons as one box, its centre against where the game
    draws the shot. frames: [(pass, +ms, (list, n), box, drawn, tex)]; a pass is the lines that
    go with one `VR diag: shot` line (two passes can come 3 ms apart)."""
    frames.sort(key=lambda f: (f[0], f[1]))
    first = [f for f in frames if f[0] == frames[0][0]]
    polys = {f[2]: f for f in first}.values()
    x0 = min(f[3][0] for f in polys)
    y0 = min(f[3][1] for f in polys)
    x1 = max(f[3][2] for f in polys)
    y1 = max(f[3][3] for f in polys)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    drawn = first[0][4]
    off = int(((cx - drawn[0]) ** 2 + (cy - drawn[1]) ** 2) ** 0.5 + 0.5)
    texes = ','.join(sorted({f[5] for f in polys}))
    return first[0][1], off, f'union of {len(polys)} polys at {cx:.0f},{cy:.0f}, drawn at {drawn[0]},{drawn[1]}: ' \
                             f'{off} px off, tex {texes}'


def shots(args):
    """The game's shot markers: per shot (trigger pull), the first frame after it with a marker
    (a polygon within 120 px of the gun, of --tex if given), its nearest one. Shots come from
    the `VR diag: shot` frame lines, so a shot that drew no marker shows as none. With --union
    (needs --tex), the first pass's polygons of --tex (VR: shot and VR new after shot lines, 3D
    ones too) within 120 px of where the game draws the shot, as one box: its centre's distance
    from that point (the gun, or with a widened view and the profile's markerWidened the point
    the widened view puts the stock-view shot at). With --max-ratio, every shot that drew a
    marker needs live/stock below it on each of its `VR diag: shot` lines up to the marker's first
    frame (later ones can fall in the next scene: a cut 0.3 s after a shot): a shootable scene
    drawn with a cutscene camera (live near stock) would aim wide, since the hit test uses stock."""
    tex = {int(t, 16) for t in args.tex.split(',')} if args.tex else None
    if args.union and tex is None:
        print('shots: --union needs --tex (without it every polygon within 120 px joins the box: '
              'the sights, the HUD)', file=sys.stderr)
        return 2
    groups = []  # [start ms, gun, marker (+ms, px off, list, n, W from, W to, tex), union frames, live/stock]
    # The pass each line belongs to. Flycast logs a pass as its `VR new after shot` lines (one +ms),
    # its `VR diag: shot` line, then its `VR: shot` lines: a pass opens with its first NEW line,
    # or with its diag line when it has none.
    npass, new_since = 0, None
    for line in open(args.log, errors='replace'):
        line = line.rstrip('\r\n')
        m = SHOT.search(line) or SHOT_FRAME.search(line) or (NEW.search(line) if args.union else None)
        t = _log_ms(line)
        if not m or t is None:
            continue
        if m.re is NEW:
            npass += new_since != m[1]
            new_since = m[1]
        else:
            npass += m.re is SHOT_FRAME and new_since is None
            new_since = None
        start = t - int(m[1])
        if (args.start is not None and start < args.start * 1000) or (args.end is not None and start > args.end * 1000):
            continue
        gun = (int(m[2]), int(m[3]))
        g = next((g for g in groups if g[1] == gun and abs(g[0] - start) <= 100), None)
        if g is None:
            g = [start, gun, None, [], []]
            groups.append(g)
        if m.re is SHOT_FRAME:
            s = STOCK.search(line)
            if s and float(s[2]):
                g[4].append((int(m[1]), float(s[1]) / float(s[2])))
            continue
        if args.union:
            a = SHOT_AT.search(line) if m.re is SHOT else None
            if m.re is SHOT and a is None:
                continue
            nums = [int(v) for v in (a.groups() if a else m.groups()[6:12])]
            texhex = m[9] if m.re is SHOT else m[6]
            box, drawn = tuple(nums[:4]), tuple(nums[4:])
            cx, cy = (box[0] + box[2]) / 2, (box[1] + box[3]) / 2
            if (tex is not None and int(texhex, 16) not in tex) \
                    or ((cx - drawn[0]) ** 2 + (cy - drawn[1]) ** 2) ** 0.5 > MARKER_MAX_OFFSET:
                continue
            g[3].append((npass, int(m[1]), (m[4], m[5]), box, drawn, texhex))
            continue
        if int(m[6]) > MARKER_MAX_OFFSET or (tex is not None and int(m[9], 16) not in tex):
            continue
        marker = (int(m[1]), int(m[6]), m[4], m[5], m[7], m[8], m[9])
        if g[2] is None or marker[:2] < g[2][:2]:
            g[2] = marker
    if args.union:
        for g in groups:
            g[2] = _union_marker(g[3]) if g[3] else None
    groups.sort(key=lambda g: (g[0], g[1]))
    worst = None
    for start, gun, marker, _, _ in groups:
        head = f'shot {start / 1000:.3f} gun {gun[0]},{gun[1]}: '
        if marker is None:
            print(head + f'none (nothing within {MARKER_MAX_OFFSET} px)')
            continue
        if args.union:
            since, off, text = marker
            print(head + f'+{since} ms {text}')
        else:
            since, off, lst, n, w0, w1, t = marker
            print(head + f'+{since} ms {lst} {n} {off} px off, W {w0}..{w1}, tex {t}')
        worst = off if worst is None else max(worst, off)
    matched = sum(g[2] is not None for g in groups)
    print(f'shots {len(groups)} matched {matched}' + (f' worst {worst} px off' if matched else ''))
    failed = matched == 0
    if args.max_off is not None:
        failed = failed or matched < len(groups) or worst > args.max_off
    if args.each_gun is not None:
        for gun in sorted({g[1] for g in groups}):
            offs = [g[2][1] for g in groups if g[1] == gun and g[2] is not None]
            n = sum(g[1] == gun for g in groups)
            print(f'gun {gun[0]},{gun[1]} shots {n} best ' + (f'{min(offs)} px off' if offs else 'none'))
            failed = failed or not offs or min(offs) > args.each_gun
    if args.max_ratio is not None:
        # each shot with a marker: its diag lines' live/stock up to the marker's first frame (+ms)
        hit = [(g[0], g[1], max(r for ms, r in g[4] if ms <= g[2][0])) for g in groups
               if g[2] is not None and any(ms <= g[2][0] for ms, _ in g[4])]
        for start, gun, ratio in hit:
            if ratio >= args.max_ratio:
                print(f'shot {start / 1000:.3f} gun {gun[0]},{gun[1]}: live/stock {ratio:.3f} (not below {args.max_ratio:g})')
        top = max((h[2] for h in hit), default=None)
        print(f'live/stock max {top:.3f} over {len(hit)} shots with a marker' if hit else
              'live/stock: no shot with a marker and a `VR diag: shot` focal/stock line')
        failed = failed or not hit or top >= args.max_ratio
    return 1 if failed else 0


STOCK = re.compile(r'VR diag: shot \+-?\d+ ms gun .*, focal (\S+) stock (\S+)$')


def stock(args):
    """The `VR diag: shot` lines (each pass while a shot is recent) binned by the game's stock
    focal: runs of lines whose stock focal stays within --tol of the run's first, with their
    span, line count and live/stock range (the widening the shot's mapped point uses). A still
    scene is one long run; a zoom is many short ones. Runs shorter than --min-s are counted, not
    shown."""
    runs, n = [], 0  # [from ms, to ms, lines, stock, live/stock min, max]
    for line in open(args.log, errors='replace'):
        m = STOCK.search(line)
        t = _log_ms(line)
        if not m or t is None or (args.start is not None and t < args.start * 1000) \
                or (args.end is not None and t > args.end * 1000):
            continue
        live, st = float(m[1]), float(m[2])
        ratio = live / st if st else 0.0
        n += 1
        if runs and abs(st - runs[-1][3]) <= args.tol * runs[-1][3]:
            r = runs[-1]
            r[1], r[2], r[4], r[5] = t, r[2] + 1, min(r[4], ratio), max(r[5], ratio)
        else:
            runs.append([t, t, 1, st, ratio, ratio])
    for t0, t1, lines, st, lo, hi in runs:
        if (t1 - t0) / 1000 >= args.min_s:
            print(f'{t0 / 1000:.3f}-{t1 / 1000:.3f} s ({(t1 - t0) / 1000:.1f} s) lines {lines} stock {st:.1f} '
                  f'live/stock {lo:.3f}..{hi:.3f}')
    print(f'lines {n} runs {len(runs)}')
    return 0 if n else 1


FOCAL = re.compile(r'VR: focal (\S+) x (\S+)  verts (\d+) overlay \d+  W near (\S+) ')


def _rank(values, q):
    """Nearest-rank percentile of sorted values."""
    return values[max(int(np.ceil(q * len(values))) - 1, 0)]


def focal(args):
    """The scene statistics the PC logs every 120 frames: the live focal, vertex counts, and
    the nearest W (a near plane shows as one value many frames share)."""
    fx, verts, wnear = [], [], Counter()
    for line in open(args.log, errors='replace'):
        m = FOCAL.search(line)
        t = _log_ms(line)
        if not m or t is None or (args.start is not None and t < args.start * 1000) \
                or (args.end is not None and t > args.end * 1000):
            continue
        fx.append(float(m[1]))
        verts.append(int(m[3]))
        wnear[f'{float(m[4]):.3f}'] += 1
    print(f'count {len(fx)}')
    if not fx:
        return 1
    fx.sort()
    verts.sort()
    median = _rank(fx, 0.5)
    print(f'focal min {fx[0]:.1f} median {median:.1f} max {fx[-1]:.1f}')
    print(f'verts p50 {_rank(verts, 0.5)} p95 {_rank(verts, 0.95)} max {verts[-1]}')
    print('wnear ' + ', '.join(f'{w} x{c}' for w, c in sorted(wnear.items())))
    code = 0
    if args.wnear_allow:
        # the after-census: each frame's nearest W after the 2D passes, one of these (W or lo-hi)
        allow = [tuple(float(v) for v in item.split('-')) for item in args.wnear_allow.split(',')]
        out = [(w, c) for w, c in sorted(wnear.items())
               if not any(a[0] - 0.0005 <= float(w) <= a[-1] + 0.0005 for a in allow)]
        if out:
            print('wnear not allowed ' + ', '.join(f'{w} x{c}' for w, c in out))
            code = 1
    if args.expect is not None and abs(median - args.expect) > args.tol * args.expect:
        return 1
    return code


def main(argv):
    ap = argparse.ArgumentParser(prog='vrcheck.py', description=__doc__.split('\n\n')[0])
    sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('matrices')
    s.add_argument('ram')
    s.set_defaults(run=matrices)
    s = sub.add_parser('find-float')
    s.add_argument('ram')
    s.add_argument('value', type=float)
    s.add_argument('--tol', type=float, default=0.01, help='relative')
    s.set_defaults(run=find_float)
    s = sub.add_parser('find-u16')
    s.add_argument('ram')
    s.add_argument('values', help='v1,v2,... (decimal or 0x hex)')
    s.set_defaults(run=find_u16)
    s = sub.add_parser('whist')
    s.add_argument('ripdir')
    s.add_argument('n', type=int)
    s.add_argument('--max-w', type=float, default=3.0)
    s.set_defaults(run=whist)
    s = sub.add_parser('border')
    s.add_argument('ripdir')
    s.add_argument('n', type=int)
    s.add_argument('stock_focal', type=float)
    s.add_argument('live_focal', type=float)
    s.add_argument('--min-ratio', type=float, default=0.5)
    s.add_argument('--max-w', type=float, default=3.0, help='flat layers up to this W are 2D')
    s.set_defaults(run=border)
    s = sub.add_parser('shots')
    s.add_argument('log')
    s.add_argument('--tex', help='t1,t2 (hex): only these textures')
    s.add_argument('--max-off', type=float, help='every shot needs a marker this close')
    s.add_argument('--each-gun', type=float, help='every gun position needs a marker this close')
    s.add_argument('--from', dest='start', type=float, help='shots from this second of the run on')
    s.add_argument('--to', dest='end', type=float)
    s.add_argument('--union', action='store_true',
                   help="a pass's polygons of --tex (required) as one box, against where the game draws the shot")
    s.add_argument('--max-ratio', type=float,
                   help='every shot with a marker: live/stock below this on its VR diag: shot lines to the marker')
    s.set_defaults(run=shots)
    s = sub.add_parser('stock')
    s.add_argument('log')
    s.add_argument('--tol', type=float, default=0.01, help='relative: a run\'s stock focal stays this close')
    s.add_argument('--min-s', type=float, default=1.0, help='show runs this long (s)')
    s.add_argument('--from', dest='start', type=float, help='seconds from process start')
    s.add_argument('--to', dest='end', type=float)
    s.set_defaults(run=stock)
    s = sub.add_parser('focal')
    s.add_argument('log')
    s.add_argument('--expect', type=float)
    s.add_argument('--tol', type=float, default=0.02, help='relative')
    s.add_argument('--from', dest='start', type=float, help='seconds from process start')
    s.add_argument('--to', dest='end', type=float)
    s.add_argument('--wnear-allow', help='W,lo-hi,...: every frame\'s nearest W one of these')
    s.set_defaults(run=focal)
    s = sub.add_parser('selftest')
    s.set_defaults(run=lambda args: selftest())
    args = ap.parse_args(argv)
    return args.run(args)


#
# selftest: synthetic RAM, rip and log, with MOK's values
#

def _matrix(*diag_and_more):
    return struct.pack('<16f', *diag_and_more)


def _synthetic_ram():
    ram = bytearray(0x100000)
    f = 240 / np.tan(np.radians(30))  # 415.692, the maze's stock focal
    ram[0x0F24D0:0x0F24D0 + 64] = _matrix(320, 0, 0, 0, 0, -240, 0, 0, 0, 0, 1, 0, 320, 240, 0, 1)
    ram[0x020000:0x020000 + 64] = _matrix(320, 0, 0, 0, 0, 240, 0, 0, 0, 0, 1, 0, 320, 240, 0, 1)
    ram[0x030000:0x030000 + 64] = _matrix(320, 0, 0, 0, 0, 100, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)  # not a viewport
    ram[0x0E7A60:0x0E7A60 + 64] = _matrix(f / 320, 0, 0, 0, 0, f / 240, 0, 0, 0, 0, -1.0002, -1, 0, 0, -0.2, 0)
    ram[0x010000:0x010000 + 64] = _matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)  # identity: not a projection
    ram[0x040000:0x040000 + 64] = _matrix(2, 0, 0, 0, 0, 2e36, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)  # [5] * 240 overflows a float
    struct.pack_into('<ff', ram, 0x0E7248, f, 500.0)
    for at in (0x0E2058, 0x0E2064):  # the factory gun calibration, twice
        struct.pack_into('<6H', ram, at, 64, 128, 500, 0, 128, 500)
    return bytes(ram)


def _synthetic_rip(ripdir):
    """pass0000.bin: HUD and letterbox layers, a layer past --max-w, and 3D triangles placed
    for border's bands (live focal 277.1, stock 415.7: the stock frame's edge 213.3 px out)."""
    from riplib import VERTEX
    lists = [[], [], []]

    def poly(lst, corners, tex=0):
        v = np.zeros(len(corners), VERTEX)
        for k, (x, y, w) in enumerate(corners):
            v[k]['x'], v[k]['y'], v[k]['z'] = x, y, 1.0 / w
        pcw = 8 if tex else 0
        lists[lst].append(struct.pack('<5I', 0, 0, tex >> 3, pcw, len(corners)) + v.tobytes())

    def quad(lst, x0, y0, x1, y1, w, tex=0):
        poly(lst, [(x0, y0, w), (x1, y0, w), (x0, y1, w), (x1, y1, w)], tex)

    quad(2, 12, 20, 100, 60, 1.082, 0x4f6800)
    quad(2, 500, 400, 630, 470, 1.082, 0x50b800)
    quad(0, 0, 0, 640, 40, 1.090)
    quad(0, 200, 200, 300, 300, 3.5)
    for dx in (0, 20, -40, 60, -80, 100, -120, 150, 180, -190, 200, -210, 230):
        x = 320 + dx
        poly(0, [(x - 2, 238, 5.0), (x + 2, 238, 6.0), (x - 2, 242, 7.0)])
    data = struct.pack('<4sIIffffI', b'RIP1', 640, 480, 277.1, 277.1, 1.0, 415.7, 3)
    for lst in lists:
        data += struct.pack('<I', len(lst)) + b''.join(lst)
    os.makedirs(ripdir, exist_ok=True)
    with open(os.path.join(ripdir, 'pass0000.bin'), 'wb') as f:
        f.write(data)


def _synthetic_log():
    pre = 'rend\\vr_reproject.cpp:1616 N[RENDERER]: '
    shot = 'VR: shot +{} ms gun {}: {} {} (4 verts) 40x40, {} px off, W {}, tex {} tsp 00000000 col ffffffff{}'
    frame = ('rend\\vr_reproject.cpp:1550 N[RENDERER]: VR diag: shot +{} ms gun {}, fb 640x480, '
             'polygons op 1 pt 2 tr 3, background 00000000')
    focal = 'VR: focal {} x {}  verts {} overlay 10  W near {} p10 2.000 median 10.000 p90 50.000 far 100.000'
    sky = ('OP', 2, 250, '3984.620..11820.980', 'd5a000', '')  # big: logged however far from the gun
    lines = [
        '00:00:864 hw\\naomi\\naomi_cart.cpp:667 N[NAOMI]: NAOMI GAME ID [THE MAZE OF THE KINGS] region ff players 0',
        # shot 1 at 31.484 s, centre: the starburst 3 px off in the first frame, then the red stars
        # 1 px off and a HUD sprite on the gun
        '00:31:500 ' + frame.format(16, '320,240'),
        '00:31:500 ' + pre + shot.format(16, '320,240', 'TR', 5, 3, '1.500..1.500', '50b800', ''),
        '00:31:517 ' + frame.format(33, '320,240'),
        '00:31:517 ' + pre + shot.format(33, '320,240', 'TR', 6, 1, '1.500..1.500', '4f6800', ''),
        '00:31:517 ' + pre + shot.format(33, '320,240', 'TR', 7, 0, '1.082..1.082 all-2D', '123400', ''),
        # shot 2 at 34.480 s elsewhere, into the 2D plane, dropped
        '00:34:500 ' + frame.format(20, '100,200 (2D)'),
        '00:34:500 ' + pre + shot.format(20, '100,200 (2D)', 'PT', 9, 0, '1.082..1.082 all-2D', '50b800', ' DROPPED'),
        # shot 3 at 34.500 s, centre again: 5 px off
        '00:34:516 ' + frame.format(16, '320,240'),
        '00:34:516 ' + pre + shot.format(16, '320,240', 'TR', 8, 5, '1.500..1.500', '50b800', ''),
        '00:34:516 ' + pre + shot.format(16, '320,240', *sky),
        # shot 4 at 37.000 s, bottom right: no marker, only the sky far off
        '00:37:017 ' + frame.format(17, '600,400'),
        '00:37:017 ' + pre + shot.format(17, '600,400', *sky),
    ]
    for t, (fx, verts, wnear) in enumerate([('415.7', 1000, '1.000'), ('415.7', 2000, '1.000'), ('277.1', 3000, '1.082'),
                                            ('277.1', 4000, '1.000'), ('415.7', 5000, '1.000')]):
        lines.append(f'00:{40 + 2 * t}:000 rend\\vr_reproject.cpp:176 N[RENDERER]: ' + focal.format(fx, fx, verts, wnear))
    return '\n'.join(lines) + '\n'


def _synthetic_log_union():
    """Newer shot lines (box, where the shot is drawn) for --union, in the order Flycast logs a
    pass: its `VR new after shot` lines, its `VR diag: shot` line, then its `VR: shot` lines (px
    off from where the shot is drawn). A burst of two triangles around the hit, a 3D impact, a
    widened view drawing the shots nearer the centre, two passes 3 ms apart."""
    pre = 'rend\\vr_reproject.cpp:1800 N[RENDERER]: '
    shot = ('VR: shot +{} ms gun {}: OP {} (3 verts) 12x8, {} px off, W 1.000..1.000 all-2D, tex 5e8000 tsp 2009a452 '
            'col ffffffff, at {}, shot drawn at {}')
    new = ('VR new after shot +{} ms gun {}: {} tex {} tsp 8491a4d2 isp 8b000000 tcw 100c2800, 4 verts at {} '
           '(centre {} px from the gun) W 27.00..27.00 col ffffffff, shot drawn at {}')
    frame = ('rend\\vr_reproject.cpp:1750 N[RENDERER]: VR diag: shot +{} ms gun {}, fb 640x480, '
             'polygons op 1 pt 2 tr 3, background 00000000, focal {} stock {}')
    return '\n'.join([
        # stock view: the impact new in the first pass, the burst's halves 6 px either side of the gun
        '00:50:016 ' + pre + new.format(16, '320,240', 'TR 40', '614000', '302,222..338,258', 0, '320,240'),
        '00:50:016 ' + frame.format(16, '320,240', '540.0', '540.0'),
        '00:50:016 ' + pre + shot.format(16, '320,240', 10, 6, '320,236..332,244', '320,240'),
        '00:50:016 ' + pre + shot.format(16, '320,240', 11, 6, '308,236..320,244', '320,240'),
        '00:50:032 ' + frame.format(32, '320,240', '540.0', '540.0'),
        '00:50:032 ' + pre + shot.format(32, '320,240', 10, 9, '320,234..338,246', '320,240'),
        # widened (219.4 / 540.0): drawn at 216,162 for the gun at 64,48, 179 px from it
        '00:52:016 ' + pre + new.format(16, '64,48', 'OP 10', '5e8000', '212,158..218,166', 177, '216,162'),
        '00:52:016 ' + frame.format(16, '64,48', '219.4', '540.0'),
        '00:52:016 ' + pre + shot.format(16, '64,48', 10, 1, '212,158..218,166', '216,162'),
        '00:52:016 ' + pre + shot.format(16, '64,48', 11, 1, '214,158..220,166', '216,162'),
        # widened, 4 px off where it is drawn
        '00:54:016 ' + frame.format(16, '576,240', '219.4', '540.0'),
        '00:54:016 ' + pre + shot.format(16, '576,240', 10, 4, '425,237..431,245', '424,240'),
        # another scene (436.4 / 879.2: drawn at 447,145), one frame drawn in two passes 3 ms apart:
        # the first pass's burst on the drawn point, the second's 10 px right of it
        '00:56:052 ' + frame.format(52, '576,48', '436.4', '879.2'),
        '00:56:052 ' + pre + shot.format(52, '576,48', 20, 0, '441,141..453,149', '447,145'),
        '00:56:055 ' + frame.format(55, '576,48', '436.4', '879.2'),
        '00:56:055 ' + pre + shot.format(55, '576,48', 30, 10, '451,141..463,149', '447,145'),
        # a cut to a cutscene after that marker (live near stock): the shot's later diag line
        '00:56:100 ' + frame.format(100, '576,48', '280.0', '288.9'),
    ]) + '\n'


def selftest():
    failures = []

    def check(argv, want_code, want_lines):
        buf = io.StringIO()
        try:
            with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
                code = main(argv)
        except SystemExit as e:
            code = e.code
        except Exception as e:  # report and go on with the other checks
            code = f'{type(e).__name__}: {e}'
        got = buf.getvalue().splitlines()
        if code != want_code or got != want_lines:
            failures.append(f'{" ".join(argv[:1] + argv[2:])}: exit {code}, want {want_code}\n'
                            + ''.join(f'    got  {line}\n' for line in got)
                            + ''.join(f'    want {line}\n' for line in want_lines))

    with tempfile.TemporaryDirectory() as tmp:
        ram = os.path.join(tmp, 'ram0000.bin')
        log = os.path.join(tmp, 'flycast.log')
        ripdir = os.path.join(tmp, 'rip')
        with open(ram, 'wb') as f:
            f.write(_synthetic_ram())
        with open(log, 'w') as f:
            f.write(_synthetic_log())
        try:
            _synthetic_rip(ripdir)
        except Exception as e:
            failures.append(f'synthetic rip: {type(e).__name__}: {e}')

        check(['matrices', ram], 0, ['viewport 0x020000 320.0 240.0', 'viewport 0x0F24D0 320.0 -240.0',
                                     'proj 0x0E7A60 focal 415.7 415.7'])
        check(['find-float', ram, '415.72'], 0, ['0x0E7248 415.692'])
        check(['find-float', ram, '415.72', '--tol', '0.00001'], 0, [])
        check(['find-u16', ram, '64,128,500,0,128,500'], 0, ['0x0E2058', '0x0E2064'])
        check(['find-u16', ram, '0x40,0x80,0x1f4'], 0, ['0x0E2058', '0x0E2064'])
        check(['whist', ripdir, '0'], 0, ['W 1.082 polys 2 list tr tex 4f6800,50b800 box 12,20..630,470',
                                          'W 1.090 polys 1 list op tex none box 0,0..640,40'])
        check(['whist', ripdir, '0', '--max-w', '4'], 0, ['W 1.082 polys 2 list tr tex 4f6800,50b800 box 12,20..630,470',
                                                          'W 1.090 polys 1 list op tex none box 0,0..640,40',
                                                          'W 3.500 polys 1 list op tex none box 200,200..300,300'])
        # the flat quad at W 3.5 is past --max-w: a 3D sprite, in the first band
        border = ['band 0.0-0.8 polys 9 density 11.2', 'band 0.8-1.0 polys 4 density 20.0',
                  'band 1.0-1.2 polys 1 density 5.0', 'band 1.2-1.5 polys 0 density 0.0', 'cliff ratio 0.25']
        check(['border', ripdir, '0', '415.7', '277.1'], 1, border)
        check(['border', ripdir, '0', '415.7', '277.1', '--min-ratio', '0.2'], 0, border)
        check(['border', ripdir, '0', '415.7', '277.1', '--min-ratio', '0.2', '--max-w', '4'], 0,
              ['band 0.0-0.8 polys 8 density 10.0'] + border[1:])
        none = ': none (nothing within 120 px)'
        first3 = ['shot 31.484 gun 320,240: +16 ms TR 5 3 px off, W 1.500..1.500, tex 50b800',
                  'shot 34.480 gun 100,200: +20 ms PT 9 0 px off, W 1.082..1.082, tex 50b800',
                  'shot 34.500 gun 320,240: +16 ms TR 8 5 px off, W 1.500..1.500, tex 50b800']
        all4 = first3 + ['shot 37.000 gun 600,400' + none, 'shots 4 matched 3 worst 5 px off']
        check(['shots', log], 0, all4)
        check(['shots', log, '--tex', '50b800,4f6800', '--max-off', '5'], 1, all4)
        check(['shots', log, '--tex', '50b800,4f6800', '--max-off', '5', '--to', '35'], 0,
              first3 + ['shots 3 matched 3 worst 5 px off'])
        check(['shots', log, '--tex', '50b800,4f6800', '--max-off', '2', '--to', '35'], 1,
              first3 + ['shots 3 matched 3 worst 5 px off'])
        stars = ['shot 31.484 gun 320,240: +33 ms TR 6 1 px off, W 1.500..1.500, tex 4f6800']
        check(['shots', log, '--tex', '4f6800', '--max-off', '2', '--to', '35'], 1,
              stars + ['shot 34.480 gun 100,200' + none, 'shot 34.500 gun 320,240' + none, 'shots 3 matched 1 worst 1 px off'])
        check(['shots', log, '--tex', '4f6800', '--max-off', '2', '--to', '32'], 0, stars + ['shots 1 matched 1 worst 1 px off'])
        check(['shots', log, '--from', '36'], 1, ['shot 37.000 gun 600,400' + none, 'shots 1 matched 0'])
        guns = ['gun 100,200 shots 1 best 0 px off', 'gun 320,240 shots 2 best 3 px off']
        check(['shots', log, '--each-gun', '3', '--to', '35'], 0, first3 + ['shots 3 matched 3 worst 5 px off'] + guns)
        check(['shots', log, '--each-gun', '2', '--to', '35'], 1, first3 + ['shots 3 matched 3 worst 5 px off'] + guns)
        check(['shots', log, '--each-gun', '3'], 1, all4 + guns + ['gun 600,400 shots 1 best none'])
        check(['shots', log, '--tex', '777777'], 1, ['shot 31.484 gun 320,240' + none, 'shot 34.480 gun 100,200' + none,
                                                    'shot 34.500 gun 320,240' + none, 'shot 37.000 gun 600,400' + none,
                                                    'shots 4 matched 0'])
        focal = ['count 5', 'focal min 277.1 median 415.7 max 415.7', 'verts p50 3000 p95 5000 max 5000',
                 'wnear 1.000 x4, 1.082 x1']
        check(['focal', log], 0, focal)
        check(['focal', log, '--expect', '415.72'], 0, focal)
        check(['focal', log, '--expect', '300'], 1, focal)
        check(['focal', log, '--from', '43', '--to', '47'], 0, ['count 2', 'focal min 277.1 median 277.1 max 277.1',
                                                                 'verts p50 3000 p95 4000 max 4000', 'wnear 1.000 x1, 1.082 x1'])
        check(['focal', log, '--from', '100'], 1, ['count 0'])
        check(['focal', log, '--wnear-allow', '0.999-1.001,1.082'], 0, focal)
        check(['focal', log, '--wnear-allow', '0.300,0.998-1.002'], 1, focal + ['wnear not allowed 1.082 x1'])

        log2 = os.path.join(tmp, 'flycast-union.log')
        with open(log2, 'w') as f:
            f.write(_synthetic_log_union())
        union = ['shot 50.000 gun 320,240: +16 ms union of 2 polys at 320,240, drawn at 320,240: 0 px off, tex 5e8000',
                 'shot 52.000 gun 64,48: +16 ms union of 2 polys at 216,162, drawn at 216,162: 0 px off, tex 5e8000',
                 'shot 54.000 gun 576,240: +16 ms union of 1 polys at 428,241, drawn at 424,240: 4 px off, tex 5e8000',
                 'shot 56.000 gun 576,48: +52 ms union of 1 polys at 447,145, drawn at 447,145: 0 px off, tex 5e8000']
        check(['shots', log2, '--tex', '5e8000', '--union'], 0, union + ['shots 4 matched 4 worst 4 px off'])
        check(['shots', log2, '--tex', '5e8000', '--union', '--max-off', '3'], 1, union + ['shots 4 matched 4 worst 4 px off'])
        check(['shots', log2, '--tex', '5e8000', '--union', '--max-off', '3', '--to', '53'], 0,
              union[:2] + ['shots 2 matched 2 worst 0 px off'])
        # two passes 3 ms apart are two frames: only the first one's polygons (merged: 452,145, 5 px off)
        check(['shots', log2, '--tex', '5e8000', '--union', '--max-off', '0', '--from', '55'], 0,
              union[3:] + ['shots 1 matched 1 worst 0 px off'])
        check(['shots', log2, '--tex', '614000', '--union', '--max-off', '3'], 1,
              ['shot 50.000 gun 320,240: +16 ms union of 1 polys at 320,240, drawn at 320,240: 0 px off, tex 614000',
               'shot 52.000 gun 64,48' + none, 'shot 54.000 gun 576,240' + none, 'shot 56.000 gun 576,48' + none,
               'shots 4 matched 1 worst 0 px off'])
        # --max-ratio: the stock-view shot (live/stock 1.000) fails it, the widened ones pass (0.406, 0.496);
        # the 56 s shot's cutscene line (0.969) comes after its marker
        check(['shots', log2, '--tex', '5e8000', '--union', '--max-ratio', '0.9'], 1,
              union + ['shots 4 matched 4 worst 4 px off', 'shot 50.000 gun 320,240: live/stock 1.000 (not below 0.9)',
                       'live/stock max 1.000 over 4 shots with a marker'])
        check(['shots', log2, '--tex', '5e8000', '--union', '--max-ratio', '0.9', '--from', '51'], 0,
              union[1:] + ['shots 3 matched 3 worst 4 px off', 'live/stock max 0.496 over 3 shots with a marker'])
        # a shot without a marker is not checked; none with one fails
        check(['shots', log2, '--tex', '614000', '--union', '--max-ratio', '0.9', '--from', '51'], 1,
              ['shot 52.000 gun 64,48' + none, 'shot 54.000 gun 576,240' + none, 'shot 56.000 gun 576,48' + none,
               'shots 3 matched 0', 'live/stock: no shot with a marker and a `VR diag: shot` focal/stock line'])
        # --union needs --tex: without it the box would take in the sights and the HUD
        check(['shots', log2, '--union'], 2, [])
        # without --union: each polygon on its own, its px off as logged (from where the shot is drawn)
        check(['shots', log2, '--tex', '5e8000'], 0,
              ['shot 50.000 gun 320,240: +16 ms OP 10 6 px off, W 1.000..1.000, tex 5e8000',
               'shot 52.000 gun 64,48: +16 ms OP 10 1 px off, W 1.000..1.000, tex 5e8000',
               'shot 54.000 gun 576,240: +16 ms OP 10 4 px off, W 1.000..1.000, tex 5e8000',
               'shot 56.000 gun 576,48: +52 ms OP 20 0 px off, W 1.000..1.000, tex 5e8000',
               'shots 4 matched 4 worst 6 px off'])
        # the stock focal's runs: the first scene 4 s (the live focal widened from its second shot), the other one
        # 3 ms (shown with --min-s 0)
        scene1 = '50.016-54.016 s (4.0 s) lines 4 stock 540.0 live/stock 0.406..1.000'
        scene2 = '56.052-56.055 s (0.0 s) lines 2 stock 879.2 live/stock 0.496..0.496'
        scene3 = '56.100-56.100 s (0.0 s) lines 1 stock 288.9 live/stock 0.969..0.969'
        check(['stock', log2], 0, [scene1, 'lines 7 runs 3'])
        check(['stock', log2, '--min-s', '0'], 0, [scene1, scene2, scene3, 'lines 7 runs 3'])
        check(['stock', log2, '--min-s', '0', '--from', '55'], 0, [scene2, scene3, 'lines 3 runs 2'])
        check(['stock', log2, '--from', '60'], 1, ['lines 0 runs 0'])

    for f in failures:
        print('FAIL ' + f, end='' if f.endswith('\n') else '\n')
    print('selftest OK' if not failures else f'selftest FAILED ({len(failures)})')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
