#ifndef LED_ANIMATION_SERVICE_H
#define LED_ANIMATION_SERVICE_H

#include <FastLED.h>
#include "../config.h"
#include "../models/Item.h"

enum class LEDAnimationType {
  PULSE,
  FADE,
  RAINBOW,
  BREATHING,
  STROBE,
  SOLID
};

struct LEDAnimation {
  LEDAnimationType type;
  CRGB color;
  unsigned long duration;
  unsigned long startTime;
  int intensity;
};

class LEDAnimationService {
private:
  CRGB* stripPlastic;   // Strip 1
  CRGB* stripPaper;     // Strip 2
  CRGB* stripGreen;     // Strip 3
  CRGB* stripWaste;     // Strip 4
  
  LEDAnimation currentAnimation;
  bool isAnimating = false;
  ItemCategory activeCategory = ItemCategory::PLASTIC;

  // Get the strip for a category
  CRGB* getStripForCategory(ItemCategory cat) {
    switch (cat) {
      case ItemCategory::PLASTIC: return stripPlastic;
      case ItemCategory::PAPER:   return stripPaper;
      case ItemCategory::GREEN:   return stripGreen;
      case ItemCategory::WASTE:   return stripWaste;
      default:                    return stripPlastic;
    }
  }

  // Animate a single strip with breathing effect
  void animateBreathingStrip(CRGB* strip, CRGB color, float progress) {
    float sine = sin(progress * 2 * PI);
    int brightness = 128 + (128 * sine);
    
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      strip[i] = color;
      strip[i].nscale8(brightness);
    }
  }

  // Turn off a single strip
  void turnOffStrip(CRGB* strip) {
    fill_solid(strip, NUM_LEDS_PER_STRIP, CRGB::Black);
  }

  // Set solid color on a strip
  void setSolidStrip(CRGB* strip, CRGB color, int brightness = 255) {
    for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      strip[i] = color;
      strip[i].nscale8(brightness);
    }
  }

public:
  LEDAnimationService(CRGB* plastic, CRGB* paper, CRGB* green, CRGB* waste) 
    : stripPlastic(plastic), stripPaper(paper), stripGreen(green), stripWaste(waste) {
    currentAnimation = {
      LEDAnimationType::BREATHING,
      CRGB::Blue,
      2000,
      0,
      255
    };
  }

  void init() {
    // LEDs are already initialized in main.cpp
    allOff();
    Serial.println("LEDAnimationService initialized with 4 strips");
  }

  // Start animation on a specific category's strip
  void startAnimationForCategory(ItemCategory category, LEDAnimationType type, 
                                  CRGB color, unsigned long duration) {
    activeCategory = category;
    currentAnimation = {
      type,
      color,
      duration,
      millis(),
      255
    };
    isAnimating = true;
    
    // Turn off all other strips
    allOff();
    
    Serial.printf("Starting animation for category %d\n", (int)category);
  }

  // Convenience method - start animation with auto color from category
  void startAnimation(LEDAnimationType type, CRGB color, unsigned long duration) {
    currentAnimation = {
      type,
      color,
      duration,
      millis(),
      255
    };
    isAnimating = true;
  }

  // Light up the correct strip based on item category
  void showCategoryStrip(ItemCategory category, CRGB color) {
    allOff();
    activeCategory = category;
    
    CRGB* strip = getStripForCategory(category);
    setSolidStrip(strip, color, 255);
    FastLED.show();
    
    Serial.printf("Showing strip for category: %d\n", (int)category);
  }

  // Breathing animation for current item's category
  void breatheForCategory(ItemCategory category, CRGB color, unsigned long duration) {
    activeCategory = category;
    currentAnimation = {
      LEDAnimationType::BREATHING,
      color,
      duration,
      millis(),
      255
    };
    isAnimating = true;
  }

  void update() {
    if (!isAnimating) return;

    unsigned long elapsed = millis() - currentAnimation.startTime;
    float progress = fmod((float)elapsed / currentAnimation.duration, 1.0);

    // First turn off all strips
    turnOffStrip(stripPlastic);
    turnOffStrip(stripPaper);
    turnOffStrip(stripGreen);
    turnOffStrip(stripWaste);

    // Animate only the active category's strip
    CRGB* activeStrip = getStripForCategory(activeCategory);

    switch (currentAnimation.type) {
      case LEDAnimationType::BREATHING:
        animateBreathingStrip(activeStrip, currentAnimation.color, progress);
        break;
      case LEDAnimationType::PULSE: {
        int brightness = 255 * progress;
        setSolidStrip(activeStrip, currentAnimation.color, brightness);
        break;
      }
      case LEDAnimationType::FADE: {
        int brightness = 255 * (1.0 - progress);
        setSolidStrip(activeStrip, currentAnimation.color, brightness);
        break;
      }
      case LEDAnimationType::SOLID:
        setSolidStrip(activeStrip, currentAnimation.color, 255);
        break;
      case LEDAnimationType::STROBE:
        if (fmod(progress * 10, 1.0) < 0.5) {
          setSolidStrip(activeStrip, currentAnimation.color, 255);
        }
        break;
      case LEDAnimationType::RAINBOW:
        for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
          uint8_t hue = (progress * 255) + (i * 255 / NUM_LEDS_PER_STRIP);
          activeStrip[i] = CHSV(hue, 255, 255);
        }
        break;
    }

    FastLED.show();
  }

  void stop() {
    isAnimating = false;
    allOff();
  }

  void allOff() {
    turnOffStrip(stripPlastic);
    turnOffStrip(stripPaper);
    turnOffStrip(stripGreen);
    turnOffStrip(stripWaste);
    FastLED.show();
  }

  // Test all strips - useful for debugging
  void testAllStrips() {
    Serial.println("Testing all LED strips...");
    
    // Plastic - Orange
    setSolidStrip(stripPlastic, CRGB::Orange, 255);
    FastLED.show();
    delay(500);
    turnOffStrip(stripPlastic);
    
    // Paper - Blue
    setSolidStrip(stripPaper, CRGB::Blue, 255);
    FastLED.show();
    delay(500);
    turnOffStrip(stripPaper);
    
    // Green - Green
    setSolidStrip(stripGreen, CRGB::Green, 255);
    FastLED.show();
    delay(500);
    turnOffStrip(stripGreen);
    
    // Waste - White/Grey
    setSolidStrip(stripWaste, CRGB::White, 128);
    FastLED.show();
    delay(500);
    turnOffStrip(stripWaste);
    
    FastLED.show();
    Serial.println("LED strip test complete");
  }

  bool isRunning() const {
    return isAnimating;
  }
  
  ItemCategory getActiveCategory() const {
    return activeCategory;
  }
};

#endif