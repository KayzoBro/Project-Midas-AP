"""Bake the Archipelago toast marks into port/include/ap_popup_icon.h.

The popup draws 32x32 RGBA32. The PNGs are 64x66, which does not fit one
load block, so this averages them down. Run from the repo root after
changing a PNG:

    python scripts/port/make_ap_icons.py
"""
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "assets", "archipelago")
OUT = os.path.join(ROOT, "port", "include", "ap_popup_icon.h")
SIZE = 32
NAMES = (
    "archipelago_progressive_icon.png",
    "archipelago_useful_icon.png",
    "archipelago_useless_icon.png",
)


def load_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit("%s is not a PNG" % path)
    i = 8
    width = height = 0
    color = 0
    bit = 0
    inter = 0
    idat = b""
    while i + 8 <= len(data):
        length = struct.unpack(">I", data[i:i + 4])[0]
        kind = data[i + 4:i + 8]
        chunk = data[i + 8:i + 8 + length]
        if kind == b"IHDR":
            width, height, bit, color, comp, filt, inter = struct.unpack(">IIBBBBB", chunk)
            if comp != 0 or filt != 0:
                raise SystemExit("%s: unsupported PNG compression" % path)
        elif kind == b"IDAT":
            idat += chunk
        elif kind == b"IEND":
            break
        i += 12 + length
    if bit != 8 or color != 6 or inter != 0:
        raise SystemExit("%s: need 8-bit RGBA, no interlace" % path)
    raw = zlib.decompress(idat)
    stride = width * 4
    rows = []
    prev = bytearray(stride)
    p = 0
    for _y in range(height):
        filt = raw[p]
        p += 1
        row = bytearray(raw[p:p + stride])
        p += stride
        if filt == 1:
            for x in range(stride):
                left = row[x - 4] if x >= 4 else 0
                row[x] = (row[x] + left) & 255
        elif filt == 2:
            for x in range(stride):
                row[x] = (row[x] + prev[x]) & 255
        elif filt == 3:
            for x in range(stride):
                left = row[x - 4] if x >= 4 else 0
                row[x] = (row[x] + ((left + prev[x]) // 2)) & 255
        elif filt == 4:
            for x in range(stride):
                a = row[x - 4] if x >= 4 else 0
                b = prev[x]
                c = prev[x - 4] if x >= 4 else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                row[x] = (row[x] + pr) & 255
        elif filt != 0:
            raise SystemExit("%s: filter %d" % (path, filt))
        rows.append(row)
        prev = row
    return width, height, rows


def downscale(width, height, rows):
    """Area average. Colour is weighted by alpha so a clear edge stays clean."""
    out = []
    for dy in range(SIZE):
        y0 = dy * height / SIZE
        y1 = (dy + 1) * height / SIZE
        for dx in range(SIZE):
            x0 = dx * width / SIZE
            x1 = (dx + 1) * width / SIZE
            red = green = blue = alpha = cover = 0.0
            y = int(y0)
            while y < height and y < y1:
                wy = min(y1, y + 1) - max(y0, y)
                x = int(x0)
                while x < width and x < x1:
                    wx = min(x1, x + 1) - max(x0, x)
                    w = wx * wy
                    px = rows[y][x * 4:x * 4 + 4]
                    a = px[3] / 255.0
                    red += px[0] * a * w
                    green += px[1] * a * w
                    blue += px[2] * a * w
                    alpha += a * w
                    cover += w
                    x += 1
                y += 1
            if alpha <= 0.0 or cover <= 0.0:
                out.append(0)
                continue
            r = int(round(red / alpha))
            g = int(round(green / alpha))
            b = int(round(blue / alpha))
            a = int(round(alpha / cover * 255.0))
            r = 0 if r < 0 else (255 if r > 255 else r)
            g = 0 if g < 0 else (255 if g > 255 else g)
            b = 0 if b < 0 else (255 if b > 255 else b)
            a = 0 if a < 0 else (255 if a > 255 else a)
            # Native u32, R in the top byte: the same layout texWriteLevel uses.
            out.append((r << 24) | (g << 16) | (b << 8) | a)
    return out


def main():
    icons = []
    for name in NAMES:
        path = os.path.join(SRC, name)
        width, height, rows = load_png(path)
        icons.append(downscale(width, height, rows))
        opaque = [px for px in icons[-1] if (px & 0xFF) > 16]
        if not opaque:
            raise SystemExit("%s scaled to nothing" % name)
        n = len(opaque)
        r = sum((px >> 24) & 255 for px in opaque) / n
        g = sum((px >> 16) & 255 for px in opaque) / n
        b = sum((px >> 8) & 255 for px in opaque) / n
        print("%s %dx%d -> %d  avg %d %d %d  opaque %d" % (name, width, height, SIZE, r, g, b, n))
    lines = [
        "/* generated by scripts/port/make_ap_icons.py - do not edit",
        " * 32x32 RGBA32, top row first, native u32 with R in the top byte.",
        " * 0 gold (progression or trap), 1 colour (useful), 2 grey (filler). */",
        "#ifndef AP_POPUP_ICON_H",
        "#define AP_POPUP_ICON_H",
        "",
        "#define AP_ICON_N 3",
        "#define AP_ICON_PX 32",
        "",
        "static const unsigned int ap_icon_px[AP_ICON_N][AP_ICON_PX * AP_ICON_PX] = {",
    ]
    for icon in icons:
        lines.append("    {")
        for i in range(0, len(icon), 8):
            chunk = ", ".join("0x%08x" % px for px in icon[i:i + 8])
            lines.append("        " + chunk + ",")
        lines.append("    },")
    lines.append("};")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    with open(OUT, "w", newline="\n") as handle:
        handle.write("\n".join(lines))
    print(OUT)


if __name__ == "__main__":
    main()
