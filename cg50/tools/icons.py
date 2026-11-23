"""Writes the two 92x64 24-bit BMP menu icons for the Squad Rush add-in."""
import struct, sys

W, H = 92, 64

def make(selected):
    px = [[(0x2f, 0x86, 0xb8)] * W for _ in range(H)]   # sea
    def rect(x0, y0, x1, y1, c):
        for y in range(max(0, y0), min(H, y1)):
            for x in range(max(0, x0), min(W, x1)):
                px[y][x] = c
    rect(0, 0, W, 8, (0xbf, 0xe6, 0xf5))                 # sky
    # road trapezoid: narrow at top, wide at bottom
    for y in range(8, H):
        t = (y - 8) / (H - 8)
        half = int(8 + t * 34)
        rect(46 - half - 3, y, 46 - half, y + 1, (0xbd, 0xbd, 0xbd))
        rect(46 + half, y, 46 + half + 3, y + 1, (0xbd, 0xbd, 0xbd))
        rect(46 - half, y, 46 + half, y + 1, (0xcb, 0xbd, 0xbb))
        if (y // 5) % 2 == 0:
            rect(45, y, 47, y + 1, (0xff, 0xff, 0xff))
    # gate (blue good / red bad)
    rect(28, 18, 46, 24, (0x2f, 0x8c, 0xff))
    rect(46, 18, 64, 24, (0xff, 0x3b, 0x3b))
    # red enemy blob (far)
    for (x, y) in [(40, 12), (44, 12), (48, 12), (42, 14), (46, 14), (50, 14)]:
        rect(x, y, x + 2, y + 3, (0xe8, 0x34, 0x3a))
    # blue squad (near)
    for (x, y) in [(38, 44), (43, 43), (48, 44), (53, 45), (36, 50), (41, 49), (46, 50), (51, 49), (56, 51), (40, 55), (45, 56), (50, 55)]:
        rect(x, y, x + 4, y + 2, (0x2f, 0x7b, 0xf0))        # head
        rect(x - 1, y + 2, x + 5, y + 6, (0x2f, 0x7b, 0xf0))  # body
        rect(x - 1, y + 6, x + 1, y + 8, (0x1a, 0x4f, 0xb0))  # legs
        rect(x + 3, y + 6, x + 5, y + 8, (0x1a, 0x4f, 0xb0))
    if selected:
        for i in range(2):
            rect(i, i, W - i, i + 1, (0xff, 0xd2, 0x3a)); rect(i, H - 1 - i, W - i, H - i, (0xff, 0xd2, 0x3a))
            rect(i, i, i + 1, H - i, (0xff, 0xd2, 0x3a)); rect(W - 1 - i, i, W - i, H - i, (0xff, 0xd2, 0x3a))
    row = W * 3                                         # 276, already 4-byte aligned
    data = b"".join(bytes(c for (r, g, b) in px[y] for c in (b, g, r)) for y in range(H - 1, -1, -1))
    header = b"BM" + struct.pack("<IHHI", 54 + len(data), 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, W, H, 1, 24, 0, len(data), 2835, 2835, 0, 0)
    return header + info + data

out = sys.argv[1]
open(out + "/icon-uns.bmp", "wb").write(make(False))
open(out + "/icon-sel.bmp", "wb").write(make(True))
print("icons written")
