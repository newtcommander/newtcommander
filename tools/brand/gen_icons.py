"""Pack the Newt Commander folder-tile icon rasters into the shipped .ico files.

The master artwork (newt-commander-icon.svg, feature 033) uses SVG filter
effects that cannot be reproduced procedurally, so the committed PNG renders
in tools/brand/png/ are the authoritative rasters. This script only PACKS
them (and derives the red/green/blue window-icon variants by hue-remapping
the saturated orange folder pixels) — it does not draw anything.

Usage (from anywhere; paths are resolved relative to this file):

    python tools/brand/gen_icons.py            # regenerate all shipped ICOs
    python tools/brand/gen_icons.py --verify   # structural check, no writes

ICO packing convention (kept from feature 032): 32-bpp; BMP-encoded entries
for sizes <= 64 px, PNG-encoded entries for sizes >= 128 px.

src/res/logo.svg (About/splash tile, IDB_LOGO_HAND) is NOT generated here —
it is a hand-maintained nanosvg-safe copy of tools/brand/logo.svg.
"""

import colorsys
import io
import struct
import sys
from pathlib import Path

from PIL import Image

BRAND_DIR = Path(__file__).resolve().parent
REPO_ROOT = BRAND_DIR.parent.parent
PNG_DIR = BRAND_DIR / "png"

# Hue-remap tuning (see specs/033-replace-app-icon/research.md R3): only
# pixels inside the orange hue band AND above the saturation threshold are
# recolored, so the pale cream pill/dot and the papers stay neutral.
ORANGE_BAND = (15.0, 50.0)  # degrees
SAT_THRESHOLD = 0.45

# Target hues for the main-window icon color variants (degrees).
HUE_RED = 0.0
HUE_GREEN = 142.0
HUE_BLUE = 213.0

FULL_SIZES = (16, 24, 32, 48, 64, 128, 256)
STATE_SIZES = (16, 32)

# Shipped outputs: repo-relative path -> (sizes, hue-remap target or None).
TARGETS = {
    "src/res/salamand.ico": (FULL_SIZES, None),
    "src/res/sal_r.ico": (STATE_SIZES, HUE_RED),
    "src/res/sal_g.ico": (STATE_SIZES, HUE_GREEN),
    "src/res/sal_b.ico": (STATE_SIZES, HUE_BLUE),
    "src/salmon/res/salmon.ico": (FULL_SIZES, None),
    "src/setup/res/setup.ico": (FULL_SIZES, None),
    "src/setup/remove/icon1.ico": (FULL_SIZES, None),
}


def load_src(size):
    """Load the authoritative raster for one size."""
    path = PNG_DIR / f"newt-commander-icon-{size}.png"
    img = Image.open(path).convert("RGBA")
    if img.size != (size, size):
        raise SystemExit(f"error: {path} is {img.size}, expected {size}x{size}")
    return img


def recolor(img, target_hue):
    """Return a copy with saturated orange-band pixels remapped to target_hue."""
    out = img.copy()
    px = out.load()
    w, h = out.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                continue
            hue, light, sat = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
            deg = hue * 360.0
            if ORANGE_BAND[0] <= deg <= ORANGE_BAND[1] and sat >= SAT_THRESHOLD:
                nr, ng, nb = colorsys.hls_to_rgb(target_hue / 360.0, light, sat)
                px[x, y] = (round(nr * 255), round(ng * 255), round(nb * 255), a)
    return out


def write_ico(path, frames):
    """Manual ICO writer: BMP entries for sizes <= 64, PNG for larger."""
    entries = []
    for img in frames:
        w, h = img.size
        if w >= 128:
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


def generate():
    for rel, (sizes, hue) in TARGETS.items():
        frames = [load_src(s) for s in sizes]
        if hue is not None:
            frames = [recolor(f, hue) for f in frames]
        out = REPO_ROOT / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        write_ico(out, frames)
        print(f"OK: wrote {rel} ({', '.join(str(s) for s in sizes)} px)")


def verify():
    """Structural check of every shipped ICO against the TARGETS table."""
    failures = 0
    for rel, (sizes, _hue) in TARGETS.items():
        path = REPO_ROOT / rel
        problems = []
        try:
            data = path.read_bytes()
            count = struct.unpack("<H", data[4:6])[0]
            if count != len(sizes):
                problems.append(f"{count} entries, expected {len(sizes)}")
            for i, size in enumerate(sizes[:count]):
                off = 6 + 16 * i
                w, h, _cc, _res, _planes, bpp, length, imgoff = struct.unpack(
                    "<BBBBHHII", data[off:off + 16])
                w, h = w or 256, h or 256
                if (w, h) != (size, size):
                    problems.append(f"entry {i}: {w}x{h}, expected {size}x{size}")
                if bpp != 32:
                    problems.append(f"entry {i}: {bpp} bpp, expected 32")
                payload = data[imgoff:imgoff + length]
                if size >= 128:
                    if payload[:8] != b"\x89PNG\r\n\x1a\n":
                        problems.append(f"entry {i}: expected PNG encoding")
                else:
                    bi_size, bi_w, bi_h, _p, bi_bpp = struct.unpack(
                        "<IiiHH", payload[:16])
                    if (bi_size, bi_w, bi_h, bi_bpp) != (40, size, size * 2, 32):
                        problems.append(f"entry {i}: bad BITMAPINFOHEADER")
        except FileNotFoundError:
            problems.append("missing file")
        except (struct.error, IndexError):
            problems.append("truncated/corrupt ICO")
        status = "OK" if not problems else "FAIL: " + "; ".join(problems)
        print(f"{rel}: {status}")
        failures += bool(problems)
    return failures == 0


if __name__ == "__main__":
    if "--verify" in sys.argv[1:]:
        sys.exit(0 if verify() else 1)
    generate()
