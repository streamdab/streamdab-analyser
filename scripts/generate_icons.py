#!/usr/bin/env python3
"""Regenerate every application icon from resources/icons/eti-stream-analyser.svg.

The generated files are COMMITTED, so building the application never needs an
SVG renderer or ImageMagick. Run this script only when the SVG changes:

    python3 scripts/generate_icons.py

Requires `rsvg-convert` (librsvg2-bin / brew install librsvg) and Pillow.
ImageMagick is deliberately not used: its built-in SVG renderer drops the
gradient and the transparency (the icon came out as a black disc on white).

Outputs (resources/icons/):
  eti-stream-analyser_<N>.png   N = 16 24 32 48 64 128 256 512   (Qt resources,
                                Linux hicolor theme, AppImage)
  eti-stream-analyser.ico       Windows: 16..256, BMP frames up to 48 px, PNG above
  eti-stream-analyser.icns      macOS: 16..512 plus the @2x variants up to 1024
"""
import io
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ICONS = ROOT / "resources" / "icons"
SVG = ICONS / "eti-stream-analyser.svg"
NAME = "eti-stream-analyser"

PNG_SIZES = (16, 24, 32, 48, 64, 128, 256, 512)
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)
# (icns type, pixel size): PNG-compressed members understood by macOS 10.7+.
ICNS_MEMBERS = (
    (b"icp4", 16), (b"icp5", 32), (b"icp6", 64),
    (b"ic07", 128), (b"ic08", 256), (b"ic09", 512), (b"ic10", 1024),
    (b"ic11", 32),    # 16x16@2x
    (b"ic12", 64),    # 32x32@2x
    (b"ic13", 256),   # 128x128@2x
    (b"ic14", 512),   # 256x256@2x
)


def render(size: int) -> Image.Image:
    """Render the SVG at size x size with a transparent background."""
    out = subprocess.run(
        ["rsvg-convert", "-w", str(size), "-h", str(size), "-f", "png", str(SVG)],
        check=True, capture_output=True).stdout
    return Image.open(io.BytesIO(out)).convert("RGBA")


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, "PNG", optimize=True)
    return buf.getvalue()


def ico_bmp_frame(img: Image.Image) -> bytes:
    """32-bit BGRA DIB (+ AND mask) as stored inside .ico files."""
    w, h = img.size
    header = struct.pack("<IiiHHIIiiII", 40, w, h * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    rows = []
    px = img.load()
    for y in range(h - 1, -1, -1):  # bottom-up
        row = bytearray()
        for x in range(w):
            r, g, b, a = px[x, y]
            row += bytes((b, g, r, a))
        rows.append(bytes(row))
    mask_row = ((w + 31) // 32) * 4
    return header + b"".join(rows) + b"\x00" * (mask_row * h)


def write_ico(path: Path, frames: dict) -> None:
    entries = []
    blobs = []
    for size in ICO_SIZES:
        img = frames[size]
        data = png_bytes(img) if size > 48 else ico_bmp_frame(img)
        blobs.append(data)
        entries.append((size, len(data)))
    out = bytearray(struct.pack("<HHH", 0, 1, len(entries)))
    offset = 6 + 16 * len(entries)
    for (size, length) in entries:
        dim = 0 if size >= 256 else size
        out += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, length, offset)
        offset += length
    for blob in blobs:
        out += blob
    path.write_bytes(bytes(out))


def write_icns(path: Path, frames: dict) -> None:
    body = bytearray()
    for kind, size in ICNS_MEMBERS:
        data = png_bytes(frames[size])
        body += kind + struct.pack(">I", len(data) + 8) + data
    path.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + bytes(body))


def main() -> int:
    if not shutil.which("rsvg-convert"):
        print("rsvg-convert not found (apt install librsvg2-bin / brew install librsvg)",
              file=sys.stderr)
        return 1
    sizes = sorted(set(PNG_SIZES) | set(ICO_SIZES) | {s for _, s in ICNS_MEMBERS})
    frames = {s: render(s) for s in sizes}
    for s in PNG_SIZES:
        (ICONS / f"{NAME}_{s}.png").write_bytes(png_bytes(frames[s]))
    write_ico(ICONS / f"{NAME}.ico", frames)
    write_icns(ICONS / f"{NAME}.icns", frames)
    print("wrote", ", ".join(f"{NAME}_{s}.png" for s in PNG_SIZES), f"{NAME}.ico", f"{NAME}.icns")
    return 0


if __name__ == "__main__":
    sys.exit(main())
