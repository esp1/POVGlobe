#include <OctoWS2811.h>

#include "Display.h"
#include "Bitmap.h"
#include "Debug.h"

const int DISPLAY_MEMORY_SIZE = Display::SECTOR_HEIGHT * 6;
DMAMEM int displayMemory[DISPLAY_MEMORY_SIZE];
int drawingMemory[DISPLAY_MEMORY_SIZE];

const int config = WS2811_800kHz;

OctoWS2811 leds(Display::SECTOR_HEIGHT, displayMemory, drawingMemory, config);

// Display ///////////////////////////////////////////////////////////

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF

Display::Display() {
  leds.begin();
  leds.show();
  clearAllTargets();
  
  int microsec = 2000000 / leds.numPixels();  // change them all in 2 seconds

  // uncomment for voltage controlled speed
  // millisec = analogRead(A9) / 40;

  colorWipe(RED, microsec);
  colorWipe(GREEN, microsec);
  colorWipe(BLUE, microsec);
  colorWipe(YELLOW, microsec);
  colorWipe(PINK, microsec);
  colorWipe(ORANGE, microsec);
  colorWipe(WHITE, microsec);
}

void Display::colorWipe(int color, int wait) {
  for (int i=0; i < Display::SECTOR_HEIGHT * Display::NUM_X_SECTORS; i++) {
    leds.setPixel(i, color);
    leds.show();
    delayMicroseconds(wait);
  }
}

// Position //////////////////////////////////////////////////////////

int bitmapXOffset = 0;

void Display::setGlobePosition(int bitmapX) {
  bitmapXOffset = bitmapX;
}

// Reticle ///////////////////////////////////////////////////////////

const int RETICLE_RADIUS = 2;
const int RETICLE_COLOR = 0xFF0000;  // Red
int reticleX = -1;
int reticleY = -1;

void Display::clearReticle() {
  reticleX = -1;
  reticleY = -1;
}

void Display::setReticle(int bitmapX, int bitmapY) {
  reticleX = bitmapX;
  reticleY = bitmapY;
}

boolean isReticlePixel(int bitmapX, int bitmapY) {
  if (reticleX < 0 || reticleY < 0) return false;

  // Square with side length = RETICLE_RADIUS * 2
  const int diffX = abs(bitmapX - reticleX);
  const int diffY = abs(bitmapY - reticleY);
  return
    diffX == RETICLE_RADIUS && diffY <= RETICLE_RADIUS ||
    diffY == RETICLE_RADIUS && diffX <= RETICLE_RADIUS;
}

// Targets ///////////////////////////////////////////////////////////

const int TARGET_COLOR = 0x00FF00;  // Green
// Targets are stored in an array where the array index is the target id and the value is an array that holds the target's x, y location
const int MAX_TARGETS = 4;  // Maximum number of targets
int numTargets = 0;
int targets[MAX_TARGETS][2];

void Display::clearAllTargets() {
  for (int i = 0; i < MAX_TARGETS; i++) {
    clearTarget(i);
  }
}

void Display::clearTarget(int id) {
  if (id >= 0 && id < MAX_TARGETS) {
    int* target = targets[id];
    if (target[0] >= 0 && target[1] >= 0) {
      setTarget(id, -1, -1);
      numTargets -= 1;
    }
  }
}

void Display::setTarget(int id, int bitmapX, int bitmapY) {
  if (id >= 0 && id < MAX_TARGETS) {
    int* target = targets[id];
    if (target[0] < 0 && target[1] < 0) {
      target[0] = bitmapX;
      target[1] = bitmapY;
      numTargets += 1;
    }
  }
}

boolean isTargetPixel(int bitmapX, int bitmapY) {
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (targets[i][0] == bitmapX &&
        targets[i][1] == bitmapY)
      return true;
  }
  
  return false;
}

// Animations ////////////////////////////////////////////////////////

int animationFrames = -1;  // Number of remaining animation frames. Or -1 if there is no animation.
int animationIncrement;  // How much to increment the radius by
int animationFPI;  // Frames per increment

int animationColor;
const int ABDUCTION_ANIMATION_COLOR = 0x00FF00;  // Green
const int SCANNER_ANIMATION_COLOR = 0x0000FF;  // Blue

// Cuz we're only animating circles
int animationRadius;

void Display::clearAnimation() {
  animationFrames = -1;
}

void Display::playAbductionAnimation(int bitmapX, int bitmapY) {
  animationColor = ABDUCTION_ANIMATION_COLOR;
  animationRadius = 0;
  animationIncrement = 2;
  animationFPI = 10;
  animationFrames = 100;
}

void Display::playScannerAnimation(int bitmapX, int bitmapY) {
  animationColor = SCANNER_ANIMATION_COLOR;
  animationRadius = 0;
  animationIncrement = 1;
  animationFPI = 3;
  animationFrames = 30;
}

void Display::advanceAnimation() {
  if (animationFrames >= 0) {
    if (animationFrames % animationFPI == 0) animationRadius += animationIncrement;
    animationFrames--;
    if (animationFrames < 0) clearAnimation();
  }
}

boolean isAnimationPixel(int bitmapX, int bitmapY) {
  if (animationFrames < 0) return false;
  
  // Circle
  const int diffX = abs(bitmapX - reticleX);
  const int diffY = abs(bitmapY - reticleY);
  return abs((diffX * diffX) + (diffY + diffY) - (animationRadius * animationRadius)) <= 1;
}

// Display ///////////////////////////////////////////////////////////

/**
 * Display data on all physical display strips.
 */
void Display::show(int originalBitmapX) {
  const int bitmapX = (bitmapXOffset + originalBitmapX) % BITMAP_WIDTH;
  const int bitmapSliceIndex = bitmapX % BITMAP_SECTOR_WIDTH;
  const int xSector = bitmapX / BITMAP_SECTOR_WIDTH;
  
  // Display all strips with this bitmap slice index
  const unsigned int *bitmap = BITMAP[bitmapSliceIndex];
  
  // rewrite strip data for appropriate output pin mapping
  for (int i = 0; i < DISPLAY_MEMORY_SIZE; i++) {
    const unsigned int x = bitmap[i];
    drawingMemory[i] = ((x & A_MASKS[xSector]) >> (Display::NUM_X_SECTORS - xSector)) |
                       ((x & B_MASKS[xSector]) << xSector);
  }
  
  // Overlays
  for (int bitmapY = 0; bitmapY < BITMAP_HEIGHT; bitmapY++) {
    int color = -1;
    
    if (isReticlePixel(bitmapX, bitmapY))  // Reticle
      color = RETICLE_COLOR;
    else if (isTargetPixel(bitmapX, bitmapY))  // Target
      color = TARGET_COLOR;
    else if (isAnimationPixel(bitmapX, bitmapY))  // Animation
      color = animationColor;
    
    if (color >= 0)
      leds.setPixel(bitmapX * Display::SECTOR_HEIGHT + bitmapY, color);
  }
  
  leds.show();
}


