#ifndef TTGO_NOEL_GAME_LOGIC_H
#define TTGO_NOEL_GAME_LOGIC_H

#include <stdint.h>

// The game rules are shared by the Arduino firmware and the SDL host harness.
// Rendering, input devices, persistence, and clocks stay in their platform
// adapters.
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 170;
constexpr int GROUND_HEIGHT = 10;
constexpr int PLAYFIELD_HEIGHT = SCREEN_HEIGHT - GROUND_HEIGHT;

constexpr float GRAVITY = 0.3f;
constexpr float JUMP_STRENGTH = -4.0f;
constexpr int OBSTACLE_SPAWN_DISTANCE = 80;
constexpr int OBSTACLE_SPAWN_OFFSET = 40;

constexpr int SLEIGH_WIDTH = 20;
constexpr int SLEIGH_HEIGHT = 14;
constexpr int SLEIGH_HITBOX = 8;
constexpr int SLEIGH_START_X = 40;

constexpr int TREE_WIDTH = 20;
constexpr int TREE_HEIGHT = SCREEN_HEIGHT / 4;
constexpr int TREE_COUNT = 5;

constexpr int DUCK_WIDTH = 20;
constexpr int DUCK_HEIGHT = 14;
constexpr int DUCK_HITBOX = 10;
constexpr int DUCK_COUNT = 5;
constexpr uint32_t DUCK_FLAP_INTERVAL = 500;

constexpr int GIFT_WIDTH = 13;
constexpr int GIFT_HEIGHT = 14;

constexpr uint32_t SPAWN_DELAY_MIN = 800;
constexpr uint32_t SPAWN_DELAY_MAX = 2500;
constexpr int MAX_SNOWFLAKES = 50;
constexpr int GAME_MODE_COUNT = 3;

enum GameState
{
  STATE_MENU,
  STATE_PLAYING,
  STATE_GAME_OVER
};

enum GameMode
{
  MODE_NORMAL,
  MODE_SPEED,
  MODE_CHEAT
};

enum ObstacleType
{
  TYPE_DUCK,
  TYPE_FOE,
  TYPE_GIFT
};

struct Position
{
  int x = 0;
  int y = 0;
  int oldX = 0;
  int oldY = 0;

  void updateOld()
  {
    oldX = x;
    oldY = y;
  }

  void move(int dx, int dy = 0)
  {
    updateOld();
    x += dx;
    y += dy;
  }
};

struct Tree
{
  Position pos;
  bool active = false;
  uint32_t spawnTimer = 0;
  bool scored = false;
};

struct FlyingObstacle
{
  Position pos;
  bool active = false;
  uint32_t spawnTimer = 0;
  bool scored = false;
  ObstacleType type = TYPE_DUCK;
  uint32_t lastFlap = 0;
  bool flapFrame = false;
  bool falling = false;
  float fallVelocity = 0.0f;
};

struct SnowFlake
{
  int x = 0;
  int y = 0;
  int oldX = 0;
  int oldY = 0;
  bool active = false;
  bool eraseOldPixel = true;
};

struct GameData
{
  GameState state = STATE_MENU;
  uint32_t lastStateChange = 0;

  float sleighY = 30.0f;
  float sleighVelocity = 0.0f;
  float sleighOldY = 30.0f;
  bool sleighCrashed = false;
  uint32_t crashingStartTime = 0;
  bool sleighExploding = false;
  uint32_t explosionStartTime = 0;

  GameMode gameMode = MODE_NORMAL;

  int currentScore = 0;
  int sessionHighScore[GAME_MODE_COUNT] = {};
  int foreverHighScore[GAME_MODE_COUNT] = {};

  bool buttonPressed = false;
  bool gameOverScreenDrawn = false;
  bool highScoreUpdated = false;

  Tree trees[TREE_COUNT];
  FlyingObstacle flyingObstacles[DUCK_COUNT];
  SnowFlake snowflakes[MAX_SNOWFLAKES];
};

using SnowBlockedFn = bool (*)(int x, int y, void *context);

class Game
{
public:
  Game();

  GameData data;

  void seed(uint32_t value);
  void reset(uint32_t now);
  void initializeSnow();
  void handleInput(bool actionPressed, bool modePressed, uint32_t now);
  void updatePlaying(uint32_t now);
  void updateSnow(SnowBlockedFn blocked = nullptr, void *context = nullptr);
  void updateHighScores();
  bool consumeHighScoreDirty();

private:
  uint32_t randomState;
  bool highScoreDirty;

  uint32_t randomBelow(uint32_t exclusiveLimit);
  int randomRange(int minimum, int maximumExclusive);
  int obstacleSpeed() const;
  bool obstacleOverlapsWithOthers(int newX, int newY, int obstacleIndex) const;
  bool treeOverlapsWithOthers(int newX, int treeIndex) const;
  void updatePhysics();
  void updateFlyingAnimation(uint32_t now);
  void updateObstacles(uint32_t now);
  void updateScore();
  void checkCollisions(uint32_t now);
};

#endif
