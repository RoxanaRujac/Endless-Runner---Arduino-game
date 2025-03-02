#include <LiquidCrystal.h>
// Pin Definitions
#define PIN_BUTTON 8         // Button for jumping or toggling hero position
#define PIN_READWRITE 10     // RW pin of the LCD (set to LOW for write-only)
#define PIN_CONTRAST 12      // Contrast control pin
#define PIN_RESET 13         // Button to reset the game

// Sprite Definitions
#define SPRITE_RUN1 1        // Running pose 1
#define SPRITE_RUN2 2        // Running pose 2
#define SPRITE_JUMP 3        // Jumping sprite
#define SPRITE_JUMP_UPPER '.' // Upper part of the jump (a simple head character)
#define SPRITE_JUMP_LOWER 4  // Lower part of the jump
#define SPRITE_TERRAIN_EMPTY '.' // Empty terrain
#define SPRITE_TERRAIN_SOLID 5   // Solid ground block
#define SPRITE_TERRAIN_SOLID_RIGHT 6 // Right edge of a solid block
#define SPRITE_TERRAIN_SOLID_LEFT 7  // Left edge of a solid block

// Hero Position Definitions
#define HERO_HORIZONTAL_POSITION 1 // Horizontal position of the hero on the LCD
#define TERRAIN_WIDTH 16           // Width of the terrain arrays
#define TERRAIN_EMPTY 0            // Represents no terrain
#define TERRAIN_LOWER_BLOCK 1      // Lower terrain block
#define TERRAIN_UPPER_BLOCK 2      // Upper terrain block

// Hero States (for animation and movement)
#define HERO_POSITION_OFF 0  // Hero is invisible
#define HERO_POSITION_RUN_LOWER_1 1  // Hero is running on lower row (pose 1)
#define HERO_POSITION_RUN_LOWER_2 2  // (pose 2)
#define HERO_POSITION_JUMP_1 3   // Starting a jump
#define HERO_POSITION_JUMP_2 4   // Half-way up
#define HERO_POSITION_JUMP_3 5   // Jump is on upper row
#define HERO_POSITION_JUMP_4 6   // Jump is on upper row
#define HERO_POSITION_JUMP_5 7   // Jump is on upper row
#define HERO_POSITION_JUMP_6 8   // Jump is on upper row
#define HERO_POSITION_JUMP_7 9   // Half-way down
#define HERO_POSITION_JUMP_8 10  // About to land

#define HERO_POSITION_RUN_UPPER_1 11  // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12  // (pose 2)
// LCD Setup
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // Initialize the LCD with specified pins
static char terrainUpper[TERRAIN_WIDTH + 1]; // Upper row terrain buffer
static char terrainLower[TERRAIN_WIDTH + 1]; // Lower row terrain buffer
static bool buttonPushed = false;           // Tracks if the button is pushed
static bool heroOnUpperSide = false;        // Tracks if the hero is on the upper row

// Level and Speed Configurations
static byte level = 1;  // Current game level
static unsigned long levelTime = 0; // Tracks the time spent on the current level
static unsigned int levelThreshold[] = {10, 30, 50, 70, 100}; // Score thresholds for levels
static unsigned int levelSpeed[] = {150, 100, 50, 30, 15}; // Speed for each level (lower = faster)

// Function to initialize graphics (custom characters for the LCD)
void initializeGraphics() {
  static byte graphics[] = {
    // Sprite data for various states
    // Each sprite is an 8x5 pixel matrix, stored as bytes

    // Run position 1
    B01100,
    B01100,
    B00000,
    B01110,
    B11100,
    B01100,
    B11010,
    B10011,
    // Run position 2
    B01100,
    B01100,
    B00000,
    B01100,
    B01100,
    B01100,
    B01100,
    B01110,
    // Jump
    B01100,
    B01100,
    B00000,
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    // Jump lower
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };

  int i;
  for (i = 0; i < 7; ++i) {
    lcd.createChar(i + 1, &graphics[i * 8]);// Load each sprite into the LCD memory
  }
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY; // Initialize upper terrain to empty
    terrainLower[i] = SPRITE_TERRAIN_EMPTY; // Initialize lower terrain to empty
  }
}


// Function to advance terrain (shift left and add new terrain at the end)
void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    
    // Update terrain based on the current and next block type

    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

// Function to draw the hero on the screen and check for collisions
bool drawHero(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  // Save current terrain state at hero's position
  // Determine the hero's sprite based on the current position
  // Check for collisions with terrain
  // Update the LCD display with terrain and score
  // Restore terrain state after drawing
  bool collide = false;
  char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
  byte upper, lower;

  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }

  if (upper != SPRITE_TERRAIN_EMPTY) {
    terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave != SPRITE_TERRAIN_EMPTY);
  }
  if (lower != SPRITE_TERRAIN_EMPTY) {
    terrainLower[HERO_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave != SPRITE_TERRAIN_EMPTY);
  }

  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(terrainUpper);
  lcd.setCursor(0, 1);
  lcd.print(terrainLower);

  lcd.setCursor(13, 0);
  lcd.print(score);

  terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;

  return collide;
}

void setup() {
   pinMode(PIN_READWRITE, OUTPUT); // Set RW pin to write-only
  digitalWrite(PIN_READWRITE, LOW);
  pinMode(PIN_CONTRAST, OUTPUT); // Set contrast control pin
  digitalWrite(PIN_CONTRAST, LOW);
  pinMode(PIN_BUTTON, INPUT_PULLUP); // Configure button pin with pull-up resistor
  pinMode(PIN_RESET, INPUT_PULLUP);  // Configure reset button with pull-up resistor

  initializeGraphics(); // Initialize LCD and custom characters
  lcd.begin(16, 2);     // Start LCD with 16x2 dimensions

  // Display initial message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press 1st button");
  lcd.setCursor(4, 1);
  lcd.print("to start");
  while (digitalRead(PIN_BUTTON) == HIGH) { // Wait for button press to start
    delay(100);
  }
  lcd.clear();
}

// static array for keeping last 3 scores
static unsigned int lastScores[3] = {0, 0, 0};

// Function to update the last three scores
void updateLastScores(unsigned int newScore) {
  lastScores[2] = lastScores[1];
  lastScores[1] = lastScores[0];
  lastScores[0] = newScore;
}

void loop() {
 static byte heroPos = HERO_POSITION_RUN_LOWER_1; // Hero's current position
  static byte newTerrainType = TERRAIN_EMPTY;     // Type of new terrain to add
  static byte newTerrainDuration = 1;            // Duration of current terrain type
  static bool playing = true;                    // Tracks if the game is active
  static unsigned int distance = 0;              // Distance traveled (score)

  if (digitalRead(PIN_RESET) == LOW) {
   // Reset the game when reset button is pressed

    level = 1;
    distance = 0;
    heroPos = HERO_POSITION_RUN_LOWER_1;
    newTerrainType = TERRAIN_EMPTY;
    newTerrainDuration = 1;
    playing = true;
    levelTime = millis();
    memset(terrainUpper, SPRITE_TERRAIN_EMPTY, sizeof(terrainUpper));
    memset(terrainLower, SPRITE_TERRAIN_EMPTY, sizeof(terrainLower));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Press 1st button");
    lcd.setCursor(4, 1);
    lcd.print("to start");
    while (digitalRead(PIN_BUTTON) == HIGH) {
      delay(100);
    }
    lcd.clear();
  }

  if (playing) {
   // Toggle hero's position between upper and lower row on button press
    if (digitalRead(PIN_BUTTON) == LOW) {
      heroOnUpperSide = !heroOnUpperSide;
      delay(200);  // Debounce pentru buton
    }

    if (heroOnUpperSide) {
      heroPos = (heroPos == HERO_POSITION_RUN_UPPER_1) ? HERO_POSITION_RUN_UPPER_2 : HERO_POSITION_RUN_UPPER_1;
    } else {
      heroPos = (heroPos == HERO_POSITION_RUN_LOWER_1) ? HERO_POSITION_RUN_LOWER_2 : HERO_POSITION_RUN_LOWER_1;
    }

    // Update terrain and hero position based on game state
    // Check for collisions and update score and level
    advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
    advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

    if (--newTerrainDuration == 0) {
      if (newTerrainType == TERRAIN_EMPTY) {
        newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
        newTerrainDuration = 2 + random(10);
      } else {
        newTerrainType = TERRAIN_EMPTY;
        newTerrainDuration = 10 + random(10);
      }
    }

    if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3)) {
      playing = false;
      updateLastScores(distance >> 3);
    } else {
      ++distance;
      if (distance >> 3 >= levelThreshold[level - 1]) {
        level++;
        if (level <= 5) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Level ");
          lcd.print(level);
          delay(1000);
        } else {
          playing = false;
        }
        levelTime = millis();
      }
    }

    delay(levelSpeed[level - 1]);
  } else {
  // Handle end of game (display score or level-up messages)

    lcd.clear();
    if (level == 6) {
      lcd.setCursor(0, 0);
      lcd.print("Congratulations!");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Last Scores:");
      for (int i = 0; i < 3; i++) {
        lcd.setCursor(0, 1);
        lcd.print(i + 1);
        lcd.print(": ");
        lcd.print(lastScores[i]);
        delay(2000);
      }
    } else {
      lcd.setCursor(4, 0);
      lcd.print("Game Over");
      lcd.setCursor(4, 1);
      lcd.print("Score: ");
      lcd.print(distance >> 3);
    }
    delay(250);
  }
}