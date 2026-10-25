"""Reading Flycast rip dumps (hotd2-vr): rip/passNNNN.bin as written by vr_reproject.cpp
(ripRequested), with the VRAM and palette next to them, and decoding PowerVR textures.

Copyright 2026 mikermak. Part of the hotd2-vr fork of Flycast, GPL v2 or later.
"""
import glob
import os
import struct

import numpy as np

# PowerVR texture control word / TSP fields used here
TSP_CLAMP_V, TSP_CLAMP_U, TSP_FLIP_V, TSP_FLIP_U, TSP_IGNORE_TEX_A = 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19


class Poly:
    """One polygon (a triangle strip) as the TA got it: lst 0 opaque, 1 punch-through, 2 translucent."""
    __slots__ = ('lst', 'idx', 'isp', 'tsp', 'tcw', 'pcw', 'v')

    @property
    def textured(self):
        return (self.pcw >> 3) & 1

    @property
    def tex(self):
        """VRAM address of the texture (0: none)."""
        return (self.tcw & 0x1FFFFF) << 3 if self.textured else 0

    @property
    def cull(self):
        """ISP cull mode: 0 none, 1 small, 2 and 3 by winding."""
        return (self.isp >> 27) & 3


VERTEX = np.dtype([('x', '<f4'), ('y', '<f4'), ('z', '<f4'), ('u', '<f4'), ('v', '<f4'),
                   ('col', 'u1', 4), ('spc', 'u1', 4)])


class Pass:
    """One ripped render pass. Vertices are in framebuffer pixels with z = 1/W."""

    def __init__(self, ripdir, n):
        self.dir = ripdir
        self.n = n
        data = open(os.path.join(ripdir, f'pass{n:04}.bin'), 'rb').read()
        magic, self.fbw, self.fbh, fx, fy, self.hudw, self.stockfocal, lists = struct.unpack_from('<4sIIffffI', data, 0)
        if magic != b'RIP1':
            raise ValueError(f'pass {n}: not a rip')
        self.focal = (fx, fy)
        off = 32
        self.polys = []
        for lst in range(lists):
            count, = struct.unpack_from('<I', data, off)
            off += 4
            for k in range(count):
                p = Poly()
                p.lst, p.idx = lst, k
                p.isp, p.tsp, p.tcw, p.pcw, nv = struct.unpack_from('<5I', data, off)
                off += 20
                p.v = np.frombuffer(data, VERTEX, nv, off).copy()
                off += nv * VERTEX.itemsize
                self.polys.append(p)
        self._vram = None

    @staticmethod
    def numbers(ripdir):
        return sorted(int(os.path.basename(f)[4:8]) for f in glob.glob(os.path.join(ripdir, 'pass????.bin')))

    def _latest(self, kind):
        best = None
        for f in glob.glob(os.path.join(self.dir, f'{kind}????.bin')):
            k = int(os.path.basename(f)[len(kind):len(kind) + 4])
            if k <= self.n and (best is None or k > best[0]):
                best = (k, f)
        if best is None:
            raise FileNotFoundError(f'no {kind} dump at or before pass {self.n}')
        return best[1]

    @property
    def vram(self):
        if self._vram is None:
            self._vram = np.fromfile(self._latest('vram'), np.uint8)
            pal = np.fromfile(self._latest('pal'), '<u4')
            self.palctrl = int(pal[0]) & 3
            self.palette = pal[1:]
        return self._vram

    def eye(self, p):
        """Game eye space of a polygon's vertices: game units, y up, looking down -z."""
        w = 1.0 / p.v['z']
        x = (p.v['x'] - self.fbw * 0.5) / self.focal[0] * w
        y = -(p.v['y'] - self.fbh * 0.5) / self.focal[1] * w
        return np.stack([x, y, -w], 1).astype(np.float64)


def strip_triangles(n):
    """Corner indices of the triangles of an n-vertex strip, with one winding throughout."""
    return [(i, i + 1, i + 2) if i % 2 == 0 else (i + 1, i, i + 2) for i in range(n - 2)]


def twiddle(x, y, w, h):
    """Index of pixel x, y in a twiddled w x h texture (as Flycast's twiddle_slow)."""
    x = np.asarray(x, np.int64)
    y = np.asarray(y, np.int64)
    rv = np.zeros(np.broadcast(x, y).shape, np.int64)
    sh = 0
    xs, ys = w >> 1, h >> 1
    while xs or ys:
        if ys:
            rv |= (y & 1) << sh
            y = y >> 1
            ys >>= 1
            sh += 1
        if xs:
            rv |= (x & 1) << sh
            x = x >> 1
            xs >>= 1
            sh += 1
    return rv


VQ_CODEBOOK = 256 * 8
VQ_MIP = [0x0, 0x1, 0x2, 0x6, 0x16, 0x56, 0x156, 0x556, 0x1556, 0x5556, 0x15556]
OTHER_MIP = [0x3, 0x4, 0x8, 0x18, 0x58, 0x158, 0x558, 0x1558, 0x5558, 0x15558, 0x55558]


def _unpack16(c, fmt):
    c = c.astype(np.uint32)
    if fmt == 0:  # ARGB1555
        a = np.where(c & 0x8000, 255, 0)
        r, g, b = ((c >> 10) & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, (c & 31) * 255 // 31
    elif fmt == 1:  # RGB565
        a = np.full(c.shape, 255)
        r, g, b = ((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31
    elif fmt == 2:  # ARGB4444
        a = ((c >> 12) & 15) * 17
        r, g, b = ((c >> 8) & 15) * 17, ((c >> 4) & 15) * 17, (c & 15) * 17
    else:
        raise ValueError(f'16-bit format {fmt}')
    return np.stack([r, g, b, a], -1).astype(np.uint8)


def _unpack_palette(entries, palctrl):
    if palctrl == 3:  # ARGB8888
        c = entries.astype(np.uint32)
        return np.stack([(c >> 16) & 255, (c >> 8) & 255, c & 255, (c >> 24) & 255], -1).astype(np.uint8)
    return _unpack16(entries & 0xFFFF, palctrl)


def decode_texture(ps, tcw, tsp):
    """RGBA image (h, w, 4) of a texture from the pass's VRAM dump (the top mip level)."""
    vram = ps.vram
    w = 8 << ((tsp >> 3) & 7)
    h = 8 << (tsp & 7)
    addr = (tcw & 0x1FFFFF) << 3
    planar = (tcw >> 26) & 1
    fmt = (tcw >> 27) & 7
    vq = (tcw >> 30) & 1
    mip = (tcw >> 31) & 1
    palsel = (tcw >> 21) & 63
    if fmt == 7:
        fmt = 0
    if mip:
        h = w
    ys, xs = np.mgrid[0:h, 0:w]
    lg = w.bit_length() - 1
    if fmt in (5, 6):  # palettised (always twiddled)
        bpp = 4 if fmt == 5 else 8
        base = addr + (OTHER_MIP[lg] * bpp // 8 if mip else 0)
        t = twiddle(xs, ys, w, h)
        if bpp == 4:
            b = vram[base + (t >> 1)]
            idx = np.where(t & 1, b >> 4, b & 15).astype(np.int64) + (palsel << 4)
        else:
            idx = vram[base + t].astype(np.int64) + ((palsel >> 4) << 8)
        return _unpack_palette(ps.palette[idx], ps.palctrl)
    if fmt in (3, 4):
        raise ValueError(f'texture format {fmt} (YUV, bump) not handled')
    if vq:
        if planar:
            raise ValueError('planar VQ texture')
        book = vram[addr:addr + VQ_CODEBOOK].view('<u2').reshape(256, 4)
        base = addr + VQ_CODEBOOK + (VQ_MIP[lg] if mip else 0)
        code = vram[base + twiddle(xs >> 1, ys >> 1, w >> 1, h >> 1)].astype(np.int64)
        return _unpack16(book[code, ((xs & 1) << 1) | (ys & 1)], fmt)
    if planar:  # (stride selection ignored)
        return _unpack16(vram[addr:addr + w * h * 2].view('<u2').reshape(h, w), fmt)
    base = addr + (OTHER_MIP[lg] * 2 if mip else 0)
    t = twiddle(xs, ys, w, h)
    lo = vram[base + t * 2].astype(np.uint16)
    hi = vram[base + t * 2 + 1].astype(np.uint16)
    return _unpack16(lo | (hi << 8), fmt)


class Canvas:
    """A small numpy z-buffered rasteriser, for previews."""

    def __init__(self, w, h, bg=(40, 40, 48)):
        self.w, self.h = w, h
        self.img = np.zeros((h, w, 3), np.float32)
        self.img[:] = np.array(bg, np.float32) / 255
        self.depth = np.zeros((h, w), np.float32)  # 1/distance, larger is nearer

    def triangle(self, p, invz, uv, col, tex=None, wrap=0, alpha_test=False):
        """p (3,2) pixels, invz (3,) 1/depth, uv (3,2), col (3,4) 0..1, tex float RGBA image."""
        x0 = int(max(np.floor(p[:, 0].min()), 0))
        x1 = int(min(np.ceil(p[:, 0].max()), self.w - 1))
        y0 = int(max(np.floor(p[:, 1].min()), 0))
        y1 = int(min(np.ceil(p[:, 1].max()), self.h - 1))
        if x1 < x0 or y1 < y0:
            return
        area = (p[1, 0] - p[0, 0]) * (p[2, 1] - p[0, 1]) - (p[2, 0] - p[0, 0]) * (p[1, 1] - p[0, 1])
        if abs(area) < 1e-9:
            return
        ys, xs = np.mgrid[y0:y1 + 1, x0:x1 + 1].astype(np.float32) + 0.5
        w0 = ((p[1, 0] - xs) * (p[2, 1] - ys) - (p[2, 0] - xs) * (p[1, 1] - ys)) / area
        w1 = ((p[2, 0] - xs) * (p[0, 1] - ys) - (p[0, 0] - xs) * (p[2, 1] - ys)) / area
        w2 = 1 - w0 - w1
        m = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
        iz = w0 * invz[0] + w1 * invz[1] + w2 * invz[2]
        zb = self.depth[y0:y1 + 1, x0:x1 + 1]
        m &= iz > zb
        if not m.any():
            return
        pw = np.stack([w0 * invz[0], w1 * invz[1], w2 * invz[2]], -1) / iz[..., None]
        c = pw @ col
        if tex is not None:
            th, tw = tex.shape[:2]
            uvp = pw @ uv
            c = c * tex[_wrap(uvp[..., 1], th, wrap & 2, wrap & 8), _wrap(uvp[..., 0], tw, wrap & 1, wrap & 4)]
        if alpha_test:
            m &= c[..., 3] > 0.5
        sub = self.img[y0:y1 + 1, x0:x1 + 1]
        sub[m] = c[m, :3]
        zb[m] = iz[m]

    def image(self):
        from PIL import Image
        return Image.fromarray((np.clip(self.img, 0, 1) * 255).astype(np.uint8))


def _wrap(t, n, clamp, mirror):
    i = np.floor(t * n).astype(np.int64)
    if clamp:
        return np.clip(i, 0, n - 1)
    if mirror:
        i = i % (2 * n)
        return np.where(i >= n, 2 * n - 1 - i, i)
    return i % n
