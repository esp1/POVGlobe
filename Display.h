#ifndef Display_h_
#define Display_h_

#include "Arduino.h"

class Display {
  public:
    static const int NUM_X_SECTORS = 2;
    static const int NUM_Y_SECTORS = 1;
    
    static const int SECTOR_WIDTH = 60;  // Width of each sector
    static const int SECTOR_HEIGHT = 60;  // Height of each sector
    
    static const int WIDTH = NUM_X_SECTORS * SECTOR_WIDTH;  // Width of the entire display
    static const int HEIGHT = NUM_Y_SECTORS * SECTOR_HEIGHT;  // Height of the entire display
    
    Display();
    void colorWipe(int color, int wait);
    
    void show(int bitmapX);
    
    void setGlobePosition(int bitmapX);
    
    // Reticle
    void clearReticle();
    void setReticle(int bitmapX, int bitmapY);
    
    // Targets
    void clearAllTargets();
    void clearTarget(int id);
    void setTarget(int id, int bitmapX, int bitmapY);
    
    // Animation
    void clearAnimation();
    void playAbductionAnimation(int bitmapX, int bitmapY);
    void playScannerAnimation(int bitmapX, int bitmapY);
    void advanceAnimation();
};

#endif
