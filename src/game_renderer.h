#ifndef TTGO_NOEL_GAME_RENDERER_H
#define TTGO_NOEL_GAME_RENDERER_H

#include "game_logic.h"
#include "tft_display.h"

constexpr TftColor SKY_BLUE = 0x3A9F;
constexpr TftColor GROUND_GREEN = 0x2589;
constexpr TftColor TREE_GREEN = 0x2444;
constexpr TftColor TREE_BROWN = 0x7140;
constexpr TftColor SLEIGH_RED = 0xF800;
constexpr TftColor DUCK_YELLOW = 0xFFE0;
constexpr TftColor WHITE = 0xFFFF;

class GameRenderer
{
public:
  GameRenderer(TftDisplay &display, const RendererAssets &assets);

  void drawMenu(const GameData &data, uint32_t now);
  void drawGameplay(GameData &data, uint32_t now);
  void drawGameOver(Game &game);

private:
  TftDisplay &display;
  const RendererAssets &assets;

  static bool snowBlocked(int x, int y, void *context);
  void drawSnowPixel(int x, int y, TftColor color);
  void pushSprite(TftSprite *sprite, int x, int y);
};

#endif
