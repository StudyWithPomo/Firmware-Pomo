"""Convert the preview's raw 320x240 RGB565 dumps (out/*.rgb565) to PNG. Standard library only."""
import pathlib
import struct
import sys
import zlib

W, H = 320, 240


def rgb565_to_png(raw: bytes) -> bytes:
    rows = []
    for y in range(H):
        row = bytearray(b"\x00")  # filter: none
        for x in range(W):
            (v,) = struct.unpack_from("<H", raw, (y * W + x) * 2)
            r, g, b = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
            row += bytes(((r * 527 + 23) >> 6, (g * 259 + 33) >> 6, (b * 527 + 23) >> 6))
        rows.append(bytes(row))

    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    ihdr = struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))


def main() -> None:
    out = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "out")
    for raw_path in sorted(out.glob("*.rgb565")):
        raw = raw_path.read_bytes()
        if len(raw) != W * H * 2:
            print(f"skip {raw_path.name}: unexpected size {len(raw)}")
            continue
        raw_path.with_suffix(".png").write_bytes(rgb565_to_png(raw))
        raw_path.unlink()
        print(f"wrote {raw_path.with_suffix('.png').name}")


if __name__ == "__main__":
    main()
