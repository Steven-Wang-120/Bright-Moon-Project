from __future__ import annotations

import argparse
import re
from pathlib import Path

from PIL import Image


VALID_FACES = (
    "happy",
    "sad",
    "angry",
    "surprised",
    "thinking",
    "neutral",
)

SUPPORTED_SUFFIXES = {".png", ".bmp", ".jpg", ".jpeg"}
WIDTH = 128
HEIGHT = 64
BITMAP_SIZE = WIDTH * HEIGHT // 8


def sanitize_identifier(text: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", text)


def load_bitmap(image_path: Path, invert: bool) -> list[int]:
    image = Image.open(image_path).convert("RGBA")
    background = Image.new("RGBA", image.size, (255, 255, 255, 255))
    image = Image.alpha_composite(background, image).convert("L")
    image = image.resize((WIDTH, HEIGHT))

    buffer = [0] * BITMAP_SIZE
    pixels = image.load()

    for page in range(HEIGHT // 8):
        for x in range(WIDTH):
            value = 0
            for bit in range(8):
                y = page * 8 + bit
                pixel = pixels[x, y]
                lit = pixel < 128
                if invert:
                    lit = not lit
                if lit:
                    value |= 1 << bit
            buffer[page * WIDTH + x] = value
    return buffer


def format_byte_lines(data: list[int], indent: str = "    ", columns: int = 16) -> str:
    lines: list[str] = []
    for start in range(0, len(data), columns):
        chunk = data[start : start + columns]
        line = ", ".join(f"0x{value:02X}" for value in chunk)
        lines.append(f"{indent}{line},")
    return "\n".join(lines)


def collect_assets(assets_root: Path) -> list[tuple[str, int, Path]]:
    entries: list[tuple[str, int, Path]] = []
    for face_dir in sorted(assets_root.iterdir()):
        if not face_dir.is_dir():
            continue
        face = face_dir.name.lower()
        if face not in VALID_FACES:
            print(f"[warn] skip unknown face directory: {face_dir.name}")
            continue

        for image_path in sorted(face_dir.iterdir()):
            if not image_path.is_file():
                continue
            if image_path.suffix.lower() not in SUPPORTED_SUFFIXES:
                continue
            try:
                intensity = int(image_path.stem)
            except ValueError:
                print(f"[warn] skip non-numeric file name: {image_path.name}")
                continue
            if intensity < 0 or intensity > 100:
                print(f"[warn] skip out-of-range intensity: {image_path.name}")
                continue
            entries.append((face, intensity, image_path))
    return entries


def emit_header(header_path: Path) -> None:
    text = """#ifndef GENERATED_FACE_ASSETS_H
#define GENERATED_FACE_ASSETS_H

#include <stdint.h>

#include "emotion.h"

typedef struct {
    FaceType face;
    uint8_t intensity;
    const uint8_t *bitmap;
} GeneratedFaceAsset;

extern const GeneratedFaceAsset g_generated_face_assets[];
extern const uint16_t g_generated_face_asset_count;

#endif
"""
    header_path.write_text(text, encoding="utf-8")


def emit_source(source_path: Path, assets: list[tuple[str, int, Path]], invert: bool) -> None:
    lines: list[str] = [
        '#include "generated_face_assets.h"',
        "",
    ]
    registry_entries: list[str] = []

    if not assets:
        lines.extend(
            [
                "static const uint8_t g_empty_bitmap[1024] = {0};",
                "",
                "const GeneratedFaceAsset g_generated_face_assets[1] = {",
                "    { FACE_UNKNOWN, 0U, g_empty_bitmap },",
                "};",
                "",
                "const uint16_t g_generated_face_asset_count = 0U;",
                "",
            ]
        )
        source_path.write_text("\n".join(lines), encoding="utf-8")
        return

    for face, intensity, image_path in assets:
        bitmap = load_bitmap(image_path, invert=invert)
        array_name = sanitize_identifier(f"bitmap_{face}_{intensity}")
        lines.append(f"static const uint8_t {array_name}[1024] = {{")
        lines.append(format_byte_lines(bitmap))
        lines.append("};")
        lines.append("")
        registry_entries.append(
            f"    {{ FACE_{face.upper()}, {intensity}U, {array_name} }},"
        )

    lines.append("const GeneratedFaceAsset g_generated_face_assets[] = {")
    lines.extend(registry_entries)
    lines.append("};")
    lines.append("")
    lines.append(
        "const uint16_t g_generated_face_asset_count = "
        "sizeof(g_generated_face_assets) / sizeof(g_generated_face_assets[0]);"
    )
    lines.append("")
    source_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    project_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Generate STM32H743 SSD1306 face assets.")
    parser.add_argument(
        "--assets-root",
        type=Path,
        default=project_root / "assets",
        help="Directory containing assets/<face>/<intensity>.*",
    )
    parser.add_argument(
        "--output-inc",
        type=Path,
        default=project_root / "inc" / "generated_face_assets.h",
    )
    parser.add_argument(
        "--output-src",
        type=Path,
        default=project_root / "src" / "generated_face_assets.c",
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert black/white when converting to SSD1306 bitmap.",
    )
    args = parser.parse_args()

    assets_root = args.assets_root.resolve()
    if not assets_root.exists():
        raise SystemExit(f"assets root does not exist: {assets_root}")

    assets = collect_assets(assets_root)
    args.output_inc.parent.mkdir(parents=True, exist_ok=True)
    args.output_src.parent.mkdir(parents=True, exist_ok=True)

    emit_header(args.output_inc)
    emit_source(args.output_src, assets, invert=args.invert)

    print(f"[ok] generated {args.output_inc}")
    print(f"[ok] generated {args.output_src}")
    print(f"[ok] asset count: {len(assets)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
