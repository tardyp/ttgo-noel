/*
 Christmas Flappy Bird Game - Refactored Architecture
 - Sleigh sprite from image
 - Trees and Ducks from images
 - Button on GPIO26 to flap
 - Screen: 240x135
 - Clean separation of concerns with state machine
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

// ============================================================================
// CONSTANTS & CONFIGURATION
// ============================================================================

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define GROUND_HEIGHT 10
#define PLAYFIELD_HEIGHT (SCREEN_HEIGHT - GROUND_HEIGHT)

// Game constants
#define BUTTON_PIN 12
#define BUTTON2_PIN 26 // Optional second button for changing game mode
#define GRAVITY 0.3
#define JUMP_STRENGTH -4.0
#define OBSTACLE_SPEED (gameData.gameMode == MODE_SPEED ? 8 : gameData.gameMode == MODE_CHEAT ? (8 + gameData.currentScore / 20) \
                                                                                              : 2)
#define OBSTACLE_SPAWN_DISTANCE 80
#define OBSTACLE_SPAWN_OFFSET 40

// Sleigh configuration
#define SLEIGH_WIDTH 20
#define SLEIGH_HEIGHT 14
#define SLEIGH_HITBOX 8
#define SLEIGH_START_X 40

// Tree configuration
#define TREE_WIDTH 20
#define TREE_HEIGHT (SCREEN_HEIGHT / 4) // 34 pixels
#define TREE_COUNT 5

// Duck configuration
#define DUCK_WIDTH 20
#define DUCK_HEIGHT 14
#define DUCK_HITBOX 10
#define DUCK_COUNT 5
#define DUCK_FLAP_INTERVAL 500 // milliseconds

// Gift configuration
#define GIFT_WIDTH 13
#define GIFT_HEIGHT 14

// Obstacle spawning configuration
#define SPAWN_DELAY_MIN 800  // milliseconds
#define SPAWN_DELAY_MAX 2500 // milliseconds

// Snow effect
#define MAX_SNOWFLAKES 50

// Colors
#define SKY_BLUE 0x3A9F
#define GROUND_GREEN 0x2589
#define TREE_GREEN 0x2444
#define TREE_BROWN 0x7140
#define SLEIGH_RED 0xF800
#define DUCK_YELLOW 0xFFE0
#define WHITE 0xFFFF

// ============================================================================
// ENUMS & STRUCTURES
// ============================================================================

enum GameState
{
  STATE_MENU,     // Waiting for player to start
  STATE_PLAYING,  // Active gameplay
  STATE_GAME_OVER // Game ended, showing results
};

enum GameMode
{
  MODE_NORMAL,
  MODE_SPEED,
  MODE_CHEAT,
};
enum ObstacleType
{
  TYPE_DUCK, // Collision = game over
  TYPE_FOE,  // Hit from above = 20 points, hit while flapping = -10 points + game over
  TYPE_GIFT  // Hit = 10 points, disappears
};

// Position structure for 2D objects
struct Position
{
  int x;
  int y;
  int oldX;
  int oldY;

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
  bool active;         // Whether this obstacle is currently in play
  uint32_t spawnTimer; // Timer for spawn delay
  bool scored;
  TFT_eSprite *sprite;
};

struct FlyingObstacle
{
  Position pos;
  bool active;         // Whether this obstacle is currently in play
  uint32_t spawnTimer; // Timer for spawn delay
  bool scored;
  ObstacleType type; // Type of obstacle

  // Animation state
  uint32_t lastFlap; // Last time this obstacle flapped (for ducks/foes)
  bool flapFrame;    // Current frame (0 or 1)

  // Falling state (for killed foes)
  bool falling;       // Whether foe is falling after being killed
  float fallVelocity; // Falling speed
};

struct SnowFlake
{
  int x;
  int y;
  bool active;
};

// Unified game state structure
struct GameData
{
  // Game state
  GameState state;
  uint32_t lastStateChange;

  // Player physics
  float sleighY;
  float sleighVelocity;
  float sleighOldY;
  bool sleighCrashed;          // Whether sleigh has crashed and is falling to ground
  uint32_t crashingStartTime;  // When crashing animation started
  bool sleighExploding;        // Whether sleigh is exploding (1 second animation)
  uint32_t explosionStartTime; // When explosion animation started

  GameMode gameMode; // Current game mode

  // Score & high scores
  int currentScore;
  int sessionHighScore[MODE_CHEAT + 1];
  int foreverHighScore[MODE_CHEAT + 1];

  // Input
  bool buttonPressed;

  // Animation & rendering
  uint32_t lastDuckFlap;
  bool duckFrame;
  bool gameOverScreenDrawn;
  bool highScoreUpdated;

  // Obstacles
  Tree trees[TREE_COUNT];
  FlyingObstacle flyingObstacles[DUCK_COUNT];
  SnowFlake snowflakes[MAX_SNOWFLAKES];
};

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// Sprites
TFT_eSprite sleighSprite = TFT_eSprite(&tft);
TFT_eSprite sleighSprite2 = TFT_eSprite(&tft);
TFT_eSprite duckSprite = TFT_eSprite(&tft);
TFT_eSprite duckSprite2 = TFT_eSprite(&tft);
TFT_eSprite foeSprite = TFT_eSprite(&tft);
TFT_eSprite foeSprite2 = TFT_eSprite(&tft);
TFT_eSprite giftSprite = TFT_eSprite(&tft);
TFT_eSprite explosionSprite = TFT_eSprite(&tft);
TFT_eSprite explosionSprite2 = TFT_eSprite(&tft);
TFT_eSprite scoreSprite = TFT_eSprite(&tft);

// Game state
GameData gameData;

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
  // Black Peter - dark figure
  foeSprite.fillCircle(10, 7, 6, TFT_BLACK);
  foeSprite.fillCircle(8, 5, 2, TFT_RED);
  foeSprite.fillRect(6, 10, 8, 3, TFT_BLACK);
}

void createDefaultFoe2()
{
  foeSprite2.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
  foeSprite2.fillSprite(SKY_BLUE);
  // Black Peter - dark figure (flapping)
  foeSprite2.fillCircle(10, 7, 6, TFT_BLACK);
  foeSprite2.fillCircle(8, 5, 2, TFT_RED);
  foeSprite2.fillRect(5, 9, 10, 3, TFT_BLACK);
}

void createDefaultGift()
{
  giftSprite.createSprite(GIFT_WIDTH, GIFT_HEIGHT);
  giftSprite.fillSprite(SKY_BLUE);
  // Gift box - colorful present
  giftSprite.fillRect(5, 4, 10, 8, TFT_RED);
  giftSprite.fillRect(9, 3, 2, 10, TFT_YELLOW);
  giftSprite.fillRect(4, 7, 12, 2, TFT_YELLOW);
  giftSprite.fillCircle(10, 5, 2, TFT_YELLOW);
}

void createDefaultExplosion()
{
  explosionSprite.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  explosionSprite.fillSprite(SKY_BLUE);
  // Explosion effect - jagged red/orange/yellow
  explosionSprite.fillCircle(10, 7, 8, TFT_RED);
  explosionSprite.fillCircle(10, 7, 5, TFT_ORANGE);
  explosionSprite.fillCircle(10, 7, 2, TFT_YELLOW);
  // Add some spiky points
  explosionSprite.fillTriangle(10, 0, 8, 4, 12, 4, TFT_ORANGE);
  explosionSprite.fillTriangle(18, 7, 14, 6, 14, 8, TFT_ORANGE);
  explosionSprite.fillTriangle(2, 7, 6, 6, 6, 8, TFT_ORANGE);
  explosionSprite.fillTriangle(10, 14, 8, 10, 12, 10, TFT_ORANGE);
}

void createDefaultExplosion2()
{
  explosionSprite2.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
  explosionSprite2.fillSprite(SKY_BLUE);
  // Explosion effect - larger burst with different spike positions
  explosionSprite2.fillCircle(10, 7, 7, TFT_ORANGE);
  explosionSprite2.fillCircle(10, 7, 4, TFT_YELLOW);
  explosionSprite2.fillCircle(10, 7, 1, TFT_WHITE);
  // Add spiky points at different angles
  explosionSprite2.fillTriangle(10, 1, 7, 5, 13, 5, TFT_RED);
  explosionSprite2.fillTriangle(17, 7, 13, 5, 13, 9, TFT_RED);
  explosionSprite2.fillTriangle(3, 7, 7, 5, 7, 9, TFT_RED);
  explosionSprite2.fillTriangle(10, 13, 7, 9, 13, 9, TFT_RED);
}

void createTreeSprite(int treeIndex)
{
  if (gameData.trees[treeIndex].sprite != nullptr)
  {
    gameData.trees[treeIndex].sprite->deleteSprite();
    delete gameData.trees[treeIndex].sprite;
  }

  gameData.trees[treeIndex].sprite = new TFT_eSprite(&tft);

  if (SPIFFS.exists("/tree.bin"))
  {
    gameData.trees[treeIndex].sprite->createSprite(TREE_WIDTH, TREE_HEIGHT);
    fs::File file = SPIFFS.open("/tree.bin", "r");
    if (file)
    {
      uint16_t buffer[TREE_WIDTH * TREE_HEIGHT];
      file.read((uint8_t *)buffer, TREE_WIDTH * TREE_HEIGHT * 2);
      gameData.trees[treeIndex].sprite->pushImage(0, 0, TREE_WIDTH, TREE_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    // Procedural fallback
    gameData.trees[treeIndex].sprite->createSprite(TREE_WIDTH, TREE_HEIGHT);
    gameData.trees[treeIndex].sprite->fillSprite(SKY_BLUE);

    int trunkWidth = 6;
    int trunkHeight = TREE_HEIGHT / 4;
    gameData.trees[treeIndex].sprite->fillRect(
        TREE_WIDTH / 2 - trunkWidth / 2, TREE_HEIGHT - trunkHeight,
        trunkWidth, trunkHeight, TREE_BROWN);

    for (int i = 0; i < 3; i++)
    {
      int layerHeight = (TREE_HEIGHT - trunkHeight) / 3;
      int layerWidth = TREE_WIDTH - i * 4;
      int layerY = trunkHeight + i * layerHeight;
      gameData.trees[treeIndex].sprite->fillTriangle(
          TREE_WIDTH / 2, layerY,
          TREE_WIDTH / 2 - layerWidth / 2, layerY + layerHeight,
          TREE_WIDTH / 2 + layerWidth / 2, layerY + layerHeight,
          TREE_GREEN);
    }
  }
}

void loadSpritesFromSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Load sleigh frame 1
  if (SPIFFS.exists("/sleigh0.bin"))
  {
    sleighSprite.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
    fs::File file = SPIFFS.open("/sleigh0.bin", "r");
    if (file)
    {
      uint16_t buffer[SLEIGH_WIDTH * SLEIGH_HEIGHT];
      file.read((uint8_t *)buffer, SLEIGH_WIDTH * SLEIGH_HEIGHT * 2);
      sleighSprite.pushImage(0, 0, SLEIGH_WIDTH, SLEIGH_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultSleigh();
  }

  // Load sleigh frame 2
  if (SPIFFS.exists("/sleigh1.bin"))
  {
    sleighSprite2.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
    fs::File file = SPIFFS.open("/sleigh1.bin", "r");
    if (file)
    {
      uint16_t buffer[SLEIGH_WIDTH * SLEIGH_HEIGHT];
      file.read((uint8_t *)buffer, SLEIGH_WIDTH * SLEIGH_HEIGHT * 2);
      sleighSprite2.pushImage(0, 0, SLEIGH_WIDTH, SLEIGH_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultSleigh2();
  }

  // Load duck frame 1
  if (SPIFFS.exists("/duck0.bin"))
  {
    duckSprite.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
    fs::File file = SPIFFS.open("/duck0.bin", "r");
    if (file)
    {
      uint16_t buffer[DUCK_WIDTH * DUCK_HEIGHT];
      file.read((uint8_t *)buffer, DUCK_WIDTH * DUCK_HEIGHT * 2);
      duckSprite.pushImage(0, 0, DUCK_WIDTH, DUCK_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultDuck();
  }

  // Load duck frame 2
  if (SPIFFS.exists("/duck1.bin"))
  {
    duckSprite2.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
    fs::File file = SPIFFS.open("/duck1.bin", "r");
    if (file)
    {
      uint16_t buffer[DUCK_WIDTH * DUCK_HEIGHT];
      file.read((uint8_t *)buffer, DUCK_WIDTH * DUCK_HEIGHT * 2);
      duckSprite2.pushImage(0, 0, DUCK_WIDTH, DUCK_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultDuck2();
  }

  // Load foe frame 1
  if (SPIFFS.exists("/foe0.bin"))
  {
    foeSprite.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
    fs::File file = SPIFFS.open("/foe0.bin", "r");
    if (file)
    {
      uint16_t buffer[DUCK_WIDTH * DUCK_HEIGHT];
      file.read((uint8_t *)buffer, DUCK_WIDTH * DUCK_HEIGHT * 2);
      foeSprite.pushImage(0, 0, DUCK_WIDTH, DUCK_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultFoe();
  }

  // Load foe frame 2
  if (SPIFFS.exists("/foe1.bin"))
  {
    foeSprite2.createSprite(DUCK_WIDTH, DUCK_HEIGHT);
    fs::File file = SPIFFS.open("/foe1.bin", "r");
    if (file)
    {
      uint16_t buffer[DUCK_WIDTH * DUCK_HEIGHT];
      file.read((uint8_t *)buffer, DUCK_WIDTH * DUCK_HEIGHT * 2);
      foeSprite2.pushImage(0, 0, DUCK_WIDTH, DUCK_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultFoe2();
  }

  // Load gift
  if (SPIFFS.exists("/gift0.bin"))
  {
    giftSprite.createSprite(GIFT_WIDTH, GIFT_HEIGHT);
    fs::File file = SPIFFS.open("/gift0.bin", "r");
    if (file)
    {
      uint16_t buffer[GIFT_WIDTH * GIFT_HEIGHT];
      file.read((uint8_t *)buffer, GIFT_WIDTH * GIFT_HEIGHT * 2);
      giftSprite.pushImage(0, 0, GIFT_WIDTH, GIFT_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultGift();
  }

  // Load explosion frame 1
  if (SPIFFS.exists("/explosion0.bin"))
  {
    explosionSprite.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
    fs::File file = SPIFFS.open("/explosion0.bin", "r");
    if (file)
    {
      uint16_t buffer[SLEIGH_WIDTH * SLEIGH_HEIGHT];
      file.read((uint8_t *)buffer, SLEIGH_WIDTH * SLEIGH_HEIGHT * 2);
      explosionSprite.pushImage(0, 0, SLEIGH_WIDTH, SLEIGH_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultExplosion();
  }

  // Load explosion frame 2
  if (SPIFFS.exists("/explosion1.bin"))
  {
    explosionSprite2.createSprite(SLEIGH_WIDTH, SLEIGH_HEIGHT);
    fs::File file = SPIFFS.open("/explosion1.bin", "r");
    if (file)
    {
      uint16_t buffer[SLEIGH_WIDTH * SLEIGH_HEIGHT];
      file.read((uint8_t *)buffer, SLEIGH_WIDTH * SLEIGH_HEIGHT * 2);
      explosionSprite2.pushImage(0, 0, SLEIGH_WIDTH, SLEIGH_HEIGHT, buffer);
      file.close();
    }
  }
  else
  {
    createDefaultExplosion2();
  }

  scoreSprite.createSprite(100, 16);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void initializeGameData()
{
  gameData.state = STATE_MENU;
  gameData.lastStateChange = millis();

  gameData.sleighY = 30;
  gameData.sleighVelocity = 0;
  gameData.sleighOldY = gameData.sleighY;
  gameData.sleighCrashed = false;
  gameData.sleighExploding = false;
  gameData.explosionStartTime = 0;

  gameData.currentScore = 0;
  gameData.buttonPressed = false;

  gameData.lastDuckFlap = 0;
  gameData.duckFrame = false;
  gameData.gameOverScreenDrawn = false;
  gameData.highScoreUpdated = false;

  // Initialize trees - spread them out at start
  for (int i = 0; i < TREE_COUNT; i++)
  {
    gameData.trees[i].pos.x = SCREEN_WIDTH + (i * OBSTACLE_SPAWN_DISTANCE);
    gameData.trees[i].pos.y = PLAYFIELD_HEIGHT - TREE_HEIGHT;
    gameData.trees[i].pos.oldX = gameData.trees[i].pos.x;
    gameData.trees[i].pos.oldY = gameData.trees[i].pos.y;
    gameData.trees[i].active = (i < 3); // Only first 3 are active at start
    gameData.trees[i].spawnTimer = 0;
    gameData.trees[i].scored = false;
    gameData.trees[i].sprite = nullptr;
    createTreeSprite(i);
  }

  // Initialize flying obstacles - spread them out at start
  for (int i = 0; i < DUCK_COUNT; i++)
  {
    gameData.flyingObstacles[i].pos.x = SCREEN_WIDTH + (i * OBSTACLE_SPAWN_DISTANCE) + OBSTACLE_SPAWN_OFFSET;
    gameData.flyingObstacles[i].pos.y = random(5, 40);
    gameData.flyingObstacles[i].pos.oldX = gameData.flyingObstacles[i].pos.x;
    gameData.flyingObstacles[i].pos.oldY = gameData.flyingObstacles[i].pos.y;
    gameData.flyingObstacles[i].active = (i < 3); // Only first 3 are active at start
    gameData.flyingObstacles[i].spawnTimer = 0;
    gameData.flyingObstacles[i].scored = false;
    gameData.flyingObstacles[i].lastFlap = 0;
    gameData.flyingObstacles[i].flapFrame = false;
    gameData.flyingObstacles[i].falling = false;
    gameData.flyingObstacles[i].fallVelocity = 0;
    // Randomly assign type: 80% duck, 16% gift, 4% foe
    int randType = random(100);
    if (randType < 80)
    {
      gameData.flyingObstacles[i].type = TYPE_DUCK;
    }
    else if (randType < 96)
    {
      gameData.flyingObstacles[i].type = TYPE_GIFT;
    }
    else
    {
      gameData.flyingObstacles[i].type = TYPE_FOE;
      // Foe spawns at middle height for easier combat
      gameData.flyingObstacles[i].pos.y = PLAYFIELD_HEIGHT / 2 - DUCK_HEIGHT / 2;
    }
  }
}

void initializeSnow()
{
  for (int i = 0; i < MAX_SNOWFLAKES; i++)
  {
    gameData.snowflakes[i].x = random(0, SCREEN_WIDTH);
    gameData.snowflakes[i].y = random(-20, SCREEN_HEIGHT);
    gameData.snowflakes[i].active = true;
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Load high score from NVM
  preferences.begin("flappysleigh", false);
  for (int mode = MODE_NORMAL; mode <= MODE_CHEAT; mode++)
  {
    char key[16];
    sprintf(key, "highscore%d", mode);
    gameData.foreverHighScore[mode] = preferences.getInt(key, 0);
    gameData.sessionHighScore[mode] = 0;
  }

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(SKY_BLUE);

  loadSpritesFromSPIFFS();
  initializeGameData();

  tft.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
}
void clearScreen()
{
  tft.fillScreen(SKY_BLUE);
  tft.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);
}
// ============================================================================
// INPUT HANDLING
// ============================================================================

void handleInput()
{
  bool currentButton = !digitalRead(BUTTON_PIN);   // Active LOW
  bool currentButton2 = !digitalRead(BUTTON2_PIN); // Active LOW
  bool button1Event = false;
  bool button2Event = false;

  if (currentButton2 && !gameData.buttonPressed)
  {
    gameData.buttonPressed = true;
    button2Event = true;
  }
  if (currentButton && !gameData.buttonPressed)
  {
    gameData.buttonPressed = true;
    button1Event = true;
  }
  if (!currentButton && !currentButton2)
  {
    gameData.buttonPressed = false;
    return;
  }
  bool action = button1Event || button2Event;
  switch (gameData.state)
  {
  case STATE_MENU:
    if (button2Event)
    {
      gameData.gameMode = (GameMode)((gameData.gameMode + 1) % 3);
    }
    if (button1Event)
    {
      gameData.state = STATE_PLAYING;
      gameData.lastStateChange = millis();
      clearScreen();
    }
    break;
  case STATE_PLAYING:
    if (action && !gameData.sleighCrashed)
    {
      gameData.sleighVelocity = JUMP_STRENGTH;
    }
    else if (action && gameData.sleighCrashed && gameData.gameMode == MODE_CHEAT)
    {
      if (gameData.crashingStartTime + 300 < millis())
      {
        gameData.sleighCrashed = false;
        gameData.sleighVelocity = JUMP_STRENGTH / 2;
      }
    }
    break;
  case STATE_GAME_OVER:
    if (action)
    {
      // Reset game
      gameData.state = STATE_MENU;
      gameData.lastStateChange = millis();
      gameData.currentScore = 0;
      gameData.gameOverScreenDrawn = false;
      gameData.highScoreUpdated = false;
      initializeGameData();
      clearScreen();
    }
  }
}

// ============================================================================
// PHYSICS & UPDATES
// ============================================================================

void updatePhysics()
{
  // Stop physics when exploding
  if (gameData.sleighExploding)
  {
    return;
  }

  float currentGravity = GRAVITY;

  // Double gravity when sleigh is crashed
  if (gameData.sleighCrashed)
  {
    currentGravity *= 2.0;
  }

  gameData.sleighVelocity += currentGravity;
  gameData.sleighOldY = gameData.sleighY;
  gameData.sleighY += gameData.sleighVelocity;
}

void updateFlyingAnimation()
{
  uint32_t currentTime = millis();

  // Update animation for each flying obstacle independently
  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (gameData.flyingObstacles[i].active)
    {
      // Only ducks and foes have flapping animation
      if (gameData.flyingObstacles[i].type == TYPE_DUCK ||
          gameData.flyingObstacles[i].type == TYPE_FOE)
      {
        // Check if it's time to switch frames
        if (currentTime - gameData.flyingObstacles[i].lastFlap >= DUCK_FLAP_INTERVAL)
        {
          gameData.flyingObstacles[i].flapFrame = !gameData.flyingObstacles[i].flapFrame;
          gameData.flyingObstacles[i].lastFlap = currentTime;
        }
      }
    }

    // Update falling foes
    if (gameData.flyingObstacles[i].falling)
    {
      gameData.flyingObstacles[i].fallVelocity += GRAVITY;
      gameData.flyingObstacles[i].pos.move(0, (int)gameData.flyingObstacles[i].fallVelocity);

      // Remove if hit ground
      if (gameData.flyingObstacles[i].pos.y >= PLAYFIELD_HEIGHT)
      {
        gameData.flyingObstacles[i].falling = false;
        gameData.flyingObstacles[i].active = false;
        gameData.flyingObstacles[i].spawnTimer = currentTime + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
  }
}

// Check if an obstacle at the given position would overlap with any active obstacles
bool obstacleOverlapsWithOthers(int newX, int newY, int obstacleIndex)
{
  const int X_MARGIN = 30; // Minimum horizontal distance between obstacles
  const int Y_MARGIN = 20; // Minimum vertical distance between obstacles

  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (i == obstacleIndex || !gameData.flyingObstacles[i].active || gameData.flyingObstacles[i].falling)
    {
      continue; // Skip self, inactive, and falling obstacles
    }

    // Check both x and y overlap with margin
    int xDistance = newX - gameData.flyingObstacles[i].pos.x;
    if (xDistance < 0)
      xDistance = -xDistance;

    int yDistance = newY - gameData.flyingObstacles[i].pos.y;
    if (yDistance < 0)
      yDistance = -yDistance;

    // If both distances are too small, there's an overlap
    if (xDistance < X_MARGIN && yDistance < Y_MARGIN)
    {
      return true; // Overlap detected
    }
  }

  return false; // No overlap
}

bool treeOverlapsWithOthers(int newX, int treeIndex)
{
  const int OVERLAP_MARGIN = 20; // Minimum distance between trees

  for (int i = 0; i < TREE_COUNT; i++)
  {
    if (i == treeIndex || !gameData.trees[i].active)
    {
      continue; // Skip self and inactive trees
    }

    // Check if x positions are too close (simple horizontal overlap detection)
    int distance = newX - gameData.trees[i].pos.x;
    if (distance < 0)
      distance = -distance;

    if (distance < OVERLAP_MARGIN)
    {
      return true; // Overlap detected
    }
  }

  return false; // No overlap
}

void updateObstacles()
{
  uint32_t currentTime = millis();
  if (gameData.sleighExploding)
  {
    return;
  }
  // Update trees
  for (int i = 0; i < TREE_COUNT; i++)
  {
    if (gameData.trees[i].active)
    {
      // Move active tree
      gameData.trees[i].pos.move(-OBSTACLE_SPEED);

      // Check if tree went off-screen
      if (gameData.trees[i].pos.x < -TREE_WIDTH)
      {
        gameData.trees[i].active = false;
        gameData.trees[i].scored = false;
        // Start spawn delay timer
        gameData.trees[i].spawnTimer = currentTime + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
    else
    {
      // Check if spawn delay is over
      if (currentTime >= gameData.trees[i].spawnTimer)
      {
        // Try to respawn tree
        gameData.trees[i].pos.x = SCREEN_WIDTH;
        gameData.trees[i].pos.y = PLAYFIELD_HEIGHT - TREE_HEIGHT;
        gameData.trees[i].pos.oldX = gameData.trees[i].pos.x;
        gameData.trees[i].pos.oldY = gameData.trees[i].pos.y;

        // Check if this overlaps with other trees
        if (treeOverlapsWithOthers(gameData.trees[i].pos.x, i))
        {
          // Overlap detected, reschedule spawn
          gameData.trees[i].spawnTimer = currentTime + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
        }
        else
        {
          // No overlap, activate the tree
          gameData.trees[i].active = true;
          gameData.trees[i].scored = false;
        }
      }
    }
  }

  // Update flying obstacles
  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (gameData.flyingObstacles[i].active && !gameData.flyingObstacles[i].falling)
    {
      // Move active obstacle
      gameData.flyingObstacles[i].pos.move(-OBSTACLE_SPEED);

      // Check if obstacle went off-screen
      if (gameData.flyingObstacles[i].pos.x < -DUCK_WIDTH)
      {
        gameData.flyingObstacles[i].active = false;
        gameData.flyingObstacles[i].scored = false;
        // Start spawn delay timer
        gameData.flyingObstacles[i].spawnTimer = currentTime + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
      }
    }
    else if (!gameData.flyingObstacles[i].active && !gameData.flyingObstacles[i].falling)
    {
      // Check if spawn delay is over
      if (currentTime >= gameData.flyingObstacles[i].spawnTimer)
      {
        // Respawn obstacle
        gameData.flyingObstacles[i].pos.x = SCREEN_WIDTH;
        gameData.flyingObstacles[i].pos.y = random(5, 40);
        gameData.flyingObstacles[i].pos.oldX = gameData.flyingObstacles[i].pos.x;
        gameData.flyingObstacles[i].pos.oldY = gameData.flyingObstacles[i].pos.y;
        gameData.flyingObstacles[i].falling = false;
        gameData.flyingObstacles[i].fallVelocity = 0;
        // Randomly assign new type: 80% duck, 16% gift, 4% foe
        int randType = random(100);
        if (randType < 80)
        {
          gameData.flyingObstacles[i].type = TYPE_DUCK;
        }
        else if (randType < 96)
        {
          gameData.flyingObstacles[i].type = TYPE_GIFT;
        }
        else
        {
          gameData.flyingObstacles[i].type = TYPE_FOE;
          // Foe spawns at middle height for easier combat
          gameData.flyingObstacles[i].pos.y = PLAYFIELD_HEIGHT / 2 - DUCK_HEIGHT / 2;
        }

        // Check if this overlaps with other obstacles
        if (obstacleOverlapsWithOthers(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y, i))
        {
          // Overlap detected, reschedule spawn
          gameData.flyingObstacles[i].spawnTimer = currentTime + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
        }
        else
        {
          // No overlap, activate the obstacle
          gameData.flyingObstacles[i].active = true;
          gameData.flyingObstacles[i].scored = false;
        }
      }
    }
  }
}

void updateScore()
{
  // Don't score points if sleigh has crashed
  if (gameData.sleighCrashed)
  {
    return;
  }

  for (int i = 0; i < TREE_COUNT; i++)
  {
    if (gameData.trees[i].active && !gameData.trees[i].scored &&
        gameData.trees[i].pos.x + TREE_WIDTH < SLEIGH_START_X)
    {
      gameData.trees[i].scored = true;
      gameData.currentScore++;
    }
  }

  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (gameData.flyingObstacles[i].active && !gameData.flyingObstacles[i].scored &&
        !gameData.flyingObstacles[i].falling &&
        gameData.flyingObstacles[i].pos.x + DUCK_WIDTH < SLEIGH_START_X)
    {
      gameData.flyingObstacles[i].scored = true;
      // Only ducks and foes give points for passing
      if (gameData.flyingObstacles[i].type == TYPE_DUCK ||
          gameData.flyingObstacles[i].type == TYPE_FOE)
      {
        gameData.currentScore++;
      }
    }
  }
}

void updateSnow()
{
  for (int i = 0; i < MAX_SNOWFLAKES; i++)
  {
    if (gameData.snowflakes[i].active)
    {
      if (gameData.snowflakes[i].y < SCREEN_HEIGHT - 1)
      {
        uint16_t pixelBelow = tft.readPixel(gameData.snowflakes[i].x, gameData.snowflakes[i].y + 1);

        if (pixelBelow == SKY_BLUE)
        {
          tft.drawPixel(gameData.snowflakes[i].x, gameData.snowflakes[i].y, SKY_BLUE);
          gameData.snowflakes[i].y++;
          tft.drawPixel(gameData.snowflakes[i].x, gameData.snowflakes[i].y, WHITE);
        }
        else
        {
          gameData.snowflakes[i].x = random(0, SCREEN_WIDTH);
          gameData.snowflakes[i].y = 0;
        }
      }
      else
      {
        gameData.snowflakes[i].x = random(0, SCREEN_WIDTH);
        gameData.snowflakes[i].y = 0;
      }
    }
  }
}

// ============================================================================
// COLLISION DETECTION
// ============================================================================

void checkCollisions()
{
  // ceiling
  if (gameData.sleighY < 2)
  {
    gameData.sleighY = 2;
    gameData.sleighVelocity = -gameData.sleighVelocity / 3; // Bounce effect
  }
  // Ground
  if (!gameData.sleighCrashed && gameData.sleighY >= PLAYFIELD_HEIGHT - SLEIGH_HITBOX)
  {
    gameData.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX;
    gameData.sleighVelocity = -gameData.sleighVelocity; // Bounce effect
    gameData.sleighCrashed = true;
    gameData.crashingStartTime = millis();
    return;
  }
  // Check if crashed sleigh hit the ground - start explosion animation
  if (gameData.sleighCrashed && !gameData.sleighExploding && gameData.sleighY >= PLAYFIELD_HEIGHT - SLEIGH_HITBOX)
  {
    gameData.sleighExploding = true;
    gameData.explosionStartTime = millis();
    gameData.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX; // Lock at ground
    gameData.sleighVelocity = 0;
    return;
  }

  // Check if explosion animation is complete (1000 milliseconds)
  if (gameData.sleighExploding && millis() - gameData.explosionStartTime >= 1000)
  {
    gameData.state = STATE_GAME_OVER;
    gameData.lastStateChange = millis();
    return;
  }

  // Trees
  for (int i = 0; i < TREE_COUNT; i++)
  {
    if (gameData.trees[i].active &&
        gameData.trees[i].pos.x < SLEIGH_START_X + SLEIGH_HITBOX &&
        gameData.trees[i].pos.x + TREE_WIDTH > SLEIGH_START_X + 2)
    {
      if (gameData.sleighY + SLEIGH_HITBOX > PLAYFIELD_HEIGHT - TREE_HEIGHT)
      {
        // Collision with tree - set crashed and let sleigh fall
        gameData.sleighCrashed = true;
        gameData.crashingStartTime = millis();
        gameData.sleighY = PLAYFIELD_HEIGHT - TREE_HEIGHT - SLEIGH_HITBOX;
        gameData.sleighVelocity = -gameData.sleighVelocity / 2; // Bounce effect
        return;
      }
    }
  }

  // Flying obstacles (ducks, foes, gifts)
  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (gameData.flyingObstacles[i].active && !gameData.flyingObstacles[i].falling &&
        gameData.flyingObstacles[i].pos.x < SLEIGH_START_X + SLEIGH_HITBOX &&
        gameData.flyingObstacles[i].pos.x + DUCK_HITBOX > SLEIGH_START_X + 2)
    {
      if (gameData.sleighY < gameData.flyingObstacles[i].pos.y + DUCK_HEIGHT &&
          gameData.sleighY + SLEIGH_HEIGHT > gameData.flyingObstacles[i].pos.y)
      {

        // Collision detected - handle based on obstacle type
        ObstacleType type = gameData.flyingObstacles[i].type;

        if (type == TYPE_DUCK)
        {
          // Duck: set crashed and let sleigh fall
          gameData.sleighCrashed = true;
          gameData.crashingStartTime = millis();
          if (gameData.sleighVelocity < 0)
          {
            gameData.sleighVelocity = -gameData.sleighVelocity; // bump downwards
          }
          return;
        }
        else if (type == TYPE_FOE)
        {
          // Foe: check if we're falling (hitting from above) or flapping
          if (gameData.sleighVelocity > 0)
          {
            // Falling/moving down - kill the foe
            gameData.flyingObstacles[i].falling = true;
            gameData.flyingObstacles[i].fallVelocity = 2.0;
            gameData.currentScore += 20;
            // Give sleigh a bounce
            gameData.sleighVelocity = -3.0;
          }
          else if (!gameData.sleighCrashed)
          {
            // Flapping/moving up - lose points and game over
            gameData.currentScore -= 10;
            if (gameData.currentScore < 0)
              gameData.currentScore = 0;
            gameData.sleighCrashed = true;
            gameData.crashingStartTime = millis();
            // Give sleigh a big bounce
            gameData.sleighVelocity = -6.0;
            return;
          }
        }
        else if (type == TYPE_GIFT)
        {
          // Gift: collect for 10 points
          gameData.currentScore += 10;
          // Clear the gift sprite position immediately
          tft.fillRect(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y,
                       DUCK_WIDTH, DUCK_HEIGHT, SKY_BLUE);
          gameData.flyingObstacles[i].active = false;
          gameData.flyingObstacles[i].spawnTimer = millis() + random(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
        }
      }
    }
  }
}

void updateHighScores()
{
  if (!gameData.highScoreUpdated)
  {
    if (gameData.currentScore > gameData.sessionHighScore[gameData.gameMode])
    {
      gameData.sessionHighScore[gameData.gameMode] = gameData.currentScore;
    }
    if (gameData.currentScore > gameData.foreverHighScore[gameData.gameMode])
    {
      gameData.foreverHighScore[gameData.gameMode] = gameData.currentScore;
      char key[16];
      sprintf(key, "highscore%d", gameData.gameMode);
      preferences.putInt(key, gameData.foreverHighScore[gameData.gameMode]);
    }
    gameData.highScoreUpdated = true;
  }
}

// ============================================================================
// RENDERING
// ============================================================================

void drawMenu()
{
  tft.setTextColor(WHITE, SKY_BLUE, true);
  tft.setTextSize(2);
  tft.drawString("Appuyez!", 70, 30);
  tft.setTextSize(1);
  tft.drawString("L'aventure du Pere Noel!", 40, 70);
  if (gameData.gameMode == MODE_SPEED)
  {
    tft.drawString("Mode Rapide", 85, 100);
  }
  else if (gameData.gameMode == MODE_CHEAT)
  {
    tft.drawString("Mode  Cheat", 85, 100);
  }
  else
  {
    tft.drawString("Mode Normal", 85, 100);
  }
  int speed = 600;
  float speed2 = 500;
  if (gameData.gameMode != MODE_NORMAL)
  {
    speed = 300;
    speed2 = 250;
  }

  if (millis() / speed % 2 == 0)
  {
    duckSprite.pushSprite(SCREEN_WIDTH - 40, 30);
  }
  else
  {
    duckSprite2.pushSprite(SCREEN_WIDTH - 40, 30);
  }
  tft.fillRect(10, 20, SLEIGH_WIDTH, SLEIGH_HEIGHT + 20, SKY_BLUE);
  if (cos(millis() / speed2) > 0)
  {
    sleighSprite2.pushSprite(10, 30 + sin(millis() / speed2) * 10);
  }
  else
  {
    sleighSprite.pushSprite(10, 30 + sin(millis() / speed2) * 10);
  }
  tft.fillRect(SCREEN_WIDTH - 30, 90, SLEIGH_WIDTH, SLEIGH_HEIGHT + 20, SKY_BLUE);
  if (gameData.gameMode == MODE_CHEAT)
  {
    if (cos(millis() / 200.0) > 0)
    {
      foeSprite2.pushSprite(SCREEN_WIDTH - 30, 100 + sin(millis() / 200.0) * 10);
    }
    else
    {
      foeSprite.pushSprite(SCREEN_WIDTH - 30, 100 + sin(millis() / 200.0) * 10);
    }
  }
  tft.drawString("https://github.com/tardyp/ttgo-noel", 10, 122);
}

void drawGameplay()
{

  // Draw obstacles
  for (int i = 0; i < TREE_COUNT; i++)
  {
    if (gameData.trees[i].active)
    {
      // Clear OLD tree position
      tft.fillRect(gameData.trees[i].pos.oldX, gameData.trees[i].pos.oldY,
                   TREE_WIDTH, TREE_HEIGHT, SKY_BLUE);

      // Draw tree at NEW position
      if (gameData.trees[i].sprite != nullptr)
      {
        gameData.trees[i].sprite->pushSprite(gameData.trees[i].pos.x, gameData.trees[i].pos.y);
      }
    }
  }

  for (int i = 0; i < DUCK_COUNT; i++)
  {
    if (gameData.flyingObstacles[i].active || gameData.flyingObstacles[i].falling)
    {
      // Clear OLD position
      tft.fillRect(gameData.flyingObstacles[i].pos.oldX, gameData.flyingObstacles[i].pos.oldY,
                   DUCK_WIDTH, DUCK_HEIGHT, SKY_BLUE);

      // Draw obstacle at current position based on type
      ObstacleType type = gameData.flyingObstacles[i].type;

      if (type == TYPE_DUCK)
      {
        // Draw duck with animation
        if (gameData.flyingObstacles[i].flapFrame)
        {
          duckSprite2.pushSprite(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y);
        }
        else
        {
          duckSprite.pushSprite(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y);
        }
      }
      else if (type == TYPE_FOE)
      {
        // Draw foe with animation
        if (gameData.flyingObstacles[i].flapFrame)
        {
          foeSprite2.pushSprite(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y);
        }
        else
        {
          foeSprite.pushSprite(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y);
        }
      }
      else if (type == TYPE_GIFT)
      {
        // Draw gift (no animation)
        giftSprite.pushSprite(gameData.flyingObstacles[i].pos.x, gameData.flyingObstacles[i].pos.y);
      }
    }
  }
  // // Clear sleigh area
  tft.fillRect(SLEIGH_START_X, (int)gameData.sleighOldY - 2,
               SLEIGH_WIDTH + 2, SLEIGH_HEIGHT + 4, SKY_BLUE);

  // Draw sleigh - alternate between explosion sprites if exploding
  if (gameData.sleighExploding)
  {
    // make sure we are above the ground
    gameData.sleighY = PLAYFIELD_HEIGHT - SLEIGH_HITBOX * 2;
    // Alternate between explosion frames every 300ms
    if ((millis() / 300) % 2 == 0)
    {
      explosionSprite.pushSprite(SLEIGH_START_X, (int)gameData.sleighY);
    }
    else
    {
      explosionSprite2.pushSprite(SLEIGH_START_X, (int)gameData.sleighY);
    }
  }
  else
  {
    if (gameData.sleighCrashed && !gameData.sleighExploding && millis() / 100 % 2 == 0)
    {
      // Flashing effect when crashed
      return;
    }
    // Normal rendering: choose frame based on velocity direction
    // Frame 0 when moving up (negative velocity), Frame 1 when moving down (positive velocity)
    if (gameData.sleighVelocity < 0)
    {
      sleighSprite.pushSprite(SLEIGH_START_X, (int)gameData.sleighY);
    }
    else
    {
      sleighSprite2.pushSprite(SLEIGH_START_X, (int)gameData.sleighY);
    }
  }

  // Draw ground
  // tft.fillRect(0, PLAYFIELD_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_GREEN);

  // Draw score
  scoreSprite.fillSprite(GROUND_GREEN);
  scoreSprite.setTextColor(WHITE, GROUND_GREEN);
  scoreSprite.setTextSize(1);
  scoreSprite.drawString("Score: " + String(gameData.currentScore), 0, 2);
  scoreSprite.pushSprite(5, PLAYFIELD_HEIGHT);
}

void drawGameOver()
{
  if (!gameData.gameOverScreenDrawn)
  {
    initializeSnow();
    tft.fillRect(20, 30, 200, 80, TFT_BLACK);
    tft.drawRect(20, 30, 200, 80, WHITE);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Perdu!!", 80, 38);
    tft.setTextSize(1);
    tft.setTextColor(WHITE, TFT_BLACK);
    tft.drawString("Score: " + String(gameData.currentScore), 75, 60);
    tft.drawString("Meilleur: " + String(gameData.sessionHighScore[gameData.gameMode]), 55, 75);
    tft.drawString("Record: " + String(gameData.foreverHighScore[gameData.gameMode]), 65, 88);
    tft.drawString("Appuyez pour recommencer", 35, 100);
    gameData.gameOverScreenDrawn = true;
  }

  updateSnow();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop()
{
  handleInput();

  switch (gameData.state)
  {
  case STATE_MENU:
    drawMenu();
    delay(30);
    break;

  case STATE_PLAYING:
    updatePhysics();
    updateObstacles();
    updateFlyingAnimation();
    checkCollisions();
    updateScore();
    drawGameplay();
    delay(30);
    break;

  case STATE_GAME_OVER:
    updateHighScores();
    drawGameOver();
    delay(30);
    break;
  }
}
