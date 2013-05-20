#ifndef Display_h_
#define Display_h_

#include "Arduino.h"

typedef struct { byte g; byte r; byte b; } CRGB;

class Display {
  public:
    static const int NUM_SECTIONS = 2;  // Number of sections in the display. A section is a 2-dimensional subsection of the display driven by its own LED strip. All LED strips are equally spaced around the axis, and so all sections have the same area.
    static const int SECTION_WIDTH = 60;  // Width of each section.
    static const int SECTION_HEIGHT = 60;  // Height of each section. (= number of leds on each strip)
    
    static const int WIDTH = SECTION_WIDTH * NUM_SECTIONS;  // Width of the entire display.
    static const int HEIGHT = SECTION_HEIGHT;  // Height of the entire display.
    
    static const int NUM_LEDS = SECTION_HEIGHT * NUM_SECTIONS;
    
    Display();
    
    void displayLine(int displayX);
    void displayBlankLine();
    
    void setGlobePosition(int x);
    
    // Reticle
    void clearReticle();
    void setReticle(int x, int y);
    
    // Targets
    void clearAllTargets();
    void clearTarget(int id);
    void setTarget(int id, int x, int y);
    
    // Animation
    void clearAnimation();
    void playAbductionAnimation(int x, int y);
    void playScannerAnimation(int x, int y);
    void advanceAnimation();
};

#endif
