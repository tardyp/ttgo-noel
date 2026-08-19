# Sprite Converter Guide

Convert PNG images to RGB565 binary assets for the TFT_eSPI sprites used by the firmware.

## Sprite Dimensions

- Sleigh, duck, foe, and explosion frames: 20x14 pixels
- Gift frame: 13x14 pixels
- Optional tree asset: 20x42 pixels (`TREE_HEIGHT` on the 320x170 display)

The firmware has procedural fallbacks for missing assets. The current `data/` directory contains binary assets but not the original PNG sources.

## Game Color Palette

Use these colors when creating sprites to match the game background:

| Element | RGB565 | RGB888 (Hex) | Description |
|---------|--------|--------------|-------------|
| Sky Background | 0x3A9F | #3850F8 | Light blue sky |
| Ground/Grass | 0x2589 | #20B048 | Green grass |
| Tree Foliage | 0x2444 | #208420 | Dark green trees |
| Tree Trunk | 0x7140 | #E05000 | Brown trunk |
| Sleigh | 0xF800 | #F80000 | Bright red |
| Duck | 0xFFE0 | #F8F800 | Yellow |
| White (Text) | 0xFFFF | #F8FCF8 | Off-white |

Transparent pixels are replaced with the specified background color. Use `#3850F8` for sky or `#20B048` for grass, depending on where the sprite appears.

## Python Workflow

Install the declared Python dependency in the Python 3.12 environment:

```bash
python3 -m pip install 'Pillow>=12'
```

The converter accepts `--output`; a second positional output filename is not valid.

```bash
# Single frame; writes data/sleigh0.bin
python3 convert_sprite.py sleigh.png -o data/sleigh

# Two rows; writes data/duck0.bin and data/duck1.bin
python3 convert_sprite.py duck.png --rows 2 -o data/duck --bg '#3850F8'

# Optional tree asset
python3 convert_sprite.py tree.png -o data/tree --bg '#20B048'
```

Output files are named `<output-base>0.bin`, `<output-base>1.bin`, and so on. Each pixel is RGB565, stored big-endian on disk so the ESP32 loader and TFT_eSPI parallel output produce the expected panel byte order.

## Upload to SPIFFS

1. Place generated `.bin` files in the project `data/` directory using the filenames expected by `src/main.cpp`.
2. Build and flash the firmware:

```bash
pio run -e lilygo-t-display-s3 -t upload
pio run -e lilygo-t-display-s3 -t uploadfs
```

Re-run `uploadfs` after changing any asset. The firmware loads sleigh, duck, foe, gift, and explosion files at startup; `/tree.bin` is optional because a procedural tree is available as fallback.

## Verify Sprite Files

```bash
xxd data/sleigh0.bin | head -20
```

The output should contain two bytes per RGB565 pixel. Confirm colors on the actual display after uploading; host-side byte inspection cannot verify the panel rendering or SPIFFS load path.
