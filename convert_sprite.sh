#!/bin/bash
# PNG to RGB565 Binary Converter using ImageMagick
# Alternative shell script if Python/Pillow is not available

if [ $# -lt 1 ]; then
    echo "Usage: $0 <input.png> [output.bin]"
    echo ""
    echo "Examples:"
    echo "  $0 sleigh.png"
    echo "  $0 duck.png duck.bin"
    echo ""
    echo "Predefined sprite sizes:"
    echo "  sleigh.png  -> 12x12 pixels"
    echo "  duck.png    -> 20x14 pixels"
    echo "  tree.png    -> 20x60 pixels"
    echo ""
    echo "Note: Requires ImageMagick (install with: brew install imagemagick)"
    exit 1
fi

INPUT="$1"
OUTPUT="${2:-${INPUT%.*}.bin}"

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick 'convert' command not found"
    echo "Install with: brew install imagemagick"
    exit 1
fi

if [ ! -f "$INPUT" ]; then
    echo "Error: File '$INPUT' not found"
    exit 1
fi

echo "Converting $INPUT to RGB565 format..."

# Convert PNG to RGB format, then to RGB565 binary
# This uses ImageMagick to convert to RGB raw format
# Sky blue background (0x3A9F = RGB 58, 159, 248 approximately)
convert "$INPUT" \
    -background "#3AA0F8" \
    -alpha remove \
    -depth 8 \
    RGB:- | \
python3 -c "
import sys
import struct

# Read RGB data
data = sys.stdin.buffer.read()

# Convert RGB888 to RGB565
with open('$OUTPUT', 'wb') as f:
    for i in range(0, len(data), 3):
        r, g, b = data[i:i+3]
        # RGB565 format
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        f.write(struct.pack('<H', rgb565))

print(f'Successfully converted to {sys.argv[1]}', file=sys.stderr)
" "$OUTPUT"

SIZE=$(stat -f%z "$OUTPUT" 2>/dev/null || stat -c%s "$OUTPUT" 2>/dev/null)
echo "Output size: $SIZE bytes"
echo "Done!"
