#!/usr/bin/env python3
"""Minimal LZ4 legacy-frame decompressor (magic 0x184C2102), pure Python.
usage: lz4_decode.py <in.lz4> <out>"""
import struct, sys

def lz4_block(src, si, end, out):
    while si < end:
        token = src[si]; si += 1
        litlen = token >> 4
        if litlen == 15:
            while True:
                b = src[si]; si += 1; litlen += b
                if b != 255: break
        out += src[si:si+litlen]; si += litlen
        if si >= end: break
        offset = src[si] | (src[si+1] << 8); si += 2
        matchlen = (token & 0xf) + 4
        if matchlen == 19:
            while True:
                b = src[si]; si += 1; matchlen += b
                if b != 255: break
        start = len(out) - offset
        if offset >= matchlen:
            out += out[start:start+matchlen]
        else:
            for i in range(matchlen):
                out.append(out[start+i])
    return si

def main():
    data = open(sys.argv[1], 'rb').read()
    assert data[:4] == b'\x02\x21\x4c\x18', "not lz4 legacy"
    si = 4; out = bytearray(); nblk = 0
    while si < len(data):
        bsz = struct.unpack_from('<I', data, si)[0]; si += 4
        if bsz == 0: break
        si = lz4_block(data, si, si+bsz, out)
        nblk += 1
    open(sys.argv[2], 'wb').write(out)
    print(f"decompressed {nblk} blocks -> {sys.argv[2]} ({len(out)} bytes)")
    print("first bytes:", out[:8])

if __name__ == '__main__':
    main()