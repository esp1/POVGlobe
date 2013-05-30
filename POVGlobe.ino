/*
The POV display is made up of a 1-dimensional led light strip that is rotated at high speed to make a 2-dimensional
display.

Terms:
Display - The entire globe display. The display is a rectangular surface that may be subdivided into sectors.
Sector - A rectangular subsection of the display surface. All of the sectors in the display have the same dimensions and are tiled horizontally and/or vertically to cover the display surface.
Slice - A 1-dimensional vertical slice of a 2-dimensional sector.

Setup:
Teensy, hall effect sensor, and light strips are all mounted on rotating part of globe.
Power for all of the above are drawn from slip rings on the globe axis.

SS460S latching hall effect sensor mounted on rotating part of globe near axis.
2 magnets with opposing polarities mounted on fixed base of globe near axis.
Pin 0 - 5V
Pin 1 - GND
Pin 2 - input to microcontroller (interrupt pin)

There are two LED strips mounted on opposite sides of the globe axis from each other, but wired together in series.
During one rotation, the code will display half of the desired image on each of the strips.
The display height is the height of one of the LED strips, but because the strips are wired together in series, the image data sent to the strips
has a height = the height of both LED strips.
Note that the strips should be oriented so that the input ends of both strips are at the top of the display. This is because on the back half of the
rotation the first strip should be showing the same data that was on the second strip during the first half of the rotation, so the image data should have the
same orientation for both strips.

Image data should match configuration of LED strip (in our case GRB).
*/

#include <OctoWS2811.h>

#include "Bitmap.h"
#include "Display.h"
#include "Protocol.h"
#include "Debug.h"

// Status

#define STATUS_PIN 13
int status;

// Display

Display* display;

// Display Timing

#define ROTATION_SENSOR_PIN 17
boolean interrupt = false;

unsigned long displayIntervalMicros = 500000;  // Interval in microseconds to display
unsigned long previousInterruptMicros = micros();  // Previous interrupt time in microseconds

// Setup /////////////////////////////////////////////////////////////

void setup() {
  // Status led pin
  pinMode(STATUS_PIN, OUTPUT);

  // Instantiate display
  display = new Display();

  // Set up serial command channel
  Serial1.begin(115200);
  
  // Set interrupt for rotation sensor
  pinMode(ROTATION_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(ROTATION_SENSOR_PIN, calculateDisplayTiming, FALLING);
  
#ifdef DEBUG
  // Open serial communications for debug
  Serial.begin(57600);
  delay(1000);
  DPRINTLN("POV Globe");
#endif
}

// Loop //////////////////////////////////////////////////////////////

void loop() {
  // Reset indicators
  status = HIGH;
  interrupt = false;
  
  // Display
  for (int bitmapX = 0; bitmapX < BITMAP_WIDTH; bitmapX++) {
    // If an interrupt was received it signals the start of a new display frame, so end this display loop and start over
    if (interrupt) break;
    
    // Mark expected end of interval for displaying this line
    const unsigned long intervalEndMicros = previousInterruptMicros + ((bitmapX + 1) * displayIntervalMicros);
    
    processSerial();
    display->displayStrips(bitmapX);
    
    // Calculate additional delay to end of interval
    const unsigned long now = micros();
    if (intervalEndMicros > now) {
      // Delay to end of interval
      delayMicroseconds(intervalEndMicros - now);
    } else {
      // Error condition: iteration has taken longer than the allotted interval
      status = LOW;
      DPRINT("Display interval ");
      DPRINT(displayIntervalMicros);
      DPRINT(" expected end ");
      DPRINT(intervalEndMicros);
      DPRINT(" overrun by ");
      DPRINT(now - intervalEndMicros);
      DPRINTLN(" microseconds");
    }
  }
  // Advance animation
  display->advanceAnimation();
  
  // Show status
  digitalWrite(STATUS_PIN, status);
}

// Display Timing ////////////////////////////////////////////////////

void calculateDisplayTiming() {
  interrupt = true;
  
  const unsigned long nowMicros = micros();
  displayIntervalMicros = (nowMicros - previousInterruptMicros) / (BITMAP_WIDTH);
  previousInterruptMicros = nowMicros;
}

// Serial Commands ///////////////////////////////////////////////////

boolean packetStart = false;
int opCode = -1;
int param0 = -1;
int param1 = -1;

void endPacket() {
  // Send acknowledgement
  Serial1.write(0xFF);
  
  // Reset packet variables
  packetStart = false;
  opCode = -1;
  param0 = -1;
  param1 = -1;
}

void processSerial() {
  if (Serial1.available()) {
    // Read packet
    const byte b = Serial1.read();
    
    if (!packetStart) {
      if (b == 0xFF) {  // Packet start delimiter
        DPRINTLN("packet start");
        packetStart = true;
      } else {
        // Ignore
      }
      return;
    } else if (opCode < 0) {  // Read opCode; process commands with no parameters
      opCode = b;
      
      DPRINT("opCode: ");
      DPRINTLN(b);
      
      switch (opCode) {
        // ResetGlobe() - clears everything from the globe and resets its position.
        case CMD_RESET_GLOBE:
          DPRINTLN("Reset Globe");
          display->setGlobePosition(0);
          display->clearReticle();
          display->clearAllTargets();
          display->clearAnimation();
          endPacket();
          break;
          
        // ClearReticle() - removes the reticle.
        case CMD_CLEAR_RETICLE:
          DPRINTLN("Clear Reticle");
          display->clearReticle();
          endPacket();
          break;
        
        // ClearAnimation() - clears the current animation.
        case CMD_CLEAR_ANIMATION:
          DPRINTLN("Clear Animation");
          display->clearAnimation();
          endPacket();
          break;
      }
      
      return;
    } else switch (opCode) {  // Process commands with 1 or more parameters
      // SetGlobePosition(xValue) - set the globe X columns counterclockwise from the world origin.
      case CMD_SET_GLOBE_POSITION:
        DPRINTLN("Set Globe Position");
        display->setGlobePosition(b);
        endPacket();
        break;
        
      // SetReticle(xValue, yValue) - puts the reticle (crosshairs or indicator dot) on the given coords relative to globe origin.
      case CMD_SET_RETICLE:
        DPRINTLN("Set Reticle");
        if (param0 < 0) param0 = b;
        else {
          display->setReticle(param0, b);
          endPacket();
        }
        break;
        
      // ClearTarget(id) - removes the target marker.
      case CMD_CLEAR_TARGET:
        DPRINTLN("Clear Target");
        display->clearTarget(b);
        endPacket();
        break;
        
      // SetTarget(id, xValue, yValue) - add a target marker (animal) to the given coordinates relative to the globe origin.
      case CMD_SET_TARGET:
        DPRINTLN("Set Target");
        if (param0 < 0) param0 = b;
        else if (param1 < 0) param1 = b;
        else {
          display->setTarget(param0, param1, b);
          endPacket();
        }
        break;
        
      // PlayAbductionAnimation(x, y) - plays the abduction animation (green dot grows to become green circle and fades out). Cancel other animations if you want.
      case CMD_PLAY_ABDUCTION_ANIMATION:
        DPRINTLN("Play Abduction Animation");
        if (param0 < 0) param0 = b;
        else {
          display->playAbductionAnimation(param0, b);
          endPacket();
        }
        break;
        
      // PlayScannerAnimation(x, y) - play the scanner animation (blue circle expands from point, or big blue circle flashes momentarily). Cancel other animations if you want.
      case CMD_PLAY_SCANNER_ANIMATION:
        DPRINTLN("Play Scanner Animation");
        if (param0 < 0) param0 = b;
        else {
          display->playScannerAnimation(param0, b);
          endPacket();
        }
        break;
        
      case CMD_UPLOAD_IMAGE:
        DPRINTLN("Upload Image");
        
        endPacket();
        break;
        
      default:
        DPRINT("Unknown opCode: ");
        DPRINTLN(opCode);
        break;
    }
  }
}

