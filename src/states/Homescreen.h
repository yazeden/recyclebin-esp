#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "ScreenState.h"
#include "../display/DisplayManager.h"
#include "../models/Item.h"
#include "../models/ItemRepository.h"
#include "../input/TouchInputManager.h"
#include <vector>

enum class HomeScreenMode {
  GRID,
  DIRTY_POPUP,
  RESULT
};

class HomeScreen : public ScreenState {
private:
  DisplayManager* display;
  std::vector<Item> allItems;
  int scrollOffset = 0;
  int selectedItemIndex = -1;
  bool needsRedraw = true;
  
  HomeScreenMode mode = HomeScreenMode::GRID;
  int pendingItemIndex = -1;

  void loadAllItems() {
    ItemRepository& repo = ItemRepository::getInstance();
    allItems = repo.getAllItems();
    Serial.printf("Loaded %d items (sorted A-Z)\n", allItems.size());
  }

  // Simpele grid touch - welk item is aangeklikt?
  int getItemAtPosition(int x, int y) {
    // Check bounds - alleen content area
    if (y < HEADER_HEIGHT || y > PORTRAIT_HEIGHT - FOOTER_HEIGHT) {
      Serial.printf("Touch outside content: y=%d (header=%d, footer starts=%d)\n", 
                    y, HEADER_HEIGHT, PORTRAIT_HEIGHT - FOOTER_HEIGHT);
      return -1;
    }
    
    // Bereken grid dimensies - MOET matchen met DisplayManager!
    int contentHeight = PORTRAIT_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;  // 320-40-50 = 230
    int itemWidth = PORTRAIT_WIDTH / GRID_ITEM_COLS;   // 240/2 = 120
    int itemHeight = contentHeight / GRID_ITEM_ROWS;   // 230/2 = 115
    
    // Bereken welke cel is aangeraakt
    int touchY = y - HEADER_HEIGHT;  // Relatief aan content start
    int col = x / itemWidth;
    int row = touchY / itemHeight;
    
    // Clamp values
    if (col < 0) col = 0;
    if (col >= GRID_ITEM_COLS) col = GRID_ITEM_COLS - 1;
    if (row < 0) row = 0;
    if (row >= GRID_ITEM_ROWS) row = GRID_ITEM_ROWS - 1;
    
    // Bereken item index
    int gridIndex = row * GRID_ITEM_COLS + col;
    int actualIndex = scrollOffset + gridIndex;
    
    Serial.printf("Touch->Grid: x=%d,y=%d -> col=%d,row=%d -> grid=%d + offset=%d = actual=%d\n",
                  x, y, col, row, gridIndex, scrollOffset, actualIndex);
    
    // Check of dit item bestaat
    if (actualIndex >= 0 && actualIndex < (int)allItems.size()) {
      Serial.printf("Item found: %s\n", allItems[actualIndex].name.c_str());
      return actualIndex;
    }
    Serial.println("Index out of bounds!");
    return -1;
  }

  void handleGridClick(int x, int y) {
    int itemIndex = getItemAtPosition(x, y);
    
    if (itemIndex >= 0) {
      Item& item = allItems[itemIndex];
      selectedItemIndex = itemIndex;
      
      Serial.printf("Item clicked: %s (idx=%d, canBeDirty=%d)\n", 
                    item.name.c_str(), itemIndex, item.canBeDirty);
      
      if (item.canBeDirty) {
        pendingItemIndex = itemIndex;
        mode = HomeScreenMode::DIRTY_POPUP;
      } else {
        dispatchItemEvent(item, false);
        mode = HomeScreenMode::RESULT;
      }
      needsRedraw = true;
    }
  }

  void handlePopupClick(int x, int y) {
    if (pendingItemIndex < 0 || pendingItemIndex >= (int)allItems.size()) return;
    
    Serial.printf("Popup click: x=%d, y=%d\n", x, y);
    
    Item& item = allItems[pendingItemIndex];
    
    // Simpele verdeling: bovenste helft = schoon, onderste helft = vies
    // Header is 60px, dus content start bij y=60
    // Scherm is 320px hoog
    // Midden is rond y=160
    
    if (y >= 60 && y < 190) {
      // Bovenste helft - SCHOON
      Serial.println("SCHOON button pressed!");
      item.isDirty = false;
      dispatchItemEvent(item, false);
      mode = HomeScreenMode::RESULT;
      needsRedraw = true;
      return;
    }
    
    if (y >= 190) {
      // Onderste helft - VIES
      Serial.println("VIES button pressed!");
      item.isDirty = true;
      dispatchItemEvent(item, true);
      mode = HomeScreenMode::RESULT;
      needsRedraw = true;
      return;
    }
    
    Serial.println("Popup click in header area - ignored");
  }

  void dispatchItemEvent(const Item& item, bool isDirty) {
    Event event;
    event.type = EventType::ITEM_SELECTED;
    event.param1 = item.id;
    event.param2 = isDirty ? 1 : 0;  // Stuur isDirty mee!
    EventBus::getInstance().dispatch(event);
    Serial.printf("Event: item %d (%s), dirty=%d\n", item.id, item.name.c_str(), isDirty);
  }

  void returnToGrid() {
    mode = HomeScreenMode::GRID;
    selectedItemIndex = -1;
    pendingItemIndex = -1;
    needsRedraw = true;
    
    // LEDs uit wanneer terug naar grid
    Event ledOffEvent;
    ledOffEvent.type = EventType::SCREEN_CHANGED;
    ledOffEvent.param1 = 2;  // 2 = LEDs uit
    EventBus::getInstance().dispatch(ledOffEvent);
  }

  int getTotalPages() {
    return (allItems.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
  }

  int getCurrentPage() {
    return (scrollOffset / ITEMS_PER_PAGE) + 1;
  }

public:
  HomeScreen(DisplayManager* d) : display(d) {
    loadAllItems();
  }

  ScreenType getType() const override { return ScreenType::HOME; }

  void onEnter() override {
    Serial.println("HomeScreen: Entering");
    display->getTFT()->setRotation(0);
    mode = HomeScreenMode::GRID;
    needsRedraw = true;
  }

  void onExit() override {
    Serial.println("HomeScreen: Exiting");
  }

  void handleEvent(const Event& event) override {
    if (event.type == EventType::TOUCH_PRESSED) {
      handleTouchEvent(event);
    } else if (event.type == EventType::SWIPE_LEFT) {
      // Swipe links = volgende pagina
      if (mode == HomeScreenMode::GRID) {
        Serial.println("Swipe left -> next page");
        scrollDown();
      }
    } else if (event.type == EventType::SWIPE_RIGHT) {
      // Swipe rechts = vorige pagina
      if (mode == HomeScreenMode::GRID) {
        Serial.println("Swipe right -> prev page");
        scrollUp();
      }
    }
  }

  void handleTouchEvent(const Event& event) {
    int x = event.param1;
    int y = event.param2;

    Serial.printf("Touch: x=%d, y=%d (mode=%d)\n", x, y, (int)mode);

    switch (mode) {
      case HomeScreenMode::RESULT:
        returnToGrid();
        break;
        
      case HomeScreenMode::DIRTY_POPUP:
        handlePopupClick(x, y);
        break;
        
      case HomeScreenMode::GRID:
        // Check voor search icon (rechtsboven, x > 200, y < 40)
        if (x > 200 && y < 40) {
          Serial.println("Search icon tapped!");
          Event searchEvent;
          searchEvent.type = EventType::SCREEN_CHANGED;
          searchEvent.param1 = 3;  // 3 = go to search
          EventBus::getInstance().dispatch(searchEvent);
          return;
        }
        
        // Footer area (y >= 270 = 320-50)
        if (y >= 270) {
          // Check voor vorige button (links, x < 80)
          if (x < 80) {
            Serial.println("Prev button tapped");
            scrollUp();
            return;
          }
          // Check voor volgende button (rechts, x > 160)
          if (x > 160) {
            Serial.println("Next button tapped");
            scrollDown();
            return;
          }
          // Midden van footer - doe niets
          return;
        }
        // Header tapped - doe niets (of scroll omhoog als je wilt)
        if (y < 40) {
          return;
        }
        // Grid area (y between 40 and 269)
        Serial.printf("Grid area tapped: y=%d\n", y);
        handleGridClick(x, y);
        break;
    }
  }

  void update() override {}

  void render() override {
    if (!needsRedraw) return;
    needsRedraw = false;
    
    switch (mode) {
      case HomeScreenMode::GRID:
        display->clear();
        display->drawHeader("Afval Sorteren", getCurrentPage(), getTotalPages());
        display->drawItemGrid(allItems, scrollOffset);
        display->drawFooter("Volgende >", getCurrentPage(), getTotalPages());
        break;
        
      case HomeScreenMode::DIRTY_POPUP:
        if (pendingItemIndex >= 0) {
          display->drawDirtyCleanPopup(allItems[pendingItemIndex]);
        }
        break;
        
      case HomeScreenMode::RESULT:
        if (selectedItemIndex >= 0) {
          display->drawResultScreen(allItems[selectedItemIndex], 
                                    allItems[selectedItemIndex].isDirty);
        }
        break;
    }
    Serial.printf("Rendered (mode=%d, page=%d/%d)\n", (int)mode, getCurrentPage(), getTotalPages());
  }

  void scrollUp() {
    if (scrollOffset >= ITEMS_PER_PAGE) {
      scrollOffset -= ITEMS_PER_PAGE;
    } else {
      // Wrap around naar laatste pagina
      int lastPageOffset = ((allItems.size() - 1) / ITEMS_PER_PAGE) * ITEMS_PER_PAGE;
      scrollOffset = lastPageOffset;
    }
    needsRedraw = true;
    Serial.printf("Page up: offset=%d\n", scrollOffset);
  }

  void scrollDown() {
    if (scrollOffset + ITEMS_PER_PAGE < allItems.size()) {
      scrollOffset += ITEMS_PER_PAGE;
    } else {
      // Wrap around naar eerste pagina
      scrollOffset = 0;
    }
    needsRedraw = true;
    Serial.printf("Page down: offset=%d\n", scrollOffset);
  }

  const std::vector<Item>& getAllItems() const { return allItems; }
  void clearSelection() { selectedItemIndex = -1; needsRedraw = true; }
};

#endif