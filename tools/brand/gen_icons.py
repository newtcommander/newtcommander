"""Regenerate the shipped Tandem Commander brand assets from hand-swappable sources.

Feature 035: every graphic is replaceable by swapping a file here and
re-running this script — no source-code or project-file edits.

Inputs (all in tools/brand/):

    icon-master.png   REQUIRED  square PNG, edge >= 256 px (1024 recommended);
                                every icon size is derived from it (Lanczos)
    icon-<N>.png      OPTIONAL  N in {16, 24, 32, 48, 64, 128, 256}; must be
                                exactly NxN; overrides the master-derived
                                rendering for that one size
    about.png         REQUIRED  artwork drawn in the About dialog and on the
                                splash screen (copied verbatim to
                                src/res/logo.png; alpha supported; scaled to
                                fit at draw time, aspect preserved)

Usage (from anywhere; paths are resolved relative to this file):

    python tools/brand/gen_icons.py            # regenerate all shipped assets
    python tools/brand/gen_icons.py --verify   # structural check, no writes

ICO packing convention (kept from feature 032): 32-bpp; BMP-encoded entries
for sizes <= 64 px, PNG-encoded entries for sizes >= 128 px.
"""

import io
import struct
import sys
from pathlib import Path

from PIL import Image

BRAND_DIR = Path(__file__).resolve().parent
REPO_ROOT = BRAND_DIR.parent.parent

MASTER = BRAND_DIR / "icon-master.png"
ABOUT = BRAND_DIR / "about.png"

ICON_SIZES = (16, 24, 32, 48, 64, 128, 256)
MASTER_MIN_EDGE = 256

# Shipped ICO outputs (repo-relative); all pack the full size set.
ICO_TARGETS = (
    "src/res/salamand.ico",
    "src/salmon/res/salmon.ico",
    "src/setup/res/setup.ico",
    "src/setup/remove/icon1.ico",
)

LOGO_PNG = "src/res/logo.png"

PNG_MAGIC = b"\x89PNG\r\n\x1a\n"


def fail(msg):
    sys.exit(f"error: {msg}")


def load_png(path):
    """Open a PNG as RGBA or fail with a message naming the file."""
    try:
        img = Image.open(path)
        if img.format != "PNG":
            fail(f"{path} is {img.format}, expected PNG")
        return img.convert("RGBA")
    except FileNotFoundError:
        fail(f"{path} is missing")
    except Image.UnidentifiedImageError:
        fail(f"{path} cannot be decoded as PNG")


def load_sources():
    """Validate ALL inputs up front; nothing is written if any input is bad."""
    master = load_png(MASTER)
    w, h = master.size
    if w != h:
        fail(f"{MASTER} is {w}x{h}, expected a square image")
    if w < MASTER_MIN_EDGE:
        fail(f"{MASTER} is {w}x{h}, expected edge >= {MASTER_MIN_EDGE} px")

    overrides = {}
    for size in ICON_SIZES:
        path = BRAND_DIR / f"icon-{size}.png"
        if not path.exists():
            continue
        img = load_png(path)
        if img.size != (size, size):
            fail(f"{path} is {img.size[0]}x{img.size[1]}, expected exactly {size}x{size}")
        overrides[size] = img

    if not ABOUT.exists():
        fail(f"{ABOUT} is missing")
    load_png(ABOUT)  # decodability check only; the file is copied verbatim

    return master, overrides


def frame_for(size, master, overrides):
    if size in overrides:
        return overrides[size]
    return master.resize((size, size), Image.LANCZOS)


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
            bgra = img.tobytes("raw", "BGRA")
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
    master, overrides = load_sources()
    frames = [frame_for(s, master, overrides) for s in ICON_SIZES]
    for rel in ICO_TARGETS:
        out = REPO_ROOT / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        write_ico(out, frames)
        print(f"OK: wrote {rel} ({', '.join(str(s) for s in ICON_SIZES)} px)")
    logo = REPO_ROOT / LOGO_PNG
    logo.write_bytes(ABOUT.read_bytes())
    print(f"OK: wrote {LOGO_PNG} (copy of {ABOUT.name})")


def verify():
    """Structural check of every shipped asset, no writes."""
    failures = 0
    for rel in ICO_TARGETS:
        path = REPO_ROOT / rel
        problems = []
        try:
            data = path.read_bytes()
            count = struct.unpack("<H", data[4:6])[0]
            if count != len(ICON_SIZES):
                problems.append(f"{count} entries, expected {len(ICON_SIZES)}")
            for i, size in enumerate(ICON_SIZES[:count]):
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
                    if payload[:8] != PNG_MAGIC:
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

    logo = REPO_ROOT / LOGO_PNG
    if not logo.exists():
        print(f"{LOGO_PNG}: FAIL: missing file")
        failures += 1
    elif logo.read_bytes()[:8] != PNG_MAGIC:
        print(f"{LOGO_PNG}: FAIL: not a PNG")
        failures += 1
    else:
        print(f"{LOGO_PNG}: OK")
    return failures == 0


if __name__ == "__main__":
    if "--verify" in sys.argv[1:]:
        sys.exit(0 if verify() else 1)
    generate()
