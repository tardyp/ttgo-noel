#include "game_renderer.h"
#include "sdl_tft_stub.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <zlib.h>

#define PROGMEM
#define TFT_ESPI_FONT2_DOLLAR
#define TFT_ESPI_GRAVE_IS_DEGREE
namespace tft_eSPI_fonts
{
#include <Fonts/glcdfont.c>
#include <Fonts/Font16.c>
}
#undef TFT_ESPI_GRAVE_IS_DEGREE
#undef TFT_ESPI_FONT2_DOLLAR
#undef PROGMEM

namespace
{
void setPixel(std::vector<TftColor> &pixels, int width, int height,
              int x, int y, TftColor color)
{
  if (x >= 0 && x < width && y >= 0 && y < height)
    pixels[static_cast<size_t>(y) * width + x] = color;
}

void fillPixels(std::vector<TftColor> &pixels, TftColor color)
{
  std::fill(pixels.begin(), pixels.end(), color);
}

void fillPixelsRect(std::vector<TftColor> &pixels, int width, int height,
                    int x, int y, int rectWidth, int rectHeight, TftColor color)
{
  for (int py = std::max(0, y); py < std::min(height, y + rectHeight); ++py)
  {
    for (int px = std::max(0, x); px < std::min(width, x + rectWidth); ++px)
    {
      pixels[static_cast<size_t>(py) * width + px] = color;
    }
  }
}

void fallbackSprite(std::vector<TftColor> &pixels, int width, int height, const char *kind)
{
  fillPixels(pixels, SKY_BLUE);
  const std::string type(kind);
  if (type == "sleigh")
  {
    fillPixelsRect(pixels, width, height, 2, 2, width - 4, height - 4, 0xF800);
    fillPixelsRect(pixels, width, height, 4, 0, 6, 4, 0x07E0);
  }
  else if (type == "duck")
  {
    fillPixelsRect(pixels, width, height, 3, 5, 12, 7, DUCK_YELLOW);
    fillPixelsRect(pixels, width, height, 10, 3, 7, 7, DUCK_YELLOW);
    fillPixelsRect(pixels, width, height, 17, 4, 3, 3, 0xFD20);
  }
  else if (type == "foe")
  {
    fillPixelsRect(pixels, width, height, 4, 2, 12, 10, 0x0000);
    fillPixelsRect(pixels, width, height, 7, 4, 3, 3, 0xF800);
  }
  else if (type == "tree")
  {
    const int trunkWidth = 6;
    const int trunkHeight = height / 4;
    fillPixelsRect(pixels, width, height, width / 2 - trunkWidth / 2,
                   height - trunkHeight, trunkWidth, trunkHeight, 0x7140);
    const int layerHeight = (height - trunkHeight) / 3;
    for (int layer = 0; layer < 3; ++layer)
    {
      const int layerWidth = width - layer * 4;
      const int layerY = trunkHeight + layer * layerHeight;
      for (int row = 0; row < layerHeight; ++row)
      {
        const int halfWidth = std::max(1, layerWidth * (row + 1) / layerHeight / 2);
        fillPixelsRect(pixels, width, height, width / 2 - halfWidth,
                       layerY + row, halfWidth * 2 + 1, 1, TREE_GREEN);
      }
    }
  }
  else
  {
    fillPixelsRect(pixels, width, height, 4, 2, width - 8, height - 4, 0xF800);
  }
}

uint8_t expandRed(uint16_t color)
{
  return static_cast<uint8_t>(((color >> 11) & 0x1F) * 255 / 31);
}

uint8_t expandGreen(uint16_t color)
{
  return static_cast<uint8_t>(((color >> 5) & 0x3F) * 255 / 63);
}

uint8_t expandBlue(uint16_t color)
{
  return static_cast<uint8_t>((color & 0x1F) * 255 / 31);
}

void appendU32(std::vector<uint8_t> &output, uint32_t value)
{
  output.push_back(static_cast<uint8_t>(value >> 24));
  output.push_back(static_cast<uint8_t>(value >> 16));
  output.push_back(static_cast<uint8_t>(value >> 8));
  output.push_back(static_cast<uint8_t>(value));
}

void appendChunk(std::vector<uint8_t> &png, const char type[4], const std::vector<uint8_t> &payload)
{
  appendU32(png, static_cast<uint32_t>(payload.size()));
  const size_t typeOffset = png.size();
  png.insert(png.end(), type, type + 4);
  png.insert(png.end(), payload.begin(), payload.end());
  const uLong crcStart = crc32(0L, Z_NULL, 0);
  uLong crc = crc32(crcStart, png.data() + typeOffset,
                    static_cast<uInt>(4 + payload.size()));
  appendU32(png, static_cast<uint32_t>(crc));
}
} // namespace

SdlTftDisplay::SdlTftDisplay(int width, int height)
    : width(width), height(height), pixels(static_cast<size_t>(width) * height, 0),
      textForeground(WHITE), textBackground(SKY_BLUE), textOpaque(true), textSize(1), texture(nullptr)
{
}

SdlTftDisplay::~SdlTftDisplay()
{
  if (texture != nullptr)
    SDL_DestroyTexture(texture);
}

void SdlTftDisplay::putPixel(int x, int y, TftColor color)
{
  setPixel(pixels, width, height, x, y, color);
}

void SdlTftDisplay::fillScreen(TftColor color)
{
  fillPixels(pixels, color);
}

void SdlTftDisplay::fillRect(int x, int y, int rectWidth, int rectHeight, TftColor color)
{
  fillPixelsRect(pixels, width, height, x, y, rectWidth, rectHeight, color);
}

void SdlTftDisplay::drawRect(int x, int y, int rectWidth, int rectHeight, TftColor color)
{
  for (int px = x; px < x + rectWidth; ++px)
  {
    putPixel(px, y, color);
    putPixel(px, y + rectHeight - 1, color);
  }
  for (int py = y; py < y + rectHeight; ++py)
  {
    putPixel(x, py, color);
    putPixel(x + rectWidth - 1, py, color);
  }
}

void SdlTftDisplay::drawPixel(int x, int y, TftColor color)
{
  putPixel(x, y, color);
}

TftColor SdlTftDisplay::readPixel(int x, int y) const
{
  if (x < 0 || x >= width || y < 0 || y >= height)
    return 0;
  return pixels[static_cast<size_t>(y) * width + x];
}

void SdlTftDisplay::setTextColor(TftColor foreground, TftColor background, bool opaque)
{
  textForeground = foreground;
  textBackground = background;
  textOpaque = opaque;
}

void SdlTftDisplay::setTextSize(int size)
{
  textSize = std::max(1, size);
}

void SdlTftDisplay::drawText(const char *text, int x, int y, int font, bool centered)
{
  if (text == nullptr)
    return;

  const std::string value(text);
  auto advance = [this, font](unsigned char character) {
    if (font == 2 && character >= 32 && character <= 127)
      return static_cast<int>(tft_eSPI_fonts::widtbl_f16[character - 32]) * textSize;
    return font == 1 ? 6 * textSize : 0;
  };

  int totalWidth = 0;
  for (unsigned char character : value)
    totalWidth += advance(character);
  if (centered)
    x -= totalWidth / 2;

  for (unsigned char character : value)
  {
    const int characterWidth = advance(character);
    if (characterWidth == 0)
      continue;

    const int characterHeight = font == 2 ? 16 * textSize : 8 * textSize;
    if (textOpaque)
      fillRect(x, y, characterWidth, characterHeight, textBackground);

    if (font == 2 && character >= 32 && character <= 127)
    {
      const int glyphWidth = tft_eSPI_fonts::widtbl_f16[character - 32];
      const int bytesPerRow = (glyphWidth + 6) / 8;
      const unsigned char *bitmap = tft_eSPI_fonts::chrtbl_f16[character - 32];
      for (int row = 0; row < 16; ++row)
      {
        for (int column = 0; column < glyphWidth; ++column)
        {
          const unsigned char bits = bitmap[row * bytesPerRow + column / 8];
          if ((bits & (0x80 >> (column % 8))) != 0)
            fillRect(x + column * textSize, y + row * textSize,
                     textSize, textSize, textForeground);
        }
      }
    }
    else if (font == 1)
    {
      const unsigned char *bitmap = tft_eSPI_fonts::font + character * 5;
      for (int column = 0; column < 5; ++column)
      {
        unsigned char bits = bitmap[column];
        for (int row = 0; row < 8; ++row)
        {
          if ((bits & 0x01) != 0)
            fillRect(x + column * textSize, y + row * textSize,
                     textSize, textSize, textForeground);
          bits >>= 1;
        }
      }
    }
    x += characterWidth;
  }
}

void SdlTftDisplay::drawCentreString(const char *text, int centerX, int y, int font)
{
  drawText(text, centerX, y, font, true);
}

void SdlTftDisplay::drawString(const char *text, int x, int y, int font)
{
  drawText(text, x, y, font, false);
}

void SdlTftDisplay::blit(const std::vector<TftColor> &sprite, int spriteWidth,
                         int spriteHeight, int x, int y)
{
  for (int py = 0; py < spriteHeight; ++py)
  {
    for (int px = 0; px < spriteWidth; ++px)
    {
      putPixel(x + px, y + py, sprite[static_cast<size_t>(py) * spriteWidth + px]);
    }
  }
}

void SdlTftDisplay::present(SDL_Renderer *renderer)
{
  if (texture == nullptr)
  {
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, width, height);
  }
  if (texture == nullptr)
    return;

  std::vector<uint32_t> argbPixels(static_cast<size_t>(width) * height);
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    argbPixels[i] = 0xFF000000u |
                    (static_cast<uint32_t>(expandRed(pixels[i])) << 16) |
                    (static_cast<uint32_t>(expandGreen(pixels[i])) << 8) |
                    expandBlue(pixels[i]);
  }
  SDL_UpdateTexture(texture, nullptr, argbPixels.data(), width * static_cast<int>(sizeof(uint32_t)));
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

bool SdlTftDisplay::savePng(const std::string &path) const
{
  std::vector<uint8_t> raw;
  raw.reserve(static_cast<size_t>(height) * (1 + width * 3));
  for (int y = 0; y < height; ++y)
  {
    raw.push_back(0);
    for (int x = 0; x < width; ++x)
    {
      const TftColor color = readPixel(x, y);
      raw.push_back(expandRed(color));
      raw.push_back(expandGreen(color));
      raw.push_back(expandBlue(color));
    }
  }

  uLong compressedSize = compressBound(static_cast<uLong>(raw.size()));
  std::vector<uint8_t> compressed(compressedSize);
  if (compress2(compressed.data(), &compressedSize, raw.data(),
                static_cast<uLong>(raw.size()), Z_BEST_SPEED) != Z_OK)
  {
    return false;
  }
  compressed.resize(compressedSize);

  std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  std::vector<uint8_t> header;
  appendU32(header, static_cast<uint32_t>(width));
  appendU32(header, static_cast<uint32_t>(height));
  header.push_back(8);  // bit depth
  header.push_back(2);  // truecolor RGB
  header.push_back(0);  // compression
  header.push_back(0);  // filter
  header.push_back(0);  // interlace
  appendChunk(png, "IHDR", header);
  appendChunk(png, "IDAT", compressed);
  appendChunk(png, "IEND", {});

  std::ofstream file(path, std::ios::binary);
  if (!file)
    return false;
  file.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
  return file.good();
}

SdlTftSprite::SdlTftSprite(SdlTftDisplay &display)
    : display(display), width(0), height(0)
{
}

bool SdlTftSprite::load(const std::string &path, int width, int height, const char *fallbackKind)
{
  this->width = width;
  this->height = height;
  pixels.assign(static_cast<size_t>(width) * height, SKY_BLUE);

  std::ifstream file(path, std::ios::binary);
  std::vector<uint8_t> bytes(static_cast<size_t>(width) * height * 2);
  if (file)
  {
    file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (file.gcount() == static_cast<std::streamsize>(bytes.size()))
    {
      for (size_t i = 0; i < pixels.size(); ++i)
        pixels[i] = static_cast<TftColor>(bytes[i * 2] << 8 | bytes[i * 2 + 1]);
      return true;
    }
  }

  fallbackSprite(pixels, width, height, fallbackKind);
  std::cerr << "SDL TFT stub: using fallback for " << path << '\n';
  return false;
}

void SdlTftSprite::pushSprite(int x, int y)
{
  display.blit(pixels, width, height, x, y);
}
