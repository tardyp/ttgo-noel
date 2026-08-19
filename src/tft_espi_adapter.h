#ifndef TTGO_NOEL_TFT_ESPI_ADAPTER_H
#define TTGO_NOEL_TFT_ESPI_ADAPTER_H

#include <TFT_eSPI.h>

#include "tft_display.h"

class TftEspiDisplay final : public TftDisplay
{
public:
  explicit TftEspiDisplay(TFT_eSPI &display)
      : display(display)
  {
  }

  void fillScreen(TftColor color) override { display.fillScreen(color); }
  void fillRect(int x, int y, int width, int height, TftColor color) override
  {
    display.fillRect(x, y, width, height, color);
  }
  void drawRect(int x, int y, int width, int height, TftColor color) override
  {
    display.drawRect(x, y, width, height, color);
  }
  void drawPixel(int x, int y, TftColor color) override { display.drawPixel(x, y, color); }
  TftColor readPixel(int x, int y) const override
  {
    return const_cast<TFT_eSPI &>(display).readPixel(x, y);
  }
  void setTextColor(TftColor foreground, TftColor background, bool opaque) override
  {
    display.setTextColor(foreground, background, opaque);
  }
  void setTextSize(int size) override { display.setTextSize(size); }
  void drawCentreString(const char *text, int centerX, int y, int font) override
  {
    display.drawCentreString(text, centerX, y, font);
  }
  void drawString(const char *text, int x, int y, int font) override
  {
    display.drawString(text, x, y, font);
  }

private:
  TFT_eSPI &display;
};

class TftEspiSprite final : public TftSprite
{
public:
  explicit TftEspiSprite(TFT_eSprite &sprite)
      : sprite(sprite)
  {
  }

  void pushSprite(int x, int y) override { sprite.pushSprite(x, y); }

private:
  TFT_eSprite &sprite;
};

#endif
