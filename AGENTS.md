# Repository Guidelines

## Project Overview

`ttgo-noel` is a PlatformIO/Arduino game for the LilyGo T-Display-S3 (ESP32-S3). It renders a Christmas-themed Flappy Bird-style game on the 320×170 ST7789 display, reads buttons, stores per-mode high scores in Preferences, and loads sprite binaries from SPIFFS. The firmware is the primary project; the Python files only support sprite conversion and contain an unrelated stub entry point.

## Architecture & Data Flow

- Firmware is one translation unit: `src/main.cpp` with Arduino `setup()` and `loop()`.
- `setup()` enables the display power/backlight and GPIO inputs, opens Preferences, initializes TFT_eSPI, mounts/loads SPIFFS sprites, initializes `GameData`, and paints the sky/ground.
- `loop()` debounces button edges, then dispatches by `GameState`: `STATE_MENU`, `STATE_PLAYING`, or `STATE_GAME_OVER`.
- The playing path runs in this order: physics → obstacle movement/spawn → flying animation → collision handling → scoring → rendering, with a 30 ms delay.
- `GameData` is the central mutable state: sleigh physics/crash state, mode and scores, trees, flying obstacles (duck/foe/gift), and snowflakes. Global `TFT_eSPI`, `Preferences`, and `TFT_eSprite` objects are used directly; there are no injected interfaces or test seams.
- `loadSpritesFromSPIFFS()` expects `/sleigh0/1.bin`, `/duck0/1.bin`, `/foe0/1.bin`, `/gift0.bin`, `/explosion0/1.bin`, and optionally `/tree.bin`. Missing assets use procedural fallback sprites. Asset changes require rebuilding/uploading the filesystem, not only flashing firmware.
- High scores are loaded from and written to the `flappysleigh` Preferences namespace (`highscore0`–`highscore2`).

## Key Directories

- `src/`: firmware; currently only `main.cpp`.
- `include/`: shared headers; `User_Setup.h` is the TFT_eSPI hardware setup.
- `data/`: binary RGB565 assets uploaded to SPIFFS.
- `lib/`: reserved for project-private PlatformIO libraries; currently only template guidance.
- `test/`: reserved for PlatformIO tests; currently only template guidance.
- `.pio/`: generated PlatformIO build/dependency output; ignored and disposable.
- `.vscode/`: PlatformIO/editor metadata; generated debug/index files are not portable.

## Development Commands

```bash
# Build the configured firmware environment
pio run -e lilygo-t-display-s3

# Flash firmware, then upload SPIFFS assets when data/ changed
pio run -e lilygo-t-display-s3 -t upload
pio run -e lilygo-t-display-s3 -t uploadfs

# Monitor setup()'s Serial.begin(115200); provide --port when autodetection is ambiguous
pio device monitor -b 115200

# Convert a two-row sprite sheet; quote # colors in shells that treat # as a comment
python3 convert_sprite.py sleigh.png --rows 2 -o data/sleigh --bg '#3850F8'

# Inspect generated binary bytes
xxd data/sleigh0.bin | head -20
```

The active PlatformIO environment is `lilygo-t-display-s3`; do not copy the stale `esp32dev` environment from generated `.vscode/launch.json` or `c_cpp_properties.json`. `pio run -t uploadfs` is coupled to runtime SPIFFS asset loading.

## Code Conventions & Common Patterns

- Match the existing C++ style in `src/main.cpp`: two-space indentation, braces on the following line, section banners, `PascalCase` for structs, `camelCase` for functions/fields, and uppercase `SCREAMING_SNAKE_CASE` macros/constants/enumerators.
- Keep game transitions in the existing state machine and keep `GameData` fields initialized/reset consistently. `gameMode` intentionally survives game-over reset; global zero-initialization makes the initial mode normal.
- Use `millis()` for animation/spawn timing and preserve the fixed update/render order. Avoid adding blocking timing beyond the existing loop delay.
- Hardware and library objects are global. SPIFFS access follows `exists()` → `open()` → read/push image → close, with procedural sprite fallbacks and `Serial.println()` diagnostics on mount failure.
- Collision dimensions are deliberately separate from sprite dimensions (`SLEIGH_HITBOX`/`DUCK_HITBOX`); update collision logic and rendering together.
- Prefer existing TFT_eSPI drawing/sprite primitives and RGB565 constants. Avoid introducing a second state or asset-loading abstraction without a concrete need.

## Important Files

- `src/main.cpp`: all firmware constants, state, input, physics, collision, scoring, SPIFFS loading, and rendering.
- `include/User_Setup.h`: LilyGo T-Display-S3 ST7789V 8-bit parallel bus, 170×320 panel dimensions, GPIO pin map, backlight, and enabled fonts.
- `platformio.ini`: sole build environment, Arduino framework, TFT_eSPI dependency, and forced `User_Setup.h` include.
- `data/*.bin`: deployed sprite assets and expected SPIFFS filenames.
- `convert_sprite.py`: Pillow-based PNG → RGB565 binary converter (`--rows`, `--output/-o`, `--bg`).
- `SPRITE_GUIDE.md`: asset workflow and upload instructions; verify examples against the converter before relying on them.
- `pyproject.toml`, `.python-version`, `uv.lock`: Python 3.12/Pillow tooling metadata.

## Runtime/Tooling Preferences

- Firmware requires PlatformIO CLI (`pio`), the `espressif32` platform, Arduino framework, and a LilyGo T-Display-S3. The PlatformIO IDE extension is recommended by `.vscode/extensions.json` but is optional.
- Sprite conversion requires Python 3.12 and Pillow 12.x or newer. Use the repository's locked Python metadata (`pyproject.toml`, `.python-version`, `uv.lock`) when setting up an environment; no Node/Bun runtime is involved.
- Treat `.pio/` and generated VS Code indexing/debug files as disposable. `.vscode/launch.json` and `c_cpp_properties.json` contain absolute paths and stale `esp32dev` references.
- Keep device-specific upload ports outside committed configuration unless the project explicitly standardizes one.

## Testing & QA

There are no project test sources, fixtures, mocks, CI jobs, or coverage configuration. `test/README` is only the stock PlatformIO test-runner template, so `pio test -e lilygo-t-display-s3` has no project tests to execute.

For firmware changes, verify on hardware:

1. `pio run -e lilygo-t-display-s3` compiles the firmware.
2. Upload firmware and SPIFFS with the commands above; re-upload SPIFFS after any `data/` change.
3. Open the 115200-baud monitor and exercise menu mode cycling, start/jump input, collisions, game-over restart, and high-score persistence.
4. For sprite changes, inspect output bytes and confirm rendered colors on-device. The converter writes big-endian RGB565 bytes because the ESP32 loader reads them into `uint16_t` before TFT_eSPI's parallel output swaps them for the panel.

No host-level automated regression currently covers TFT/GPIO/SPIFFS/Preferences behavior. `main.py` only prints a greeting and is not the firmware test or entry point.
