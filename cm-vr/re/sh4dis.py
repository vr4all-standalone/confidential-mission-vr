"""
Disassemble Dreamcast (SH4) code from a Flycast RAM dump: the 16 MB of main RAM, which
sits at 0x8C000000. Shows the values behind PC-relative literal loads.

    python sh4dis.py <start> <end> [ram.bin]     addresses in hex, e.g. 8c029b00 8c029c60

The dump defaults to $HOTD2VR_RAM, else run/ram.bin in the data folder next to this
repository. Needs capstone 5 (pip install capstone).
"""
import os, struct, sys
from pathlib import Path
import capstone as cs


def _ram_path():
    if __name__ == '__main__' and len(sys.argv) > 3:
        return Path(sys.argv[3])
    if os.environ.get('HOTD2VR_RAM'):
        return Path(os.environ['HOTD2VR_RAM'])
    # <data>/<repo>/cm-vr/re/sh4dis.py
    data = Path(os.environ.get('HOTD2VR_DATA') or Path(__file__).resolve().parents[3])
    return data / 'run' / 'ram.bin'


RAM = open(_ram_path(), 'rb').read()
BASE = 0x8C000000
md = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH4 | cs.CS_MODE_SHFPU)

def u32(addr):
    return struct.unpack_from('<I', RAM, addr - BASE)[0]

def f32(addr):
    return struct.unpack_from('<f', RAM, addr - BASE)[0]

def dis(start, end):
    out = []
    a = start
    while a < end:
        chunk = RAM[a - BASE:a - BASE + 2]
        ins = next(md.disasm(chunk, a), None)
        if ins is None:
            out.append((a, '.word', '0x%04x' % struct.unpack('<H', chunk)[0]))
        else:
            text = ins.op_str
            # literal loads: show the value
            if ins.mnemonic in ('mov.l', 'mova') and text.startswith('0x'):
                lit = int(text.split(',')[0], 16)
                if BASE <= lit < BASE + len(RAM):
                    v = u32(lit)
                    text += '    ; [%08x] = %08x (%g)' % (lit, v, f32(lit))
            out.append((a, ins.mnemonic, text))
        a += 2
    return out

if __name__ == '__main__':
    s, e = int(sys.argv[1], 16), int(sys.argv[2], 16)
    for a, m, t in dis(s, e):
        print('%08x  %-10s %s' % (a, m, t))
