/*
  Christmas Flappy Bird Game for the LilyGo T-Display-S3.

  Game rules live in game_logic.cpp so the same update and collision code can
  run in the SDL host harness. This file contains only Arduino/TFT_eSPI,
  SPIFFS, Preferences, and GPIO integration.
*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

#include "game_logic.h"
#include "game_renderer.h"
#include "tft_espi_adapter.h"

#ifndef ST7789_DRIVER
#error "This code requires the T-Display S3 ST7789 TFT_eSPI setup"
#endif

#define DISPLAY_POWER_PIN 15
#define DISPLAY_BACKLIGHT_PIN 38
#define BUTTON_PIN 14
#define BUTTON2_PIN 0
#define BUTTON3_PIN 1


TFT_eSPI tft = TFT_eSPI();
Preferences preferences;
Game game;
GameData &gameData = game.data;

TFT_eSprite sleighSprite = TFT_eSprite(&tft);
TFT_eSprite sleighSprite2 = TFT_eSprite(&tft);
TFT_eSprite duckSprite = TFT_eSprite(&tft);
TFT_eSprite duckSprite2 = TFT_eSprite(&tft);
TFT_eSprite foeSprite = TFT_eSprite(&tft);
TFT_eSprite foeSprite2 = TFT_eSprite(&tft);
TFT_eSprite giftSprite = TFT_eSprite(&tft);
TFT_eSprite explosionSprite = TFT_eSprite(&tft);
TFT_eSprite explosionSprite2 = TFT_eSprite(&tft);
TftEspiDisplay tftDisplay(tft);
TftEspiSprite sleighRenderSprite(sleighSprite);
TftEspiSprite sleighRenderSprite2(sleighSprite2);
TftEspiSprite duckRenderSprite(duckSprite);
TftEspiSprite duckRenderSprite2(duckSprite2);
TftEspiSprite foeRenderSprite(foeSprite);
TftEspiSprite foeRenderSprite2(foeSprite2);
TftEspiSprite giftRenderSprite(giftSprite);
TftEspiSprite explosionRenderSprite(explosionSprite);
TftEspiSprite explosionRenderSprite2(explosionSprite2);
TFT_eSprite *treeSprites[TREE_COUNT] = {};
TftEspiSprite *treeRenderSprites[TREE_COUNT] = {};
RendererAssets rendererAssets;
GameRenderer gameRenderer(tftDisplay, rendererAssets);

// ============================================================================
// SPRITE CREATION
// ============================================================================

void createDefaultSleigh()
{
  sleighSprite.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  sleighSprite.fillSprite(SKY_BLUE);
  sleighSprite.fillRect(2, 2, SLEIGH_WIDTH - 4, SLEIGH_HEIGHT - 4, SLEIGH_RED);
  sleighSprite.drawLine(0, SLEIGH_HEIGHT - 1, SLEIGH_WIDTH, SLEIGH_HEIGHT - 1, SLEIGH_RED);
  sleighSprite.fillRect(4, 0, 6, 4, TFT_GREEN);
}

void createDefaultSleigh2()
{
  sleighSprite2.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  sleighSprite2.fillSprite(SKY_BLUE);
  sleighSprite2.fillRect(2, 2, SLEIGH_WIDTH - 4, SLEIGH_HEIGHT - 4, SLEIGH_RED);
  sleighSprite2.drawLine(0, SLEIGH_HEIGHT - 1, SLEIGH_WIDTH, SLEIGH_HEIGHT - 1, SLEIGH_RED);
  sleighSprite2.fillRect(4, 0, 6, 4, TFT_GREEN);
}

void createDefaultDuck()
{
  duckSprite.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
  duckSprite.fillSprite(SKY_BLUE);
  duckSprite.fillCircle(6, 7, 5, DUCK_YELLOW);
  duckSprite.fillCircle(12, 5, 4, DUCK_YELLOW);
  duckSprite.fillTriangle(15, 5, 19, 4, 19, 6, TFT_ORANGE);
  duckSprite.fillCircle(13, 4, 1, TFT_BLACK);
}

void createDefaultDuck2()
{
  duckSprite2.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
  duckSprite2.fillSprite(SKY_BLUE);
  duckSprite2.fillCircle(6, 8, 5, DUCK_YELLOW);
  duckSprite2.fillCircle(12, 4, 4, DUCK_YELLOW);
  duckSprite2.fillTriangle(15, 4, 19, 3, 19, 5, TFT_ORANGE);
  duckSprite2.fillCircle(13, 3, 1, TFT_BLACK);
}

void createDefaultFoe()
{
  foeSprite.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
  foeSprite.fillSprite(SKY_BLUE);
  foeSprite.fillCircle(10, 7, 6, TFT_BLACK);
  foeSprite.fillCircle(8, 5, 2, TFT_RED);
  foeSprite.fillRect(6, 10, 8, 3, TFT_BLACK);
}

void createDefaultFoe2()
{
  foeSprite2.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
  foeSprite2.fillSprite(SKY_BLUE);
  foeSprite2.fillCircle(10, 7, 6, TFT_BLACK);
  foeSprite2.fillCircle(8, 5, 2, TFT_RED);
  foeSprite2.fillRect(5, 9, 10, 3, TFT_BLACK);
}

void createDefaultGift()
{
  giftSprite.createSprite(GIFT_WIDTH, GIFT_HEIGHT);
  giftSprite.fillSprite(SKY_BLUE);
  giftSprite.fillRect(5, 4, 10, 8, TFT_RED);
  giftSprite.fillRect(9, 3, 2, 10, TFT_YELLOW);
  giftSprite.fillRect(4, 7, 12, 2, TFT_YELLOW);
  giftSprite.fillCircle(10, 5, 2, TFT_YELLOW);
}

void createDefaultExplosion()
{
  explosionSprite.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  explosionSprite.fillSprite(SKY_BLUE);
  explosionSprite.fillCircle(10, 7, 8, TFT_RED);
  explosionSprite.fillCircle(10, 7, 5, TFT_ORANGE);
  explosionSprite.fillCircle(10, 7, 2, TFT_YELLOW);
  explosionSprite.fillTriangle(10, 0, 8, 4, 12, 4, TFT_ORANGE);
  explosionSprite.fillTriangle(18, 7, 14, 6, 14, 8, TFT_ORANGE);
  explosionSprite.fillTriangle(2, 7, 6, 6, 6, 8, TFT_ORANGE);
  explosionSprite.fillTriangle(10, 14, 8, 10, 12, 10, TFT_ORANGE);
}

void createDefaultExplosion2()
{
  explosionSprite2.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  explosionSprite2.fillSprite(SKY_BLUE);
  explosionSprite2.fillCircle(10, 7, 7, TFT_ORANGE);
  explosionSprite2.fillCircle(10, 7, 4, TFT_YELLOW);
  explosionSprite2.fillCircle(10, 7, 1, TFT_WHITE);
  explosionSprite2.fillTriangle(10, 1, 7, 5, 13, 5, TFT_RED);
  explosionSprite2.fillTriangle(17, 7, 13, 5, 13, 9, TFT_RED);
  explosionSprite2.fillTriangle(3, 7, 7, 5, 7, 9, TFT_RED);
  explosionSprite2.fillTriangle(10, 13, 7, 9, 13, 9, TFT_RED);
}

bool loadSpriteFromSPIFFS(TFT_eSprite &sprite, const char *path, int width, int height)
{
  if (!SPIFFS.exists(path))
  {
    return false;
  }

  sprite.createSprite(width, height);
  fs::File file = SPIFFS.open(path, "r");
  if (!file)
  {
    return false;
  }

  uint16_t buffer[TREE_WIDTH * TREE_HEIGHT];
  const size_t expectedBytes = static_cast<size_t>(width) * height * sizeof(uint16_t);
  const size_t bytesRead = file.read(reinterpret_cast<uint8_t *>(buffer), expectedBytes);
  file.close();
  if (bytesRead != expectedBytes)
  {
    return false;
  }
  sprite.pushImage(0, 0, width, height, buffer);
  return true;
}

void createTreeSprite(int treeIndex)
{
  if (treeRenderSprites[treeIndex] != nullptr)
  {
    delete treeRenderSprites[treeIndex];
    treeRenderSprites[treeIndex] = nullptr;
  }
  if (treeSprites[treeIndex] != nullptr)
  {
    treeSprites[treeIndex]->deleteSprite();
    delete treeSprites[treeIndex];
  }
  treeSprites[treeIndex] = new TFT_eSprite(&tft);

  if (!loadSpriteFromSPIFFS(*treeSprites[treeIndex], "/tree.bin", TREE_WIDTH, TREE_HEIGHT))
  {
    treeSprites[treeIndex]->createSprite(TREE_WIDTH, TREE_HEIGHT);
    treeSprites[treeIndex]->fillSprite(SKY_BLUE);
    const int trunkWidth = 6;
    const int trunkHeight = TREE_HEIGHT / 4;
    treeSprites[treeIndex]->fillRect(
        TREE_WIDTH / 2 - trunkWidth / 2, TREE_HEIGHT - trunkHeight,
        trunkWidth, trunkHeight, TREE_BROWN);
    for (int i = 0; i < 3; ++i)
    {
      const int layerHeight = (TREE_HEIGHT - trunkHeight) / 3;
      const int layerWidth = TREE_WIDTH - i * 4;
      const int layerY = trunkHeight + i * layerHeight;
      treeSprites[treeIndex]->fillTriangle(
          TREE_WIDTH / 2, layerY,
          TREE_WIDTH / 2 - layerWidth / 2, layerY + layerHeight,
          TREE_WIDTH / 2 + layerWidth / 2, layerY + layerHeight,
          TREE_GREEN);
    }
  }
  treeRenderSprites[treeIndex] = new TftEspiSprite(*treeSprites[treeIndex]);
}

void loadSpritesFromSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
  }

  if (!loadSpriteFromSPIFFS(sleighSprite, "/sleigh0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT))
    createDefaultSleigh();
  if (!loadSpriteFromSPIFFS(sleighSprite2, "/sleigh1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT))
    createDefaultSleigh2();
  if (!loadSpriteFromSPIFFS(duckSprite, "/duck0.bin", DUCK_WIDTH, DUCK_HEIGHT))
    createDefaultDuck();
  if (!loadSpriteFromSPIFFS(duckSprite2, "/duck1.bin", DUCK_WIDTH, DUCK_HEIGHT))
    createDefaultDuck2();
  if (!loadSpriteFromSPIFFS(foeSprite, "/foe0.bin", DUCK_WIDTH, DUCK_HEIGHT))
    createDefaultFoe();
  if (!loadSpriteFromSPIFFS(foeSprite2, "/foe1.bin", DUCK_WIDTH, DUCK_HEIGHT))
    createDefaultFoe2();
  if (!loadSpriteFromSPIFFS(giftSprite, "/gift0.bin", GIFT_WIDTH, GIFT_HEIGHT))
    createDefaultGift();
  if (!loadSpriteFromSPIFFS(explosionSprite, "/explosion0.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT))
    createDefaultExplosion();
  if (!loadSpriteFromSPIFFS(explosionSprite2, "/explosion1.bin", SLEIGH_WIDTH, SLEIGH_HEIGHT))
    createDefaultExplosion2();

  for (int i = 0; i < TREE_COUNT; ++i)
  {
    createTreeSprite(i);
  }
}

void bindRendererAssets()
{
  rendererAssets.sleigh[0] = &sleighRenderSprite;
  rendererAssets.sleigh[1] = &sleighRenderSprite2;
  rendererAssets.duck[0] = &duckRenderSprite;
  rendererAssets.duck[1] = &duckRenderSprite2;
  rendererAssets.foe[0] = &foeRenderSprite;
  rendererAssets.foe[1] = &foeRenderSprite2;
  rendererAssets.gift = &giftRenderSprite;
  rendererAssets.explosion[0] = &explosionRenderSprite;
  rendererAssets.explosion[1] = &explosionRenderSprite2;
  for (int i = 0; i < TREE_COUNT; ++i)
    rendererAssets.tree[i] = treeRenderSprites[i];
}

// ============================================================================
// DISPLAY AND INPUT ADAPTERS
void clearScreen()
{
  tftDisplay.fillScreen(SKY_BLUE);
  tftDisplay.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
}

void handleInput()
{
  const bool actionPressed = !digitalRead(BUTTON_PIN) || !digitalRead(BUTTON3_PIN);
  const bool modePressed = !digitalRead(BUTTON2_PIN);
  const GameState previousState = gameData.state;
  game.handleInput(actionPressed, modePressed, millis());
  if (previousState != gameData.state)
  {
    clearScreen();
  }
}

void persistHighScoreIfNeeded()
{
  if (!game.consumeHighScoreDirty())
  {
    return;
  }

  const int mode = static_cast<int>(gameData.gameMode);
  char key[16];
  sprintf(key, "highscore%d", mode);
  preferences.putInt(key, gameData.foreverHighScore[mode]);
}


// ============================================================================
// Arduino lifecycle
// ============================================================================

void setup()
{
  Serial.begin(115200);
  pinMode(DISPLAY_POWER_PIN, OUTPUT);
  digitalWrite(DISPLAY_POWER_PIN, HIGH);
  pinMode(DISPLAY_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT_PIN, TFT_BACKLIGHT_ON);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  preferences.begin("flappysleigh", false);
  for (int mode = MODE_NORMAL; mode <= MODE_CHEAT; ++mode)
  {
    char key[16];
    sprintf(key, "highscore%d", mode);
    gameData.foreverHighScore[mode] = preferences.getInt(key, 0);
    gameData.sessionHighScore[mode] = 0;
  }

  tft.init();
  tft.setRotation(1);
  tftDisplay.fillScreen(SKY_BLUE);
  loadSpritesFromSPIFFS();
  bindRendererAssets();
  game.reset(millis());
  clearScreen();
}

void loop()
{
  handleInput();
  const uint32_t now = millis();
  switch (gameData.state)
  {
  case STATE_MENU:
    gameRenderer.drawMenu(gameData, now);
    delay(30);
    break;

  case STATE_PLAYING:
    game.updatePlaying(now);
    gameRenderer.drawGameplay(gameData, now);
    delay(30);
    break;

  case STATE_GAME_OVER:
    game.updateHighScores();
    persistHighScoreIfNeeded();
    gameRenderer.drawGameOver(game);
    delay(30);
    break;
  }
}
