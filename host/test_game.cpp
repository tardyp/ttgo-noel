#include "game_logic.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void disableObstacles(Game &game)
{
  for (int i = 0; i < TREE_COUNT; ++i)
  {
    game.data.trees[i].active = false;
    game.data.trees[i].spawnTimer = UINT32_MAX;
  }
  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    game.data.flyingObstacles[i].active = false;
    game.data.flyingObstacles[i].falling = false;
    game.data.flyingObstacles[i].spawnTimer = UINT32_MAX;
  }
}

int main()
{
  Game game;
  game.seed(7);
  game.reset(0);

  assert(game.data.state == STATE_MENU);
  assert(game.data.gameMode == MODE_NORMAL);

  game.handleInput(false, true, 10);
  assert(game.data.gameMode == MODE_SPEED);
  game.handleInput(false, false, 11);
  game.handleInput(true, false, 20);
  assert(game.data.state == STATE_PLAYING);
  game.handleInput(false, false, 21);
  disableObstacles(game);

  game.data.sleighY = 30;
  game.data.sleighVelocity = 0;
  game.handleInput(true, false, 30);
  game.handleInput(false, false, 31);
  game.updatePlaying(60);
  assert(game.data.sleighVelocity < 0);
  assert(game.data.sleighY < 30);

  game.data.sleighY = 30;
  game.data.sleighVelocity = 0;
  FlyingObstacle &gift = game.data.flyingObstacles[0];
  gift.active = true;
  gift.falling = false;
  gift.type = TYPE_GIFT;
  gift.pos.x = SLEIGH_START_X + 10;
  gift.pos.y = 30;
  game.updatePlaying(100);
  assert(game.data.currentScore == 10);
  assert(!gift.active);

  game.data.sleighY = 30;
  game.data.sleighVelocity = 1;
  FlyingObstacle &foe = game.data.flyingObstacles[1];
  foe.active = true;
  foe.falling = false;
  foe.type = TYPE_FOE;
  foe.pos.x = SLEIGH_START_X + 10;
  foe.pos.y = 30;
  game.updatePlaying(200);
  assert(foe.falling);
  assert(game.data.currentScore == 30);

  game.data.sleighY = 30;
  game.data.sleighVelocity = -1;
  FlyingObstacle &duck = game.data.flyingObstacles[2];
  duck.active = true;
  duck.falling = false;
  duck.type = TYPE_DUCK;
  duck.pos.x = SLEIGH_START_X + 10;
  duck.pos.y = 30;
  game.updatePlaying(300);
  assert(game.data.sleighCrashed);

  game.data.state = STATE_PLAYING;
  game.data.sleighCrashed = true;
  game.data.sleighExploding = true;
  game.data.explosionStartTime = 500;
  game.data.currentScore = 31;
  game.updatePlaying(1500);
  assert(game.data.state == STATE_GAME_OVER);
  game.updateHighScores();
  assert(game.data.sessionHighScore[MODE_SPEED] == 31);
  assert(game.data.foreverHighScore[MODE_SPEED] == 31);
  assert(game.consumeHighScoreDirty());
  assert(!game.consumeHighScoreDirty());

  game.handleInput(false, false, 1600);
  game.handleInput(true, false, 1700);
  assert(game.data.state == STATE_MENU);
  assert(game.data.currentScore == 0);
  assert(game.data.gameMode == MODE_SPEED);

  puts("core smoke tests passed");
  return 0;
}
