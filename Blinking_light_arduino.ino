#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   3
#define DATA_PIN  4
#define CS_PIN    2

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Define the game parameters
const int numRows = 8;
const int numCols = 24;
int gameArea[numRows][numCols] = {0};
int score = 0;
int speed = 500; // Initial speed (delay in ms)
bool gameOver = false;

// Tetromino shapes
const byte shapes[7][4] = {
  {0x0F, 0x4444, 0x0F, 0x4444},  // I
  {0x446, 0x0E8, 0x644, 0x0E2},  // J
  {0x622, 0x0E2, 0x446, 0x0E8},  // L
  {0xC6, 0x0C6, 0xC6, 0x0C6},    // O
  {0x6C, 0x0C6, 0x6C, 0x0C6},    // S
  {0x264, 0x0C6, 0x264, 0x0C6},  // T
  {0xC44, 0x0E4, 0x644, 0x0E4}   // Z
};

// Current piece information
int currentShape = 0;
int currentRotation = 0;
int currentX = 0;
int currentY = 0;

void setup() {
  mx.begin();
  display.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0);
  mx.clear();
  display.setIntensity(0);
  display.displayClear();
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // Initialize the first piece
  spawnPiece();
}

void loop() {
  if (!gameOver) {
    handleInput();
    updateGame();
    drawGame();
    delay(speed);
  } else {
    display.displayClear();
    display.setTextAlignment(PA_CENTER);
    display.print("Game Over");
    display.displayAnimate();
  }
}

void handleInput() {
  if (Serial.available()) {
    char command = Serial.read();
    switch (command) {
      case 'l': movePiece(-1, 0); break; // Left
      case 'r': movePiece(1, 0); break;  // Right
      case 'd': movePiece(0, 1); break;  // Down
      case 'u': rotatePiece(); break;    // Rotate
    }
  }
}

void updateGame() {
  // Move piece down automatically
  if (!movePiece(0, 1)) {
    // If it cannot move down, fix it in place and spawn a new piece
    fixPiece();
    clearLines();
    spawnPiece();
    // Check for game over condition
    if (!isValidMove(currentShape, currentRotation, currentX, currentY)) {
      gameOver = true;
    }
  }
}

void drawGame() {
  mx.clear();
  for (int y = 0; y < numRows; y++) {
    for (int x = 0; x < numCols; x++) {
      mx.setPoint(y, x, gameArea[y][x]);
    }
  }
  // Display the score on the last matrix
  display.setTextAlignment(PA_RIGHT);
  display.print(score);
  display.displayAnimate();
}

void spawnPiece() {
  currentShape = random(0, 7);
  currentRotation = 0;
  currentX = numCols / 2 - 2;
  currentY = 0;
}

void fixPiece() {
  for (int i = 0; i < 4; i++) {
    int x = currentX + ((shapes[currentShape][currentRotation] >> (3 - i) * 4) & 0xF);
    int y = currentY + ((shapes[currentShape][currentRotation] >> (3 - i) * 4) & 0xF);
    gameArea[y][x] = 1;
  }
}

void clearLines() {
  for (int y = 0; y < numRows; y++) {
    bool fullLine = true;
    for (int x = 0; x < numCols; x++) {
      if (gameArea[y][x] == 0) {
        fullLine = false;
        break;
      }
    }
    if (fullLine) {
      for (int row = y; row > 0; row--) {
        for (int col = 0; col < numCols; col++) {
          gameArea[row][col] = gameArea[row - 1][col];
        }
      }
      for (int col = 0; col < numCols; col++) {
        gameArea[0][col] = 0;
      }
      score += 10;
    }
  }
}

bool movePiece(int dx, int dy) {
  if (isValidMove(currentShape, currentRotation, currentX + dx, currentY + dy)) {
    currentX += dx;
    currentY += dy;
    return true;
  }
  return false;
}

void rotatePiece() {
  int newRotation = (currentRotation + 1) % 4;
  if (isValidMove(currentShape, newRotation, currentX, currentY)) {
    currentRotation = newRotation;
  }
}

bool isValidMove(int shape, int rotation, int x, int y) {
  for (int i = 0; i < 4; i++) {
    int newX = x + ((shapes[shape][rotation] >> (3 - i) * 4) & 0xF);
    int newY = y + ((shapes[shape][rotation] >> (3 - i) * 4) & 0xF);
    if (newX < 0 || newX >= numCols || newY < 0 || newY >= numRows || gameArea[newY][newX] != 0) {
      return false;
    }
  }
  return true;
}