#include "game_renderer.h"

#include <math.h>
#include <stdio.h>

GameRenderer::GameRenderer(TftDisplay &display, const RendererAssets &assets)
    : display(display), assets(assets)
{
}

void GameRenderer::pushSprite(TftSprite *sprite, int x, int y)
{
  if (sprite != nullptr)
  {
    sprite->pushSprite(x, y);
  }
}

void GameRenderer::drawMenu(const GameData &data, uint32_t now)
{
  display.setTextColor(WHITE, SKY_BLUE, true);
  display.setTextSize(2);
  display.drawCentreString("Appuyez!", SCREEN_WIDTH / 2, 30, 2);
  display.setTextSize(1);
  display.drawCentreString("L'aventure du Pere Noel!", SCREEN_WIDTH / 2, 70, 1);
  if (data.gameMode == MODE_SPEED)
    display.drawCentreString("Mode Rapide", SCREEN_WIDTH / 2, 100, 1);
  else if (data.gameMode == MODE_CHEAT)
    display.drawCentreString("Mode  Cheat", SCREEN_WIDTH / 2, 100, 1);
  else
    display.drawCentreString("Mode Normal", SCREEN_WIDTH / 2, 100, 1);

  const int speed = data.gameMode == MODE_NORMAL ? 600 : 300;
  const float speed2 = data.gameMode == MODE_NORMAL ? 500.0f : 250.0f;
  if (now / speed % 2 == 0)
    pushSprite(assets.duck[0], SCREEN_WIDTH - 40, 30);
  else
    pushSprite(assets.duck[1], SCREEN_WIDTH - 40, 30);

  display.fillRect(10, 20, SLEIGH_WIDTH, SLEIGH_HEIGHT + 20, SKY_BLUE);
  if (cos(now / speed2) > 0)
    pushSprite(assets.sleigh[1], 10, 30 + sin(now / speed2) * 10);
  else
    pushSprite(assets.sleigh[0], 10, 30 + sin(now / speed2) * 10);

  display.fillRect(SCREEN_WIDTH - 30, 90, SLEIGH_WIDTH, SLEIGH_HEIGHT + 20, SKY_BLUE);
  if (data.gameMode == MODE_CHEAT)
  {
    if (cos(now / 200.0f) > 0)
      pushSprite(assets.foe[1], SCREEN_WIDTH - 30, 100 + sin(now / 200.0f) * 10);
    else
      pushSprite(assets.foe[0], SCREEN_WIDTH - 30, 100 + sin(now / 200.0f) * 10);
  }
  display.drawString("https://github.com/tardyp/ttgo-noel", 10, SCREEN_HEIGHT - 16, 1);
}

void GameRenderer::drawGameplay(GameData &data, uint32_t now)
{
  for (int i = 0; i < TREE_COUNT; ++i)
  {
    const Tree &tree = data.trees[i];
    display.fillRect(tree.pos.oldX, tree.pos.oldY, TREE_WIDTH, TREE_HEIGHT, SKY_BLUE);
    if (tree.active)
      pushSprite(assets.tree[i], tree.pos.x, tree.pos.y);
  }

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    const FlyingObstacle &obstacle = data.flyingObstacles[i];
    display.fillRect(obstacle.pos.oldX, obstacle.pos.oldY, DUCK_WIDTH, DUCK_HEIGHT, SKY_BLUE);
    if (!obstacle.active && !obstacle.falling)
      continue;

    if (obstacle.type == TYPE_DUCK)
      pushSprite(assets.duck[obstacle.flapFrame ? 1 : 0], obstacle.pos.x, obstacle.pos.y);
    else if (obstacle.type == TYPE_FOE)
      pushSprite(assets.foe[obstacle.flapFrame ? 1 : 0], obstacle.pos.x, obstacle.pos.y);
    else
      pushSprite(assets.gift, obstacle.pos.x, obstacle.pos.y);
  }

  display.fillRect(SLEIGH_START_X, static_cast<int>(data.sleighOldY) - 2,
                   SLEIGH_WIDTH + 2, SLEIGH_HEIGHT + 4, SKY_BLUE);
  if (data.sleighExploding)
  {
    data.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX * 2;
    if ((now / 300) % 2 == 0)
      pushSprite(assets.explosion[0], SLEIGH_START_X, static_cast<int>(data.sleighY));
    else
      pushSprite(assets.explosion[1], SLEIGH_START_X, static_cast<int>(data.sleighY));
  }
  else if (!(data.sleighCrashed && now / 100 % 2 == 0))
  {
    if (data.sleighVelocity < 0)
      pushSprite(assets.sleigh[0], SLEIGH_START_X, static_cast<int>(data.sleighY));
    else
      pushSprite(assets.sleigh[1], SLEIGH_START_X, static_cast<int>(data.sleighY));
  }

  char score[32];
  snprintf(score, sizeof(score), "Score: %d", data.currentScore);
  display.fillRect(5, PLAYFIELD_HEIGHT, 100, 16, GROUND_GREEN);
  display.setTextColor(WHITE, GROUND_GREEN, true);
  display.setTextSize(1);
  display.drawString(score, 5, PLAYFIELD_HEIGHT + 2, 1);
}

bool GameRenderer::snowBlocked(int x, int y, void *context)
{
  const TftDisplay *display = static_cast<const TftDisplay *>(context);
  if (display == nullptr || x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
    return false;
  return display->readPixel(x, y) != SKY_BLUE;
}

void GameRenderer::drawSnowPixel(int x, int y, TftColor color)
{
  if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    display.drawPixel(x, y, color);
}

void GameRenderer::drawGameOver(Game &game)
{
  const GameData &data = game.data;
  const int boxX = (SCREEN_WIDTH - 240) / 2;
  const int boxY = 30;

  if (!game.data.gameOverScreenDrawn)
  {
    game.initializeSnow();
    display.fillRect(boxX, boxY, 240, 100, 0x0000);
    display.drawRect(boxX, boxY, 240, 100, WHITE);
    display.setTextColor(0xF800, 0x0000, true);
    display.setTextSize(1);
    display.drawCentreString("Perdu!!", SCREEN_WIDTH / 2, boxY + 8, 2);
    display.setTextColor(WHITE, 0x0000, true);

    char text[48];
    snprintf(text, sizeof(text), "Score: %d", data.currentScore);
    display.drawCentreString(text, SCREEN_WIDTH / 2, boxY + 30, 1);
    snprintf(text, sizeof(text), "Meilleur: %d", data.sessionHighScore[data.gameMode]);
    display.drawCentreString(text, SCREEN_WIDTH / 2, boxY + 45, 1);
    snprintf(text, sizeof(text), "Record: %d", data.foreverHighScore[data.gameMode]);
    display.drawCentreString(text, SCREEN_WIDTH / 2, boxY + 60, 1);
    display.drawCentreString("Appuyez pour recommencer", SCREEN_WIDTH / 2, boxY + 75, 1);
    game.data.gameOverScreenDrawn = true;
  }

  game.updateSnow(snowBlocked, &display);
  for (int i = 0; i < MAX_SNOWFLAKES; ++i)
  {
    SnowFlake &snowflake = game.data.snowflakes[i];
    if (snowflake.eraseOldPixel)
    {
      const bool inBox = snowflake.oldX >= boxX && snowflake.oldX < boxX + 240 &&
                         snowflake.oldY >= boxY && snowflake.oldY < boxY + 100;
      const TftColor background = inBox ? 0x0000 :
                                  (snowflake.oldY >= PLAYFIELD_HEIGHT ? GROUND_GREEN : SKY_BLUE);
      drawSnowPixel(snowflake.oldX, snowflake.oldY, background);
    }
    drawSnowPixel(snowflake.x, snowflake.y, WHITE);
  }
}
