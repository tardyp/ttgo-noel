#ifndef TTGO_NOEL_SDL_TFT_STUB_H
#define TTGO_NOEL_SDL_TFT_STUB_H

#include <SDL.h>

#include "tft_display.h"

#include <stdint.h>
#include <string>
#include <vector>

class SdlTftDisplay final : public TftDisplay
{
public:
  SdlTftDisplay(int width, int height);
  ~SdlTftDisplay() override;

  void fillScreen(TftColor color) override;
  void fillRect(int x, int y, int width, int height, TftColor color) override;
  void drawRect(int x, int y, int width, int height, TftColor color) override;
  void drawPixel(int x, int y, TftColor color) override;
  TftColor readPixel(int x, int y) const override;
  void setTextColor(TftColor foreground, TftColor background, bool opaque) override;
  void setTextSize(int size) override;
  void drawCentreString(const char *text, int centerX, int y, int font) override;
  void drawString(const char *text, int x, int y, int font) override;

  void present(SDL_Renderer *renderer);
  bool savePng(const std::string &path) const;

private:
  friend class SdlTftSprite;

  int width;
  int height;
  std::vector<TftColor> pixels;
  TftColor textForeground;
  TftColor textBackground;
  bool textOpaque;
  int textSize;
  SDL_Texture *texture;

  void putPixel(int x, int y, TftColor color);
  void drawText(const char *text, int x, int y, int font, bool centered);
  void blit(const std::vector<TftColor> &sprite, int spriteWidth, int spriteHeight,
            int x, int y);
};

class SdlTftSprite final : public TftSprite
{
public:
  explicit SdlTftSprite(SdlTftDisplay &display);

  bool load(const std::string &path, int width, int height, const char *fallbackKind);
  void pushSprite(int x, int y) override;

private:
  SdlTftDisplay &display;
  int width;
  int height;
  std::vector<TftColor> pixels;
};

#endif
