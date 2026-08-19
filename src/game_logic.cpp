#include "game_logic.h"

Game::Game()
    : data{}, randomState(0x6D2B79F5u), highScoreDirty(false)
{
  reset(0);
}

void Game::seed(uint32_t value)
{
  randomState = value == 0 ? 0x6D2B79F5u : value;
}

uint32_t Game::randomBelow(uint32_t exclusiveLimit)
{
  // xorshift32 keeps the host deterministic without pulling a platform RNG
  // into the firmware build.
  randomState ^= randomState << 13;
  randomState ^= randomState >> 17;
  randomState ^= randomState << 5;
  return exclusiveLimit == 0 ? 0 : randomState % exclusiveLimit;
}

int Game::randomRange(int minimum, int maximumExclusive)
{
  if (maximumExclusive <= minimum)
  {
    return minimum;
  }
  return minimum + static_cast<int>(randomBelow(static_cast<uint32_t>(maximumExclusive - minimum)));
}

void Game::reset(uint32_t now)
{
  const GameMode mode = data.gameMode;
  int sessionScores[GAME_MODE_COUNT];
  int foreverScores[GAME_MODE_COUNT];
  for (int i = 0; i < GAME_MODE_COUNT; ++i)
  {
    sessionScores[i] = data.sessionHighScore[i];
    foreverScores[i] = data.foreverHighScore[i];
  }

  data = GameData{};
  data.gameMode = mode;
  for (int i = 0; i < GAME_MODE_COUNT; ++i)
  {
    data.sessionHighScore[i] = sessionScores[i];
    data.foreverHighScore[i] = foreverScores[i];
  }

  data.state = STATE_MENU;
  data.lastStateChange = now;
  data.sleighY = 30;
  data.sleighOldY = data.sleighY;

  for (int i = 0; i < TREE_COUNT; ++i)
  {
    data.trees[i].pos.x = SCREEN_WIDTH + (i * OBSTACLE_SPAWN_DISTANCE);
    data.trees[i].pos.y = PLAYFIELD_HEIGHT - TREE_HEIGHT;
    data.trees[i].pos.oldX = data.trees[i].pos.x;
    data.trees[i].pos.oldY = data.trees[i].pos.y;
    data.trees[i].active = i < 3;
  }

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    FlyingObstacle &obstacle = data.flyingObstacles[i];
    obstacle.pos.x = SCREEN_WIDTH + (i * OBSTACLE_SPAWN_DISTANCE) + OBSTACLE_SPAWN_OFFSET;
    obstacle.pos.y = randomRange(5, 40);
    obstacle.pos.oldX = obstacle.pos.x;
    obstacle.pos.oldY = obstacle.pos.y;
    obstacle.active = i < 3;

    const int typeRoll = randomRange(0, 100);
    if (typeRoll < 80)
    {
      obstacle.type = TYPE_DUCK;
    }
    else if (typeRoll < 96)
    {
      obstacle.type = TYPE_GIFT;
    }
    else
    {
      obstacle.type = TYPE_FOE;
      obstacle.pos.y = PLAYFIELD_HEIGHT / 2 - DUCK_HEIGHT / 2;
    }
  }

  highScoreDirty = false;
}

void Game::initializeSnow()
{
  for (int i = 0; i < MAX_SNOWFLAKES; ++i)
  {
    SnowFlake &snowflake = data.snowflakes[i];
    snowflake.x = randomRange(0, SCREEN_WIDTH);
    snowflake.y = randomRange(-20, SCREEN_HEIGHT);
    snowflake.oldX = snowflake.x;
    snowflake.oldY = snowflake.y;
    snowflake.active = true;
    snowflake.eraseOldPixel = true;
  }
}

void Game::handleInput(bool actionPressed, bool modePressed, uint32_t now)
{
  bool actionEvent = false;
  bool modeEvent = false;

  if (modePressed && !data.buttonPressed)
  {
    data.buttonPressed = true;
    modeEvent = true;
  }
  if (actionPressed && !data.buttonPressed)
  {
    data.buttonPressed = true;
    actionEvent = true;
  }
  if (!actionPressed && !modePressed)
  {
    data.buttonPressed = false;
    return;
  }

  const bool action = actionEvent || modeEvent;
  switch (data.state)
  {
  case STATE_MENU:
    if (modeEvent)
    {
      data.gameMode = static_cast<GameMode>((data.gameMode + 1) % GAME_MODE_COUNT);
    }
    if (actionEvent)
    {
      data.state = STATE_PLAYING;
      data.lastStateChange = now;
    }
    break;

  case STATE_PLAYING:
    if (action && !data.sleighCrashed)
    {
      data.sleighVelocity = JUMP_STRENGTH;
    }
    else if (action && data.sleighCrashed && data.gameMode == MODE_CHEAT &&
             data.crashingStartTime + 300 < now)
    {
      data.sleighCrashed = false;
      data.sleighVelocity = JUMP_STRENGTH / 2;
    }
    break;

  case STATE_GAME_OVER:
    if (action)
    {
      reset(now);
    }
    break;
  }
}

int Game::obstacleSpeed() const
{
  if (data.gameMode == MODE_SPEED)
  {
    return 8;
  }
  if (data.gameMode == MODE_CHEAT)
  {
    return 8 + data.currentScore / 20;
  }
  return 2;
}

void Game::updatePhysics()
{
  if (data.sleighExploding)
  {
    return;
  }

  float currentGravity = GRAVITY;
  if (data.sleighCrashed)
  {
    currentGravity *= 2.0f;
  }

  data.sleighVelocity += currentGravity;
  data.sleighOldY = data.sleighY;
  data.sleighY += data.sleighVelocity;
}

void Game::updateFlyingAnimation(uint32_t now)
{
  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    FlyingObstacle &obstacle = data.flyingObstacles[i];
    if (obstacle.active && (obstacle.type == TYPE_DUCK || obstacle.type == TYPE_FOE) &&
        now - obstacle.lastFlap >= DUCK_FLAP_INTERVAL)
    {
      obstacle.flapFrame = !obstacle.flapFrame;
      obstacle.lastFlap = now;
    }

    if (obstacle.falling)
    {
      obstacle.fallVelocity += GRAVITY;
      obstacle.pos.move(0, static_cast<int>(obstacle.fallVelocity));
      if (obstacle.pos.y >= PLAYFIELD_HEIGHT)
      {
        obstacle.falling = false;
        obstacle.active = false;
        obstacle.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
  }
}

bool Game::obstacleOverlapsWithOthers(int newX, int newY, int obstacleIndex) const
{
  constexpr int X_MARGIN = 30;
  constexpr int Y_MARGIN = 20;

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    const FlyingObstacle &other = data.flyingObstacles[i];
    if (i == obstacleIndex || !other.active || other.falling)
    {
      continue;
    }

    int xDistance = newX - other.pos.x;
    int yDistance = newY - other.pos.y;
    if (xDistance < 0)
      xDistance = -xDistance;
    if (yDistance < 0)
      yDistance = -yDistance;
    if (xDistance < X_MARGIN && yDistance < Y_MARGIN)
    {
      return true;
    }
  }
  return false;
}

bool Game::treeOverlapsWithOthers(int newX, int treeIndex) const
{
  constexpr int OVERLAP_MARGIN = 20;

  for (int i = 0; i < TREE_COUNT; ++i)
  {
    const Tree &other = data.trees[i];
    if (i == treeIndex || !other.active)
    {
      continue;
    }

    int distance = newX - other.pos.x;
    if (distance < 0)
      distance = -distance;
    if (distance < OVERLAP_MARGIN)
    {
      return true;
    }
  }
  return false;
}

void Game::updateObstacles(uint32_t now)
{
  if (data.sleighExploding)
  {
    return;
  }

  const int speed = obstacleSpeed();
  for (int i = 0; i < TREE_COUNT; ++i)
  {
    Tree &tree = data.trees[i];
    if (tree.active)
    {
      tree.pos.move(-speed);
      if (tree.pos.x < -TREE_WIDTH)
      {
        tree.active = false;
        tree.scored = false;
        tree.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
    else if (now >= tree.spawnTimer)
    {
      tree.pos.x = SCREEN_WIDTH;
      tree.pos.y = PLAYFIELD_HEIGHT - TREE_HEIGHT;
      tree.pos.oldX = tree.pos.x;
      tree.pos.oldY = tree.pos.y;
      if (treeOverlapsWithOthers(tree.pos.x, i))
      {
        tree.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
      else
      {
        tree.active = true;
        tree.scored = false;
      }
    }
  }

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    FlyingObstacle &obstacle = data.flyingObstacles[i];
    if (obstacle.active && !obstacle.falling)
    {
      obstacle.pos.move(-speed);
      if (obstacle.pos.x < -DUCK_WIDTH * 2)
      {
        obstacle.active = false;
        obstacle.scored = false;
        obstacle.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
    else if (!obstacle.active && !obstacle.falling && now >= obstacle.spawnTimer)
    {
      obstacle.pos.x = SCREEN_WIDTH;
      obstacle.pos.y = randomRange(5, 40);
      obstacle.pos.oldX = obstacle.pos.x;
      obstacle.pos.oldY = obstacle.pos.y;
      obstacle.falling = false;
      obstacle.fallVelocity = 0;

      const int typeRoll = randomRange(0, 100);
      if (typeRoll < 80)
      {
        obstacle.type = TYPE_DUCK;
      }
      else if (typeRoll < 96)
      {
        obstacle.type = TYPE_GIFT;
      }
      else
      {
        obstacle.type = TYPE_FOE;
        obstacle.pos.y = PLAYFIELD_HEIGHT / 2 - DUCK_HEIGHT / 2;
      }

      if (obstacleOverlapsWithOthers(obstacle.pos.x, obstacle.pos.y, i))
      {
        obstacle.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
      else
      {
        obstacle.active = true;
        obstacle.scored = false;
      }
    }
  }
}

void Game::updateScore()
{
  if (data.sleighCrashed)
  {
    return;
  }

  for (int i = 0; i < TREE_COUNT; ++i)
  {
    Tree &tree = data.trees[i];
    if (tree.active && !tree.scored && tree.pos.x + TREE_WIDTH < SLEIGH_START_X)
    {
      tree.scored = true;
      ++data.currentScore;
    }
  }

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    FlyingObstacle &obstacle = data.flyingObstacles[i];
    if (obstacle.active && !obstacle.scored && !obstacle.falling &&
        obstacle.pos.x + DUCK_WIDTH < SLEIGH_START_X)
    {
      obstacle.scored = true;
      if (obstacle.type == TYPE_DUCK || obstacle.type == TYPE_FOE)
      {
        ++data.currentScore;
      }
    }
  }
}

void Game::checkCollisions(uint32_t now)
{
  if (data.sleighY < 2)
  {
    data.sleighY = 2;
    data.sleighVelocity = -data.sleighVelocity / 3;
  }

  if (!data.sleighCrashed && data.sleighY >= PLAYFIELD_HEIGHT - SLEIGH_HITBOX)
  {
    data.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX;
    data.sleighVelocity = -data.sleighVelocity;
    data.sleighCrashed = true;
    data.crashingStartTime = now;
    return;
  }

  if (data.sleighCrashed && !data.sleighExploding &&
      data.sleighY >= PLAYFIELD_HEIGHT - SLEIGH_HITBOX)
  {
    data.sleighExploding = true;
    data.explosionStartTime = now;
    data.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX;
    data.sleighVelocity = 0;
    return;
  }

  if (data.sleighExploding && now - data.explosionStartTime >= 1000)
  {
    data.state = STATE_GAME_OVER;
    data.lastStateChange = now;
    return;
  }

  for (int i = 0; i < TREE_COUNT; ++i)
  {
    const Tree &tree = data.trees[i];
    if (tree.active && tree.pos.x < SLEIGH_START_X + SLEIGH_HITBOX &&
        tree.pos.x + TREE_WIDTH > SLEIGH_START_X + 2 &&
        data.sleighY + SLEIGH_HITBOX > PLAYFIELD_HEIGHT - TREE_HEIGHT)
    {
      data.sleighCrashed = true;
      data.crashingStartTime = now;
      data.sleighY = PLAYFIELD_HEIGHT - TREE_HEIGHT - SLEIGH_HITBOX;
      data.sleighVelocity = -data.sleighVelocity / 2;
      return;
    }
  }

  for (int i = 0; i < DUCK_COUNT; ++i)
  {
    FlyingObstacle &obstacle = data.flyingObstacles[i];
    if (!obstacle.active || obstacle.falling ||
        obstacle.pos.x >= SLEIGH_START_X + SLEIGH_HITBOX ||
        obstacle.pos.x + DUCK_HITBOX <= SLEIGH_START_X + 2 ||
        data.sleighY >= obstacle.pos.y + DUCK_HEIGHT ||
        data.sleighY + SLEIGH_HEIGHT <= obstacle.pos.y)
    {
      continue;
    }

    if (obstacle.type == TYPE_DUCK)
    {
      data.sleighCrashed = true;
      data.crashingStartTime = now;
      if (data.sleighVelocity < 0)
      {
        data.sleighVelocity = -data.sleighVelocity;
      }
      return;
    }

    if (obstacle.type == TYPE_FOE)
    {
      if (data.sleighVelocity > 0)
      {
        obstacle.falling = true;
        obstacle.fallVelocity = 2.0f;
        data.currentScore += 20;
        data.sleighVelocity = -3.0f;
      }
      else if (!data.sleighCrashed)
      {
        data.currentScore -= 10;
        if (data.currentScore < 0)
          data.currentScore = 0;
        data.sleighCrashed = true;
        data.crashingStartTime = now;
        data.sleighVelocity = -6.0f;
        return;
      }
    }
    else if (obstacle.type == TYPE_GIFT)
    {
      data.currentScore += 10;
      obstacle.active = false;
      obstacle.spawnTimer = now + randomRange(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
    }
  }
}

void Game::updatePlaying(uint32_t now)
{
  updatePhysics();
  updateObstacles(now);
  updateFlyingAnimation(now);
  checkCollisions(now);
  updateScore();
}

void Game::updateSnow(SnowBlockedFn blocked, void *context)
{
  for (int i = 0; i < MAX_SNOWFLAKES; ++i)
  {
    SnowFlake &snowflake = data.snowflakes[i];
    if (!snowflake.active)
    {
      continue;
    }

    snowflake.oldX = snowflake.x;
    snowflake.oldY = snowflake.y;
    if (snowflake.y < SCREEN_HEIGHT - 1 &&
        (blocked == nullptr || !blocked(snowflake.x, snowflake.y + 1, context)))
    {
      ++snowflake.y;
      snowflake.eraseOldPixel = true;
    }
    else
    {
      snowflake.x = randomRange(0, SCREEN_WIDTH);
      snowflake.y = 0;
      snowflake.eraseOldPixel = false;
    }
  }
}

void Game::updateHighScores()
{
  if (data.highScoreUpdated)
  {
    return;
  }

  const int mode = static_cast<int>(data.gameMode);
  if (data.currentScore > data.sessionHighScore[mode])
  {
    data.sessionHighScore[mode] = data.currentScore;
  }
  if (data.currentScore > data.foreverHighScore[mode])
  {
    data.foreverHighScore[mode] = data.currentScore;
    highScoreDirty = true;
  }
  data.highScoreUpdated = true;
}

bool Game::consumeHighScoreDirty()
{
  const bool dirty = highScoreDirty;
  highScoreDirty = false;
  return dirty;
}
