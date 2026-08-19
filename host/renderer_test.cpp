#include "game_renderer.h"
#include "sdl_tft_stub.h"

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <stdint.h>

int main()
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

  sleigh[0].load("missing-sleigh0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "sleigh");
  sleigh[1].load("missing-sleigh1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "sleigh");
  duck[0].load("missing-duck0.bin", DUCK_WIDTH, DUCK_HEIGHT, "duck");
  duck[1].load("missing-duck1.bin", DUCK_WIDTH, DUCK_HEIGHT, "duck");
  foe[0].load("missing-foe0.bin", DUCK_WIDTH, DUCK_HEIGHT, "foe");
  foe[1].load("missing-foe1.bin", DUCK_WIDTH, DUCK_HEIGHT, "foe");
  gift.load("missing-gift0.bin", GIFT_WIDTH, GIFT_HEIGHT, "gift");
  explosion[0].load("missing-explosion0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "explosion");
  explosion[1].load("missing-explosion1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT, "explosion");
  for (int i = 0; i < TREE_COUNT; ++i)
    tree[i].load("missing-tree.bin", TREE_WIDTH, TREE_HEIGHT, "tree");

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

  Game game;
  game.reset(0);
  GameRenderer renderer(display, assets);

  display.fillScreen(SKY_BLUE);
  display.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
  renderer.drawMenu(game.data, 0);
  assert(display.readPixel(0, 0) == SKY_BLUE);
  assert(display.readPixel(0, PLAYFIELD_HEIGHT) == GROUND_GREEN);

  game.handleInput(true, false, 1);
  game.handleInput(false, false, 2);
  game.updatePlaying(30);
  for (int i = 0; i < TREE_COUNT; ++i)
    game.data.trees[i].active = false;
  game.data.trees[0].active = true;
  game.data.trees[0].pos.x = 100;
  game.data.trees[0].pos.y = PLAYFIELD_HEIGHT - TREE_HEIGHT;
  game.data.trees[0].pos.oldX = game.data.trees[0].pos.x;
  game.data.trees[0].pos.oldY = game.data.trees[0].pos.y;
  for (int i = 0; i < DUCK_COUNT; ++i)
    game.data.flyingObstacles[i].active = false;
  display.fillScreen(SKY_BLUE);
  display.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
  renderer.drawGameplay(game.data, 30);
  assert(display.readPixel(110, 130) == TREE_GREEN);

  game.data.state = STATE_GAME_OVER;
  game.data.currentScore = 12;
  game.updateHighScores();
  display.fillScreen(SKY_BLUE);
  display.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
  renderer.drawGameOver(game);
  game.data.snowflakes[0].x = 20;
  game.data.snowflakes[0].y = PLAYFIELD_HEIGHT - 1;
  game.data.snowflakes[0].oldX = 20;
  game.data.snowflakes[0].oldY = PLAYFIELD_HEIGHT - 1;
  game.data.snowflakes[0].active = true;
  game.data.snowflakes[0].eraseOldPixel = true;
  display.drawPixel(20, PLAYFIELD_HEIGHT - 1, WHITE);
  renderer.drawGameOver(game);
  assert(!game.data.snowflakes[0].eraseOldPixel);
  for (int frame = 0; frame < 200; ++frame)
    renderer.drawGameOver(game);
  assert(display.readPixel(20, PLAYFIELD_HEIGHT - 1) == WHITE);
  assert(display.readPixel(SCREEN_WIDTH / 2, 30) == WHITE);

  assert(display.savePng("renderer-test.png"));
  std::ifstream screenshot("renderer-test.png", std::ios::binary);
  const uint8_t pngSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  uint8_t actualSignature[8] = {};
  screenshot.read(reinterpret_cast<char *>(actualSignature), sizeof(actualSignature));
  assert(screenshot && std::equal(actualSignature, actualSignature + 8, pngSignature));
  return 0;
}
