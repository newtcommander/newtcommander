"""Rasterize the Newt Commander "Split Disc - Extruded" icon to PNG + ICO.

Pure-Pillow implementation of temp/visual_style/icon/newt-commander-icon.svg
(rounded rects + 2 linear gradients + 1 radial gradient). Geometry is in the
SVG's 96x96 viewBox coordinate space; everything is drawn supersampled (SS x)
and downscaled with Lanczos for clean antialiasing.

Variants per temp/visual_style/README.txt:
  full       >= 48 px  (radial disc light, plate edges, rows)
  simple     24-48 px  (flat colors, rows kept)
  favicon    <= 16 px  (flat, no rows, wider centre gap)
"""

import math
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

VB = 96.0  # viewBox size
SS = 8     # supersample factor relative to target size


def _round_rect_mask(size_px, box, radius):
    """L-mode mask for a rounded rect given in viewBox coords."""
    s = size_px / VB
    m = Image.new("L", (size_px, size_px), 0)
    d = ImageDraw.Draw(m)
    x, y, w, h = box
    d.rounded_rectangle([x * s, y * s, (x + w) * s, (y + h) * s],
                        radius=radius * s, fill=255)
    return m


def _vlinear(size_px, box, c_top, c_bottom):
    """Vertical linear gradient image (RGBA, full canvas) clipped later by mask."""
    img = Image.new("RGBA", (size_px, size_px), (0, 0, 0, 0))
    s = size_px / VB
    x, y, w, h = box
    y0, y1 = int(y * s), int((y + h) * s) + 1
    for py in range(max(0, y0), min(size_px, y1)):
        t = (py - y * s) / (h * s)
        t = min(max(t, 0.0), 1.0)
        col = tuple(int(a + (b - a) * t) for a, b in zip(c_top, c_bottom)) + (255,)
        ImageDraw.Draw(img).line([(0, py), (size_px, py)], fill=col)
    return img


def _radial(size_px, box, c_in, c_out, cx=0.5, cy=0.3, r=1.0):
    """Radial gradient per SVG objectBoundingBox semantics (approx)."""
    img = Image.new("RGBA", (size_px, size_px), (0, 0, 0, 0))
    px = img.load()
    s = size_px / VB
    x, y, w, h = (v * s for v in box)
    ccx, ccy = x + cx * w, y + cy * h
    rad = r * math.sqrt(w * w + h * h) / math.sqrt(2)
    for py in range(size_px):
        for pxx in range(size_px):
            t = min(math.hypot(pxx - ccx, py - ccy) / rad, 1.0)
            px[pxx, py] = tuple(int(a + (b - a) * t) for a, b in zip(c_in, c_out)) + (255,)
    return img


def _paste(base, layer, mask):
    base.paste(layer, (0, 0), Image.composite(mask, Image.new("L", mask.size, 0), mask))


def _solid(size_px, color):
    return Image.new("RGBA", (size_px, size_px), color + (255,))


NAVY_EDGE = (0x02, 0x06, 0x0D)
DISC_IN, DISC_OUT = (0x1B, 0x30, 0x54), (0x07, 0x0E, 0x1A)
BLUE_TOP, BLUE_BOT = (0x6F, 0xA5, 0xFF), (0x2E, 0x6B, 0xE0)
OR_TOP, OR_BOT = (0xFF, 0xB3, 0x5C), (0xEA, 0x6A, 0x0B)
BLUE_EDGE, OR_EDGE = (0x1E, 0x4F, 0xB8), (0xB8, 0x53, 0x06)
BLUE_FLAT, OR_FLAT = (0x3B, 0x82, 0xF6), (0xF9, 0x73, 0x16)
NAVY_FLAT = (0x0A, 0x14, 0x24)
RING = (0x93, 0xC5, 0xFD)

ROWS = [(21, 31), (21, 45.5), (21, 60), (59, 31), (59, 45.5), (59, 60)]


def draw_icon(target, variant="full", plate_colors=None):
    """Render one icon frame. plate_colors optionally overrides (left, right) flat colors."""
    big = target * SS
    img = Image.new("RGBA", (big, big), (0, 0, 0, 0))

    # outer tile + disc
    _paste(img, _solid(big, NAVY_EDGE), _round_rect_mask(big, (2, 2, 92, 92), 21))
    disc_mask = _round_rect_mask(big, (4.5, 4.5, 87, 87), 18.5)
    if variant == "full":
        _paste(img, _radial(min(big, 512), (4.5, 4.5, 87, 87), DISC_IN, DISC_OUT)
               .resize((big, big), Image.LANCZOS), disc_mask)
        # inner ring, 18% light blue
        s = big / VB
        ring = Image.new("RGBA", (big, big), (0, 0, 0, 0))
        ImageDraw.Draw(ring).rounded_rectangle(
            [4.5 * s, 4.5 * s, 91.5 * s, 91.5 * s], radius=18.5 * s,
            outline=RING + (46,), width=max(1, int(1 * s)))
        img = Image.alpha_composite(img, ring)
    else:
        _paste(img, _solid(big, NAVY_FLAT), disc_mask)

    # plates
    if variant == "favicon":
        lbox, rbox = (10, 10, 34, 76), (52, 10, 34, 76)
        lrad = rrad = 9
        left, right = plate_colors or (BLUE_FLAT, OR_FLAT)
        _paste(img, _solid(big, left), _round_rect_mask(big, lbox, lrad))
        _paste(img, _solid(big, right), _round_rect_mask(big, rbox, rrad))
    elif variant == "simple":
        left, right = plate_colors or (BLUE_FLAT, OR_FLAT)
        _paste(img, _solid(big, left), _round_rect_mask(big, (11.5, 11.5, 35, 73), 9.5))
        _paste(img, _solid(big, right), _round_rect_mask(big, (49.5, 11.5, 35, 73), 9.5))
    else:  # full
        _paste(img, _solid(big, BLUE_EDGE), _round_rect_mask(big, (11.5, 11.5, 35, 73), 9.5))
        _paste(img, _vlinear(big, (13, 13, 32, 70), BLUE_TOP, BLUE_BOT),
               _round_rect_mask(big, (13, 13, 32, 70), 8))
        _paste(img, _solid(big, OR_EDGE), _round_rect_mask(big, (49.5, 11.5, 35, 73), 9.5))
        _paste(img, _vlinear(big, (51, 13, 32, 70), OR_TOP, OR_BOT),
               _round_rect_mask(big, (51, 13, 32, 70), 8))

    # listing rows (white, 50 %)
    if variant != "favicon":
        rows = Image.new("RGBA", (big, big), (0, 0, 0, 0))
        s = big / VB
        d = ImageDraw.Draw(rows)
        for rx, ry in ROWS:
            d.rounded_rectangle([rx * s, ry * s, (rx + 16) * s, (ry + 5) * s],
                                radius=2.5 * s, fill=(255, 255, 255, 128))
        img = Image.alpha_composite(img, rows)

    return img.resize((target, target), Image.LANCZOS)


def variant_for(size):
    if size <= 16:
        return "favicon"
    if size < 48:
        return "simple"
    return "full"


def write_ico(path, frames):
    """Manual ICO writer: BMP entries for sizes <= 64, PNG for larger."""
    entries = []
    for img in frames:
        w, h = img.size
        if w >= 128:
            import io
            buf = io.BytesIO()
            img.save(buf, "PNG")
            data = buf.getvalue()
        else:
            bgra = img.convert("RGBA").tobytes("raw", "BGRA")
            # rows bottom-up
            stride = w * 4
            pix = b"".join(bgra[(h - 1 - r) * stride:(h - r) * stride] for r in range(h))
            and_stride = ((w + 31) // 32) * 4
            and_mask = b"\x00" * (and_stride * h)
            header = struct.pack("<IiiHHIIiiII", 40, w, h * 2, 1, 32, 0,
                                 len(pix) + len(and_mask), 0, 0, 0, 0)
            data = header + pix + and_mask
        entries.append((w, h, data))

    out = struct.pack("<HHH", 0, 1, len(entries))
    offset = 6 + 16 * len(entries)
    dir_entries = b""
    blobs = b""
    for w, h, data in entries:
        dir_entries += struct.pack("<BBBBHHII", w % 256, h % 256, 0, 0, 1, 32,
                                   len(data), offset)
        blobs += data
        offset += len(data)
    Path(path).write_bytes(out + dir_entries + blobs)


def main(outdir, png_dir=None):
    """Write salamand.ico + sal_r/g/b.ico into outdir (upstream file names kept
    so no .rc/.vcxproj change is needed). Optional png_dir gets loose rasters."""
    out = Path(outdir)
    out.mkdir(parents=True, exist_ok=True)

    if png_dir:
        png = Path(png_dir)
        png.mkdir(parents=True, exist_ok=True)
        for size in (512, 256, 128, 64, 48, 32, 16):
            draw_icon(size, variant_for(size)).save(png / f"newt-commander-{size}.png")

    # main application icon (replaces upstream salamand.ico in place)
    sizes = (16, 24, 32, 48, 64, 128, 256)
    write_ico(out / "salamand.ico",
              [draw_icon(s, variant_for(s)) for s in sizes])

    # tray state icons (red / green / blue accents), 16 + 32 px
    states = {
        "r": ((0xEF, 0x44, 0x44), (0xB9, 0x1C, 0x1C)),
        "g": ((0x22, 0xC5, 0x5E), (0x15, 0x80, 0x3D)),
        "b": ((0x60, 0xA5, 0xFA), (0x1D, 0x4E, 0xD8)),
    }
    for key, colors in states.items():
        frames = [draw_icon(16, "favicon", plate_colors=colors),
                  draw_icon(32, "simple", plate_colors=colors)]
        write_ico(out / f"sal_{key}.ico", frames)

    print("OK: wrote salamand.ico, sal_r.ico, sal_g.ico, sal_b.ico to", out)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "out",
         sys.argv[2] if len(sys.argv) > 2 else None)
