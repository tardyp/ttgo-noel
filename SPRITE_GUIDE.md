# Sprite Converter Guide

This directory contains scripts to convert PNG images to RGB565 binary format for TFT_eSPI sprites.

## Required Sprite Sizes

- **sleigh.png**: 20x14 pixels
- **duck.png**: 20x14 pixels  
- **tree.png**: 20x60 pixels

## Game Color Palette

Use these colors when creating your sprites to match the game background:

| Element | RGB565 | RGB888 (Hex) | Description |
|---------|--------|--------------|-------------|
| Sky Background | 0x3A9F | #3850F8 | Light blue sky |
| Ground/Grass | 0x2589 | #20B048 | Green grass |
| Tree Foliage | 0x2444 | #208420 | Dark green trees |
| Tree Trunk | 0x7140 | #E05000 | Brown trunk |
| Sleigh | 0xF800 | #F80000 | Bright red |
| Duck | 0xFFE0 | #F8F800 | Yellow |
| White (Text) | 0xFFFF | #F8FCF8 | Off-white |

**Example:** If creating a duck sprite with a transparent background, set the background to **#3850F8** (sky blue) or **#20B048** (grass green) depending on where the duck appears.

## Python Method (Recommended)

### Install dependencies:
```bash
pip install Pillow
```

### Usage:
```bash
# Convert with auto-generated output name
python3 convert_sprite.py sleigh.png

# Specify output filename
python3 convert_sprite.py duck.png duck.bin

# Specify custom background color
python3 convert_sprite.py duck.png duck.bin --bg #3850F8
```

### Batch convert all sprites:
```bash
python3 convert_sprite.py sleigh.png --bg #3850F8
python3 convert_sprite.py duck.png --bg #3850F8
python3 convert_sprite.py tree.png --bg #20B048
```

## Shell Script Method (Alternative)

Requires ImageMagick:
```bash
brew install imagemagick
```

### Usage:
```bash
./convert_sprite.sh sleigh.png
./convert_sprite.sh duck.png
./convert_sprite.sh tree.png
```

## Notes

- Transparent pixels are replaced with the specified background color
- Output is in RGB565 format (16-bit per pixel)
- Binary files are little-endian encoded
- Upload .bin files to SPIFFS using PlatformIO

## Upload to SPIFFS

1. Create `data` folder in project root (if not already created)
2. Place .bin files in the data folder
3. Run: `pio run --target uploadfs`

## Verifying Sprite Files

To check your converted sprite file:
```bash
xxd your_sprite.bin | head -20
```

Each line should show hex values representing RGB565 pixel data.

