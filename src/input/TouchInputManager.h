#ifndef TOUCH_INPUT_MANAGER_H
#define TOUCH_INPUT_MANAGER_H

#include <XPT2046_Touchscreen.h>
#include "../config.h"
#include "../events/Event.h"

class TouchInputManager {
private:
  XPT2046_Touchscreen* touchscreen;
  bool lastTouchState = false;
  unsigned long lastTouchTime = 0;
  const unsigned long DEBOUNCE_MS = 150;
  
  // Valid raw value range (filter out garbage)
  const int RAW_MIN = 100;
  const int RAW_MAX = 4000;
  
  // Calibration - adjust these after seeing valid raw values!
  const int RAW_X_MIN = 200;
  const int RAW_X_MAX = 3800;
  const int RAW_Y_MIN = 200;
  const int RAW_Y_MAX = 3800;
  
  // Axis options - aangepast voor correcte mapping
  const bool INVERT_X = false;   // Geen X inversie
  const bool INVERT_Y = false;   // Geen Y inversie
  const bool SWAP_XY = false;    // Geen swap meer
  
  // Swipe detection
  bool swipeInProgress = false;
  int swipeStartX = 0;
  int swipeStartY = 0;
  unsigned long swipeStartTime = 0;
  const int SWIPE_MIN_DISTANCE = 50;      // Minimum pixels voor swipe
  const unsigned long SWIPE_MAX_TIME = 500; // Max tijd voor swipe in ms

  int mapTouch(int rawX, int rawY, int& outX, int& outY) {
    // Swap if needed (touch panel is rotated 90 degrees)
    if (SWAP_XY) {
      int temp = rawX;
      rawX = rawY;
      rawY = temp;
    }
    
    // Map to screen coordinates
    outX = map(rawX, RAW_X_MIN, RAW_X_MAX, 0, PORTRAIT_WIDTH);
    outY = map(rawY, RAW_Y_MIN, RAW_Y_MAX, 0, PORTRAIT_HEIGHT);
    
    // Invert if needed
    if (INVERT_X) outX = PORTRAIT_WIDTH - 1 - outX;
    if (INVERT_Y) outY = PORTRAIT_HEIGHT - 1 - outY;
    
    // Constrain
    outX = constrain(outX, 0, PORTRAIT_WIDTH - 1);
    outY = constrain(outY, 0, PORTRAIT_HEIGHT - 1);
    
    return 0;
  }

public:
  TouchInputManager(XPT2046_Touchscreen* ts) : touchscreen(ts) {}

  void update() {
    bool isTouched = touchscreen->touched();

    // Touch started
    if (isTouched && !lastTouchState) {
      TS_Point p = touchscreen->getPoint();
      
      // Filter invalid values
      if (p.x < RAW_MIN || p.x > RAW_MAX || p.y < RAW_MIN || p.y > RAW_MAX) {
        lastTouchState = isTouched;
        return;
      }
      
      int x, y;
      mapTouch(p.x, p.y, x, y);
      
      // Start tracking for potential swipe
      swipeInProgress = true;
      swipeStartX = x;
      swipeStartY = y;
      swipeStartTime = millis();
      
      Serial.printf("Touch start: x=%d, y=%d\n", x, y);
    }
    
    // Touch ended - check for swipe or tap
    if (!isTouched && lastTouchState) {
      if (swipeInProgress) {
        // Get final position
        TS_Point p = touchscreen->getPoint();
        int endX = swipeStartX;  // Default to start if can't read
        int endY = swipeStartY;
        
        // Calculate elapsed time
        unsigned long elapsed = millis() - swipeStartTime;
        
        // Try to get last known position from touchscreen
        // Since touch ended, we use the start position for comparison
        // The actual swipe detection happens during touch
        
        // For now, we'll detect swipe during continuous touch below
        swipeInProgress = false;
        
        // If it was a quick tap (not a swipe), dispatch tap event
        if (elapsed < SWIPE_MAX_TIME && millis() - lastTouchTime >= DEBOUNCE_MS) {
          Event event;
          event.type = EventType::TOUCH_PRESSED;
          event.param1 = swipeStartX;
          event.param2 = swipeStartY;
          EventBus::getInstance().dispatch(event);
          lastTouchTime = millis();
          Serial.printf("Tap at: x=%d, y=%d\n", swipeStartX, swipeStartY);
        }
      }
    }
    
    // Touch is ongoing - check for swipe
    if (isTouched && swipeInProgress) {
      TS_Point p = touchscreen->getPoint();
      
      // Filter invalid values
      if (p.x >= RAW_MIN && p.x <= RAW_MAX && p.y >= RAW_MIN && p.y <= RAW_MAX) {
        int currentX, currentY;
        mapTouch(p.x, p.y, currentX, currentY);
        
        int deltaX = currentX - swipeStartX;
        int deltaY = currentY - swipeStartY;
        unsigned long elapsed = millis() - swipeStartTime;
        
        // Check if this is a horizontal swipe
        if (elapsed < SWIPE_MAX_TIME && abs(deltaX) > SWIPE_MIN_DISTANCE && abs(deltaX) > abs(deltaY) * 2) {
          Event event;
          
          if (deltaX < 0) {
            // Swipe left -> volgende pagina
            event.type = EventType::SWIPE_LEFT;
            Serial.printf("Swipe LEFT detected: deltaX=%d\n", deltaX);
          } else {
            // Swipe right -> vorige pagina
            event.type = EventType::SWIPE_RIGHT;
            Serial.printf("Swipe RIGHT detected: deltaX=%d\n", deltaX);
          }
          
          EventBus::getInstance().dispatch(event);
          swipeInProgress = false;  // Prevent multiple swipe events
          lastTouchTime = millis();
        }
      }
    }

    lastTouchState = isTouched;
  }

  bool isTouched() const {
    return lastTouchState;
  }
};

#endif