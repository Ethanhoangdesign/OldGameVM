#!/usr/bin/env python3
"""Extract Wildfire map_screen_bottom.sti as a native-scale PNG."""
import argparse
import struct
import zlib
from pathlib import Path

SLF_HEADER = 532
STCI_HEADER = 64


def slf_find(path, filename):
    blob = path.read_bytes()
    count = struct.unpack_from("<I", blob, 512)[0]
    start = len(blob) - count * 280
    for i in range(count):
        entry = blob[start + i * 280:start + (i + 1) * 280]
        name = entry[:256].split(b"\0")[0].decode("latin-1")
        if name.lower() == filename.lower():
            return blob, *struct.unpack_from("<II", entry, 256)
    raise SystemExit(f"{filename} not found in {path}")


def decode_etrle(data, width, height):
    out = bytearray(width * height)
    pos = 0
    for y in range(height):
        x = 0
        while data[pos] != 0:
            count = data[pos]
            pos += 1
            if count & 0x80:
                x += count & 0x7f
            else:
                out[y * width + x:y * width + x + count] = data[pos:pos + count]
                pos += count
                x += count
        pos += 1
    return out


def png_chunk(kind, data):
    return (struct.pack(">I", len(data)) + kind + data +
            struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff))


def write_png(path, pixels, width, height):
    scanlines = b"".join(b"\0" + pixels[y * width * 3:(y + 1) * width * 3]
                         for y in range(height))
    payload = (b"\x89PNG\r\n\x1a\n" +
               png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
               png_chunk(b"IDAT", zlib.compress(scanlines, 9)) +
               png_chunk(b"IEND", b""))
    path.write_bytes(payload)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("game_dir", type=Path)
    parser.add_argument("--output", type=Path, default=Path("/tmp/wildfire-map-panel.png"))
    args = parser.parse_args()

    archive, offset, length = slf_find(args.game_dir / "Data" / "InterFace.slf", "map_screen_bottom.sti")
    sti = archive[offset:offset + length]
    palette = sti[STCI_HEADER:STCI_HEADER + 768]
    data_offset, data_len, _, _, height, width = struct.unpack_from("<IIhhHH", sti, STCI_HEADER + 768)
    subimages_end = STCI_HEADER + 768 + struct.unpack_from("<H", sti, 28)[0] * 16
    indexes = decode_etrle(sti[subimages_end + data_offset:subimages_end + data_offset + data_len], width, height)

    rgb = bytearray(width * height * 3)
    for y in range(height):
        for x in range(width):
            index = indexes[y * width + x]
            rgb[(y * width + x) * 3:(y * width + x + 1) * 3] = palette[index * 3:index * 3 + 3]
    write_png(args.output, rgb, width, height)
    print(f"{width}x{height}: {args.output}")


if __name__ == "__main__":
    main()
