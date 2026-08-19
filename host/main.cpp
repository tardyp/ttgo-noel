#include <SDL.h>

#include "game_logic.h"
#include "game_renderer.h"
#include "sdl_tft_stub.h"

#include <cstdlib>
#include <iostream>
#include <string>

#ifndef TTGO_NOEL_DATA_DIR
#define TTGO_NOEL_DATA_DIR "data"
#endif

namespace
{
constexpr int FRAME_MS = 30;

bool parseIntArgument(int argc, char **argv, const char *name, int &value)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string(argv[i]) == name)
    {
      value = std::atoi(argv[i + 1]);
      return true;
    }
  }
  return false;
}

std::string parseStringArgument(int argc, char **argv, const char *name)
{
  for (int i = 1; i + 1 < argc; ++i)
  {
    if (std::string(argv[i]) == name)
      return argv[i + 1];
  }
  return {};
}
bool hasFlag(int argc, char **argv, const char *name)
{
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == name)
      return true;
  }
  return false;
}

void printUsage(const char *program)
{
  std::cout << "Usage: " << program << " [--play] [--seed N] [--frames N] [--screenshot PATH]\n"
            << "  --play              open the interactive SDL game window\n"
            << "  --seed N            choose deterministic game randomness\n"
            << "  --frames N          exit after N rendered frames\n"
            << "  --screenshot PATH   save the final 320x170 frame as PNG\n"
            << "Controls: Space/Enter/Up/A action, M/Tab mode, Escape quit\n";
}

void loadSprites(SdlTftSprite (&sleigh)[2], SdlTftSprite (&duck)[2],
                 SdlTftSprite (&foe)[2], SdlTftSprite &gift,
                 SdlTftSprite (&explosion)[2], SdlTftSprite (&tree)[TREE_COUNT])
{
  sleigh[0].load(std::string(TTGO_NOEL_DATA_DIR) + "/sleigh0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "sleigh");
  sleigh[1].load(std::string(TTGO_NOEL_DATA_DIR) + "/sleigh1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "sleigh");
  duck[0].load(std::string(TTGO_NOEL_DATA_DIR) + "/duck0.bin", DUCK_WIDTH, DUCK_HEIGHT, "duck");
  duck[1].load(std::string(TTGO_NOEL_DATA_DIR) + "/duck1.bin", DUCK_WIDTH, DUCK_HEIGHT, "duck");
  foe[0].load(std::string(TTGO_NOEL_DATA_DIR) + "/foe0.bin", DUCK_WIDTH, DUCK_HEIGHT, "foe");
  foe[1].load(std::string(TTGO_NOEL_DATA_DIR) + "/foe1.bin", DUCK_WIDTH, DUCK_HEIGHT, "foe");
  gift.load(std::string(TTGO_NOEL_DATA_DIR) + "/gift0.bin", GIFT_WIDTH, GIFT_HEIGHT, "gift");
  explosion[0].load(std::string(TTGO_NOEL_DATA_DIR) + "/explosion0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "explosion");
  explosion[1].load(std::string(TTGO_NOEL_DATA_DIR) + "/explosion1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "explosion");

  // There is no tree.bin in the repository; the stub's fallback is used.
  for (int i = 0; i < TREE_COUNT; ++i)
    tree[i].load(std::string(TTGO_NOEL_DATA_DIR) + "/tree.bin", TREE_WIDTH, TREE_HEIGHT, "tree");
}

void clearFrame(SdlTftDisplay &display)
{
  display.fillScreen(SKY_BLUE);
  display.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
}

} // namespace

int main(int argc, char **argv)
{
  int maxFrames = -1;
  int seed = 1;
  const bool playMode = hasFlag(argc, argv, "--play");
  if (hasFlag(argc, argv, "--help"))
  {
    printUsage(argv[0]);
    return 0;
  }
  const bool framesSpecified = parseIntArgument(argc, argv, "--frames", maxFrames);
  parseIntArgument(argc, argv, "--seed", seed);
  const std::string screenshotPath = parseStringArgument(argc, argv, "--screenshot");
  if (playMode && !framesSpecified)
    maxFrames = -1;
  else if (!screenshotPath.empty() && !framesSpecified && maxFrames < 0)
    maxFrames = 1;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
    return 1;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  SDL_Window *window = SDL_CreateWindow("TTGO Noel - SDL TFT Harness",
                                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3,
                                         SDL_WINDOW_RESIZABLE);
  if (window == nullptr)
  {
    std::cerr << "SDL window creation failed: " << SDL_GetError() << '\n';
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  if (renderer == nullptr)
  {
    std::cerr << "SDL renderer creation failed: " << SDL_GetError() << '\n';
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

  int result = 0;
  {
    SdlTftDisplay display(SCREEN_WIDTH, SCREEN_HEIGHT);
    SdlTftSprite sleigh[2] = {SdlTftSprite(display), SdlTftSprite(display)};
    SdlTftSprite duck[2] = {SdlTftSprite(display), SdlTftSprite(display)};
    SdlTftSprite foe[2] = {SdlTftSprite(display), SdlTftSprite(display)};
    SdlTftSprite gift(display);
    SdlTftSprite explosion[2] = {SdlTftSprite(display), SdlTftSprite(display)};
    SdlTftSprite tree[TREE_COUNT] = {
        SdlTftSprite(display), SdlTftSprite(display), SdlTftSprite(display),
        SdlTftSprite(display), SdlTftSprite(display)};
    loadSprites(sleigh, duck, foe, gift, explosion, tree);

    RendererAssets assets;
    assets.sleigh[0] = &sleigh[0];
    assets.sleigh[1] = &sleigh[1];
    assets.duck[0] = &duck[0];
    assets.duck[1] = &duck[1];
    assets.foe[0] = &foe[0];
    assets.foe[1] = &foe[1];
    assets.gift = &gift;
    assets.explosion[0] = &explosion[0];
    assets.explosion[1] = &explosion[1];
    for (int i = 0; i < TREE_COUNT; ++i)
      assets.tree[i] = &tree[i];

    GameRenderer gameRenderer(display, assets);
    Game game;
    game.seed(static_cast<uint32_t>(seed));
    game.reset(SDL_GetTicks());

    bool running = true;
    bool actionPressed = false;
    bool modePressed = false;
    uint32_t lastUpdate = SDL_GetTicks();
    int renderedFrames = 0;
    GameState previousRenderedState = STATE_MENU;

    while (running)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event) != 0)
      {
        if (event.type == SDL_QUIT)
        {
          running = false;
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
          if (event.key.keysym.sym == SDLK_ESCAPE)
            running = false;
          if (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN ||
              event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_a)
            actionPressed = true;
          if (event.key.keysym.sym == SDLK_m || event.key.keysym.sym == SDLK_TAB)
            modePressed = true;
        }
        else if (event.type == SDL_KEYUP)
        {
          if (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN ||
              event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_a)
            actionPressed = false;
          if (event.key.keysym.sym == SDLK_m || event.key.keysym.sym == SDLK_TAB)
            modePressed = false;
        }
      }

      const uint32_t now = SDL_GetTicks();
      while (now - lastUpdate >= FRAME_MS)
      {
        game.handleInput(actionPressed, modePressed, lastUpdate + FRAME_MS);
        if (game.data.state == STATE_PLAYING)
        {
          game.updatePlaying(lastUpdate + FRAME_MS);
        }
        else if (game.data.state == STATE_GAME_OVER)
        {
          game.updateHighScores();
        }
        lastUpdate += FRAME_MS;
      }

      if (game.data.state != STATE_GAME_OVER || previousRenderedState != STATE_GAME_OVER)
        clearFrame(display);
      if (game.data.state == STATE_MENU)
        gameRenderer.drawMenu(game.data, now);
      else if (game.data.state == STATE_PLAYING)
        gameRenderer.drawGameplay(game.data, now);
      else
        gameRenderer.drawGameOver(game);
      previousRenderedState = game.data.state;
      display.present(renderer);

      ++renderedFrames;
      if (maxFrames >= 0 && renderedFrames >= maxFrames)
        running = false;
      SDL_Delay(1);
    }

    if (!screenshotPath.empty() && !display.savePng(screenshotPath))
    {
      std::cerr << "Could not write screenshot: " << screenshotPath << '\n';
      result = 1;
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
