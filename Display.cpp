#include "Display.h"
#include "FastSPI_LED2.h"
#include "Bitmap.h"
#include "Debug.h"

WS2811Controller800Mhz<6> ledStrip;

CGRB pixels[Display::NUM_LEDS];  // Assuming LED strips are wired in series
CGRB blankPixels[Display::NUM_LEDS];  // Assuming LED strips are wired in series

// Display ///////////////////////////////////////////////////////////

Display::Display() {
  ledStrip.init();
  clearAllTargets();
  
  memset(blankPixels, 0, Display::NUM_LEDS * sizeof(CGRB));

  // Cycle colors
  for(int i = 0; i < 3; i++) {
    for(int iLed = 0; iLed < NUM_LEDS; iLed++) {
      memset(pixels, 0,  Display::NUM_LEDS * sizeof(CGRB));
      switch(i) {
        case 0: pixels[iLed].r = 128; break;
        case 1: pixels[iLed].g = 128; break;
        case 2: pixels[iLed].b = 128; break;
      }
      ledStrip.showRGB((byte*)pixels, Display::NUM_LEDS);
    }
  }
  
  // Turn all lights on for 1 sec
  memset(pixels, 255, Display::NUM_LEDS * sizeof(CGRB));
  ledStrip.showRGB((byte*)pixels, Display::NUM_LEDS);
  delay(1000);
  
  // Then off
  ledStrip.showRGB((byte*)blankPixels, Display::NUM_LEDS);
}

// Position //////////////////////////////////////////////////////////

int bitmapXOffset = 0;

void Display::setGlobePosition(int x) {
  bitmapXOffset = x;
}

// Reticle ///////////////////////////////////////////////////////////

const int RETICLE_RADIUS = 2;
const CGRB RETICLE_COLOR = { 0, 255, 0 };  // Red
int reticleX = -1;
int reticleY = -1;

void Display::clearReticle() {
  reticleX = -1;
  reticleY = -1;
}

void Display::setReticle(int x, int y) {
  reticleX = x;
  reticleY = y;
}

boolean isReticlePixel(int x, int y) {
  if (reticleX < 0 || reticleY < 0) return false;

  // Square with side length = RETICLE_RADIUS * 2
  const int diffX = abs(x - reticleX);
  const int diffY = abs(y - reticleY);
  return
    diffX == RETICLE_RADIUS && diffY <= RETICLE_RADIUS ||
    diffY == RETICLE_RADIUS && diffX <= RETICLE_RADIUS;
}

// Targets ///////////////////////////////////////////////////////////

const CGRB TARGET_COLOR = { 0, 255, 0 };  // Green
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

void Display::setTarget(int id, int x, int y) {
  if (id >= 0 && id < MAX_TARGETS) {
    int* target = targets[id];
    if (target[0] < 0 && target[1] < 0) {
      target[0] = x;
      target[1] = y;
      numTargets += 1;
    }
  }
}

boolean isTargetPixel(int x, int y) {
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (targets[i][0] == x &&
        targets[i][1] == y)
      return true;
  }
  
  return false;
}

// Animations ////////////////////////////////////////////////////////

int animationFrames = -1;  // Number of remaining animation frames. Or -1 if there is no animation.
int animationIncrement;  // How much to increment the radius by
int animationFPI;  // Frames per increment

CGRB animationColor;
const CGRB ABDUCTION_ANIMATION_COLOR = { 255, 0, 0 };  // Green
const CGRB SCANNER_ANIMATION_COLOR = { 0, 0, 255 };  // Blue

// Cuz we're only animating circles
int animationRadius;

void Display::clearAnimation() {
  animationFrames = -1;
}

void Display::playAbductionAnimation(int x, int y) {
  animationColor = ABDUCTION_ANIMATION_COLOR;
  animationRadius = 0;
  animationIncrement = 2;
  animationFPI = 10;
  animationFrames = 100;
}

void Display::playScannerAnimation(int x, int y) {
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

boolean isAnimationPixel(int x, int y) {
  if (animationFrames < 0) return false;
  
  // Circle
  const int diffX = abs(x - reticleX);
  const int diffY = abs(y - reticleY);
  return abs((diffX * diffX) + (diffY + diffY) - (animationRadius * animationRadius)) <= 1;
}

// Display ///////////////////////////////////////////////////////////

void Display::displayLine(int bitmapSectionX) {
  for (int section = 0; section < Display::NUM_SECTIONS; section++) {
    const int sectionOffset = section * BITMAP_SECTION_WIDTH;
    const int bitmapX = (bitmapXOffset + sectionOffset + bitmapSectionX) % BITMAP_WIDTH;
    
    const int adjustedSection = bitmapX / BITMAP_SECTION_WIDTH;
    const int adjustedBitmapSectionX = bitmapX % BITMAP_SECTION_WIDTH;
    
    // Bitmap
    memcpy(&pixels[section * Display::HEIGHT], BITMAP_GRB[adjustedSection][adjustedBitmapSectionX], Display::HEIGHT * sizeof(CGRB));
    
    // Overlays
//    for (int bitmapY = 0; bitmapY < Display::HEIGHT; bitmapY++) {
//      if (isReticlePixel(bitmapX, bitmapY))  // Reticle
//        pixels[bitmapY] = RETICLE_COLOR;
//      else if (isTargetPixel(bitmapX, bitmapY))  // Target
//        pixels[bitmapY] = TARGET_COLOR;
//      else if (isAnimationPixel(bitmapX, bitmapY))  // Animation
//        pixels[bitmapY] = animationColor;
//    }
  }
  
  ledStrip.showRGB((byte*)pixels, Display::NUM_LEDS);
}

void Display::displayBlankLine() {
  ledStrip.showRGB((byte*)blankPixels, Display::NUM_LEDS);
}

