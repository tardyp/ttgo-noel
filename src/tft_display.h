#ifndef TTGO_NOEL_TFT_DISPLAY_H
#define TTGO_NOEL_TFT_DISPLAY_H

#include "game_logic.h"

using TftColor = uint16_t;

class TftSprite
{
public:
  virtual ~TftSprite() = default;
  virtual void pushSprite(int x, int y) = 0;
};

class TftDisplay
{
public:
  virtual ~TftDisplay() = default;

  virtual void fillScreen(TftColor color) = 0;
  virtual void fillRect(int x, int y, int width, int height, TftColor color) = 0;
  virtual void drawRect(int x, int y, int width, int height, TftColor color) = 0;
  virtual void drawPixel(int x, int y, TftColor color) = 0;
  virtual TftColor readPixel(int x, int y) const = 0;
  virtual void setTextColor(TftColor foreground, TftColor background, bool opaque) = 0;
  virtual void setTextSize(int size) = 0;
  virtual void drawCentreString(const char *text, int centerX, int y, int font) = 0;
  virtual void drawString(const char *text, int x, int y, int font) = 0;
};

struct RendererAssets
{
  TftSprite *sleigh[2] = {};
  TftSprite *duck[2] = {};
  TftSprite *foe[2] = {};
  TftSprite *gift = nullptr;
  TftSprite *explosion[2] = {};
  TftSprite *tree[TREE_COUNT] = {};
};

#endif
