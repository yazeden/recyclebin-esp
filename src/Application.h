#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include "events/Event.h"
#include "states/StateManager.h"
#include "display/DisplayManager.h"
#include "input/TouchInputManager.h"
#include "services/LEDAnimationService.h"
#include "services/SleepModeService.h"
#include "services/InteractionService.h"
#include "services/DatabaseService.h"
#include "models/ItemRepository.h"

class Application {
private:
  std::unique_ptr<DisplayManager> display;
  std::unique_ptr<TouchInputManager> touchInput;
  std::unique_ptr<LEDAnimationService> ledAnimation;
  std::unique_ptr<StateManager> stateManager;

  SleepModeService& sleepModeService = 
    SleepModeService::getInstance();
  InteractionService& interactionService = 
    InteractionService::getInstance();
  ItemRepository& itemRepository = 
    ItemRepository::getInstance();
  DatabaseService& databaseService = 
    DatabaseService::getInstance();

public:
  Application(TFT_eSPI* tft, XPT2046_Touchscreen* touch, 
              CRGB* plastic, CRGB* paper, CRGB* green, CRGB* waste) {
    display = std::make_unique<DisplayManager>(tft);
    touchInput = std::make_unique<TouchInputManager>(touch);
    ledAnimation = std::make_unique<LEDAnimationService>(plastic, paper, green, waste);
  }

  void init() {
    Serial.println("Initializing Application...");
    
    // Connect to WiFi first
    Serial.println("Connecting to WiFi...");
    display->init();  // Init display early to show status
    display->showMessage("Connecting WiFi...");
    
    bool wifiConnected = databaseService.connectWiFi(15000);  // 15 sec timeout
    
    if (wifiConnected) {
      display->showMessage("WiFi OK!\nLoading data...");
      
      // Process any pending posts from previous session
      int processed = databaseService.processPendingPosts();
      if (processed > 0) {
        Serial.printf("Processed %d pending posts\n", processed);
      }
    } else {
      display->showMessage("WiFi failed\nUsing cached data");
      delay(1500);
    }
    
    // Load items (database -> JSON -> hardcoded)
    itemRepository.loadHardcodedItems();
    Serial.printf("Loaded %d items (source: %s)\n", 
                  itemRepository.getItemCount(),
                  itemRepository.getDataSource().c_str());
    
    // Show data source status briefly
    if (itemRepository.isUsingCachedData()) {
      display->showMessage("Using cached data");
      delay(1000);
    }
    
    // Initialize LED animation
    ledAnimation->init();

    // NOW create StateManager (after items are loaded!)
    stateManager = std::make_unique<StateManager>(
      display.get(), 
      ledAnimation.get()
    );

    sleepModeService.setSleepTimeout(10000);  // 10 seconds

    EventBus::getInstance().subscribe(
      EventType::ITEM_SELECTED,
      [this](const Event& e) { onItemSelected(e); }
    );

    EventBus::getInstance().subscribe(
      EventType::TOUCH_PRESSED,
      [this](const Event& e) { onTouchPressed(e); }
    );

    EventBus::getInstance().subscribe(
      EventType::SCREEN_CHANGED,
      [this](const Event& e) { onScreenChanged(e); }
    );

    Serial.println("Application ready!");
  }

  void update() {
    touchInput->update();
    sleepModeService.update();
    ledAnimation->update();  // Update LED animations!
    stateManager->update();
  }

  void render() {
    stateManager->render();
  }

private:
  void onTouchPressed(const Event& event) {
    Serial.printf("Touch: x=%d, y=%d (screen=%d)\n", 
                  event.param1, event.param2, 
                  (int)stateManager->getCurrentScreenType());
    sleepModeService.recordActivity();
    
    // If we're in sleep mode, wake up and go to home screen
    if (stateManager->getCurrentScreenType() == ScreenType::SLEEP) {
      Serial.println("Waking up from sleep mode!");
      stateManager->goToHomeScreen();
    } else {
      // Forward touch event to current state (HomeScreen)
      stateManager->handleEvent(event);
    }
  }

  void onItemSelected(const Event& event) {
    int itemId = event.param1;
    bool isDirty = (event.param2 == 1);  // Haal isDirty uit event
    Item* item = itemRepository.getItemById(itemId);
    
    if (item) {
      Serial.printf("Item selected: %s (category: %d, dirty: %d)\n", 
                    item->name.c_str(), (int)item->category, isDirty);
      
      // Update item dirty status
      item->isDirty = isDirty;
      
      // Post selection to database (async, queued if offline)
      itemRepository.postItemSelection(DEVICE_LOCATION, *item);
      
      // Determine color based on dirty status
      ItemCategory displayCategory = item->category;
      uint16_t displayColor = item->color;
      
      // If dirty, show waste strip instead
      if (isDirty) {
        displayCategory = ItemCategory::WASTE;
        displayColor = COLOR_WASTE;
        Serial.println("Item is dirty -> showing WASTE strip");
      }
      
      // Convert 16-bit color to CRGB
      CRGB color = CRGB(
        (displayColor >> 11) * 8,
        ((displayColor >> 5) & 0x3F) * 4,
        (displayColor & 0x1F) * 8
      );
      
      // Start BREATHING animation for 5 seconds on the correct strip
      ledAnimation->startAnimationForCategory(
        displayCategory,
        LEDAnimationType::BREATHING,
        color,
        2000  // 2 second breathing cycle
      );

      interactionService.recordInteraction(itemId);
      sleepModeService.recordActivity();
    }
  }

  void onScreenChanged(const Event& event) {
    if (event.param1 == 1) {
      stateManager->goToSleepMode();
    } else if (event.param1 == 2) {
      // LEDs uit (terugkeer naar grid)
      ledAnimation->stop();  // stop() zet isAnimating op false EN doet allOff()
      Serial.println("LEDs turned off (returned to grid)");
    } else {
      stateManager->goToHomeScreen();
    }
  }
};

#endif