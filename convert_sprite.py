#!/usr/bin/env python3
"""
PNG to RGB565 Binary Converter for TFT_eSPI Sprites
Converts PNG images to raw 16-bit RGB565 binary format
Supports sprite sheets organized by rows
"""

import sys
import argparse
from PIL import Image
import struct

def rgb888_to_rgb565(r, g, b):
    """Convert 8-bit RGB (888) to 16-bit RGB565 format"""
    # RGB565: RRRRRGGG GGGBBBBB (bit layout)
    # Shift and mask to 5/6/5 bits
    r5 = (r >> 3) & 0x1F  # 5 bits for red
    g6 = (g >> 2) & 0x3F  # 6 bits for green
    b5 = (b >> 3) & 0x1F  # 5 bits for blue

    # Combine: bits 15-11=R, 10-5=G, 4-0=B
    rgb565 = (r5 << 11) | (g6 << 5) | b5
    return rgb565

def hex_to_rgb(hex_color):
    """Convert hex color string to RGB tuple"""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def convert_png_to_bin(input_png, output_base, rows=1, transparent_color=None):
    """
    Convert PNG to RGB565 binary format.
    Supports sprite sheets organized by rows.

    Args:
        input_png: Input PNG file path
        output_base: Output base path without extension (e.g., "data/sleigh")
                     For sprite sheets with N rows, creates sleigh0.bin, sleigh1.bin, etc.
        rows: Number of rows in the sprite sheet (default: 1 for single sprite)
              Total frames = rows (assumes 1 frame per row)
        transparent_color: Optional hex string (e.g., "#3090A0") or RGB tuple
                          Default is sky blue matching game background
    """
    try:
        # Open the PNG image
        img = Image.open(input_png)

        # Convert to RGBA to handle transparency
        img = img.convert('RGBA')

        width, height = img.size
        print(f"Image size: {width}x{height}")

        if rows < 1:
            print("Error: Number of rows must be at least 1")
            sys.exit(1)

        # Calculate frame dimensions
        frame_width = width
        frame_height = height // rows

        print(f"Sprite sheet with {rows} row(s), each frame: {frame_width}x{frame_height}")

        # Default transparent color to sky blue
        if transparent_color is None:
            transparent_color = (48, 144, 160)  # #3090A0
        elif isinstance(transparent_color, str):
            transparent_color = hex_to_rgb(transparent_color)

        print(f"Transparent color: RGB{transparent_color}")
        rgb565_preview = rgb888_to_rgb565(*transparent_color)
        print(f"RGB565 value: 0x{rgb565_preview:04X}")

        # Process each row as a separate frame
        for frame_idx in range(rows):
            output_bin = f"{output_base}{frame_idx}.bin"

            # Extract the region for this frame
            top = frame_idx * frame_height
            bottom = top + frame_height
            frame_region = (0, top, frame_width, bottom)
            frame = img.crop(frame_region)

            # Convert to binary RGB565 format
            with open(output_bin, 'wb') as f:
                for y in range(frame_height):
                    for x in range(frame_width):
                        r, g, b, a = frame.getpixel((x, y))

                        # Handle transparency - replace with transparent color
                        if a < 128:  # Semi-transparent or fully transparent
                            r, g, b = transparent_color

                        # Convert to RGB565
                        rgb565 = rgb888_to_rgb565(r, g, b)

                        # Write as big-endian 16-bit value
                        f.write(struct.pack('>H', rgb565))

            print(f"Converted frame {frame_idx} to {output_bin} ({frame_width * frame_height * 2} bytes)")

        if rows == 1:
            print(f"Successfully converted {input_png} to {output_base}0.bin")
        else:
            print(f"Successfully converted {input_png} to {rows} frames ({output_base}0.bin through {output_base}{rows-1}.bin)")

    except FileNotFoundError:
        print(f"Error: File '{input_png}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(
        description='Convert PNG sprite sheets to RGB565 binary format for TFT_eSPI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''Examples:
  python convert_sprite.py sleigh.png
  python convert_sprite.py sleigh.png --rows 2
  python convert_sprite.py sleigh.png --rows 2 --bg #3090A0
  python convert_sprite.py duck.png --rows 2 --output data/duck
        '''
    )

    parser.add_argument('input', help='Input PNG file path')
    parser.add_argument('-r', '--rows', type=int, default=1,
                        help='Number of rows in sprite sheet (default: 1)')
    parser.add_argument('-o', '--output', help='Output base path (default: data/<filename>)')
    parser.add_argument('-bg', '--background', '--bg', dest='background',
                        help='Background color for transparency as hex (e.g., #3090A0)')

    args = parser.parse_args()

    # Determine output base path
    if args.output:
        output_base = args.output
    else:
        output_base = "data/" + args.input.rsplit('.', 1)[0]

    convert_png_to_bin(args.input, output_base, rows=args.rows, transparent_color=args.background)

if __name__ == "__main__":
    main()

