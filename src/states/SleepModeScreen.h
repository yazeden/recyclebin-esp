#ifndef SLEEP_MODE_SCREEN_H
#define SLEEP_MODE_SCREEN_H

#include "ScreenState.h"
#include "../display/DisplayManager.h"
#include "../services/LEDAnimationService.h"
#include "../models/Item.h"
#include "../models/ItemRepository.h"
#include <vector>

class SleepModeScreen : public ScreenState {
private:
  DisplayManager* display;
  LEDAnimationService* ledAnimation;
  std::vector<Item> allItems;
  int currentItemIndex = 0;
  unsigned long lastItemChangeTime = 0;
  const unsigned long ITEM_DISPLAY_TIME = 5000;  // 5 seconds
  bool needsRedraw = true;

  // Large circle in center
  const int CIRCLE_RADIUS = 70;
  const int CIRCLE_BORDER_THICKNESS = 8;

  void loadItems() {
    ItemRepository& repo = ItemRepository::getInstance();
    allItems = repo.getAllItems();
    Serial.printf("SleepModeScreen: Loaded %d items\n", allItems.size());
  }

  void drawCurrentItem() {
    if (allItems.empty()) return;

    TFT_eSPI* tft = display->getTFT();
    const Item& item = allItems[currentItemIndex];

    // Clear screen to black
    tft->fillScreen(TFT_BLACK);

    // Center of screen in PORTRAIT mode (240 wide x 320 tall)
    int centerX = PORTRAIT_WIDTH / 2;   // 120
    int centerY = PORTRAIT_HEIGHT / 2 - 30;  // Slightly above center for "tap for more"

    // Draw thick colored border ring
    for (int i = 0; i < CIRCLE_BORDER_THICKNESS; i++) {
      tft->drawCircle(centerX, centerY, CIRCLE_RADIUS - i, item.color);
    }

    // Draw item name in center (white text)
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    
    // Handle long names - wrap text inside circle
    String name = item.name;
    int maxWidth = CIRCLE_RADIUS * 2 - 20;  // Max width inside circle
    
    if (name.length() <= 8) {
      // Short name - use large font
      tft->drawString(name.c_str(), centerX, centerY, 4);
    } else if (name.length() <= 14) {
      // Medium name - use smaller font
      tft->drawString(name.c_str(), centerX, centerY, 2);
    } else {
      // Long name - split into multiple lines
      // Try to split at space, comma, or parenthesis
      int splitIdx = -1;
      for (int i = name.length() / 2; i >= 0; i--) {
        char c = name.charAt(i);
        if (c == ' ' || c == ',' || c == '(') {
          splitIdx = i;
          break;
        }
      }
      // Also check second half
      if (splitIdx < 0) {
        for (int i = name.length() / 2; i < (int)name.length(); i++) {
          char c = name.charAt(i);
          if (c == ' ' || c == ',' || c == '(') {
            splitIdx = i;
            break;
          }
        }
      }
      
      if (splitIdx > 0) {
        String line1 = name.substring(0, splitIdx);
        String line2 = name.substring(splitIdx + 1);
        line1.trim();
        line2.trim();
        tft->drawString(line1.c_str(), centerX, centerY - 10, 2);
        tft->drawString(line2.c_str(), centerX, centerY + 10, 2);
      } else {
        // No good split point - just use small font
        tft->drawString(name.c_str(), centerX, centerY, 1);
      }
    }

    // Draw "Tap for more" at bottom
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Tap for more", centerX, PORTRAIT_HEIGHT - 40, 2);
    
    tft->setTextDatum(TL_DATUM);

    Serial.printf("Drew item: %s (index %d)\n", item.name.c_str(), currentItemIndex);
  }

  void updateLEDsForCurrentItem() {
    if (allItems.empty()) return;

    const Item& item = allItems[currentItemIndex];

    // Extract RGB from 16-bit color (RGB565)
    uint8_t r = (item.color >> 11) << 3;           // 5 bits -> 8 bits
    uint8_t g = ((item.color >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits
    uint8_t b = (item.color & 0x1F) << 3;          // 5 bits -> 8 bits

    CRGB color = CRGB(r, g, b);

    // Breathe on the correct strip for this item's category
    ledAnimation->breatheForCategory(
      item.category,
      color,
      2000  // 2 second breathing cycle
    );
    
    Serial.printf("Breathing LEDs for category: %d\n", (int)item.category);
  }

public:
  SleepModeScreen(DisplayManager* d, LEDAnimationService* led)
    : display(d), ledAnimation(led) {}

  ScreenType getType() const override {
    return ScreenType::SLEEP;
  }

  void onEnter() override {
    Serial.println("SleepModeScreen: Entering sleep mode");
    
    loadItems();
    currentItemIndex = 0;
    lastItemChangeTime = millis();
    needsRedraw = true;

    // Set rotation to portrait (quarter turn)
    display->getTFT()->setRotation(0);

    updateLEDsForCurrentItem();
  }

  void onExit() override {
    Serial.println("SleepModeScreen: Exiting sleep mode");
    ledAnimation->stop();
    
    // Reset rotation back to landscape
    display->getTFT()->setRotation(1);
  }

  void handleEvent(const Event& event) override {
    if (event.type == EventType::TOUCH_PRESSED) {
      Serial.println("Touch detected - waking up!");
    }
  }

  void update() override {
    ledAnimation->update();

    // Check if it's time to switch to next item
    if (millis() - lastItemChangeTime >= ITEM_DISPLAY_TIME) {
      lastItemChangeTime = millis();
      
      // Move to next item (cycle back to 0 if at end)
      if (!allItems.empty()) {
        currentItemIndex = (currentItemIndex + 1) % allItems.size();
        needsRedraw = true;
        updateLEDsForCurrentItem();
      }
    }
  }

  void render() override {
    if (!needsRedraw) return;
    needsRedraw = false;

    if (allItems.empty()) {
      Serial.println("SleepModeScreen: No items to render!");
      TFT_eSPI* tft = display->getTFT();
      tft->fillScreen(TFT_BLACK);
      tft->setTextColor(TFT_WHITE, TFT_BLACK);
      tft->setTextDatum(MC_DATUM);
      tft->drawString("No items loaded", PORTRAIT_WIDTH/2, PORTRAIT_HEIGHT/2, 2);
      return;
    }

    drawCurrentItem();
  }
};

#endif