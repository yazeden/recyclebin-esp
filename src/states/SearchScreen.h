#ifndef SEARCH_SCREEN_H
#define SEARCH_SCREEN_H

#include "ScreenState.h"
#include "../display/DisplayManager.h"
#include "../models/Item.h"
#include "../models/ItemRepository.h"
#include <vector>

// Keyboard layout: 6 columns x 5 rows = 30 keys (26 letters + clear + back + space + .)
#define KB_COLS 6
#define KB_ROWS 5

class SearchScreen : public ScreenState {
private:
  DisplayManager* display;
  std::vector<Item> allItems;
  std::vector<Item> filteredItems;
  String searchQuery = "";
  bool needsRedraw = true;
  bool showingResults = false;
  int resultScrollOffset = 0;

  // Keyboard characters
  const char* keyboardChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ<X .";
  //                           0123456789...                   26=< 27=X 28=space 29=.
  
  void loadAllItems() {
    ItemRepository& repo = ItemRepository::getInstance();
    allItems = repo.getAllItems();
  }

  void filterItems() {
    filteredItems.clear();
    if (searchQuery.length() == 0) {
      // Geen filter - toon niets of alles
      return;
    }
    
    String queryUpper = searchQuery;
    queryUpper.toUpperCase();
    
    for (const Item& item : allItems) {
      String nameUpper = item.name;
      nameUpper.toUpperCase();
      
      // ALLEEN startsWith - zoeken op eerste letter(s)
      if (nameUpper.startsWith(queryUpper)) {
        filteredItems.push_back(item);
      }
    }
    
    Serial.printf("Search '%s' found %d items\n", searchQuery.c_str(), filteredItems.size());
  }

  int getKeyAtPosition(int x, int y) {
    // Keyboard begint onder de zoekbalk
    int kbStartY = 60;  // Na search bar
    int kbHeight = PORTRAIT_HEIGHT - kbStartY;  // Rest van scherm
    
    if (y < kbStartY) return -1;  // In search bar
    
    int keyW = PORTRAIT_WIDTH / KB_COLS;  // 240/6 = 40
    int keyH = kbHeight / KB_ROWS;        // 260/5 = 52
    
    int col = x / keyW;
    int row = (y - kbStartY) / keyH;
    
    if (col >= KB_COLS) col = KB_COLS - 1;
    if (row >= KB_ROWS) row = KB_ROWS - 1;
    
    int keyIndex = row * KB_COLS + col;
    if (keyIndex >= 30) keyIndex = 29;  // Max 30 keys
    
    return keyIndex;
  }

  void handleKeyPress(int keyIndex) {
    if (keyIndex < 0 || keyIndex >= 30) return;
    
    char c = keyboardChars[keyIndex];
    
    if (keyIndex == 26) {
      // '<' = Backspace
      if (searchQuery.length() > 0) {
        searchQuery.remove(searchQuery.length() - 1);
        Serial.printf("Backspace -> '%s'\n", searchQuery.c_str());
      }
    } else if (keyIndex == 27) {
      // 'X' = Clear all / Exit
      if (searchQuery.length() > 0) {
        searchQuery = "";
        Serial.println("Clear search");
      } else {
        // Exit search
        exitSearch();
        return;
      }
    } else if (keyIndex == 28) {
      // Space - skip for now
    } else if (keyIndex == 29) {
      // '.' - skip for now
    } else {
      // Letter
      if (searchQuery.length() < 15) {  // Max 15 characters
        searchQuery += c;
        Serial.printf("Added '%c' -> '%s'\n", c, searchQuery.c_str());
      }
    }
    
    filterItems();
    needsRedraw = true;
  }

  void exitSearch() {
    // Dispatch event to go back to homescreen
    Event event;
    event.type = EventType::SCREEN_CHANGED;
    event.param1 = 0;  // 0 = back to home
    EventBus::getInstance().dispatch(event);
  }

  int getResultItemAtPosition(int x, int y) {
    // Results tonen: lijst onder zoekbalk + "X gevonden" header (20px)
    int listStartY = 80;  // 60 (search bar) + 20 (count header)
    int itemHeight = 50;
    
    if (y < listStartY) return -1;
    
    int touchY = y - listStartY;
    int rowIndex = touchY / itemHeight;
    int actualIndex = resultScrollOffset + rowIndex;
    
    if (actualIndex >= 0 && actualIndex < (int)filteredItems.size()) {
      return actualIndex;
    }
    return -1;
  }

  void handleResultClick(int index) {
    if (index < 0 || index >= (int)filteredItems.size()) return;
    
    const Item& item = filteredItems[index];
    Serial.printf("Selected from search: %s\n", item.name.c_str());
    
    // Dispatch item selected event
    Event event;
    event.type = EventType::ITEM_SELECTED;
    event.param1 = item.id;
    event.param2 = 0;  // Not dirty by default
    EventBus::getInstance().dispatch(event);
  }

public:
  SearchScreen(DisplayManager* d) : display(d) {
    loadAllItems();
  }

  ScreenType getType() const override { return ScreenType::SEARCH; }

  void onEnter() override {
    Serial.println("SearchScreen: Entering");
    searchQuery = "";
    filteredItems.clear();
    showingResults = false;
    resultScrollOffset = 0;
    needsRedraw = true;
  }

  void onExit() override {
    Serial.println("SearchScreen: Exiting");
  }

  void handleEvent(const Event& event) override {
    if (event.type == EventType::TOUCH_PRESSED) {
      int x = event.param1;
      int y = event.param2;
      
      bool hasResults = (filteredItems.size() > 0);
      int kbStartY = hasResults ? 140 : 60;
      
      Serial.printf("==> Search touch: x=%d, y=%d, results=%d, kbY=%d\n", 
                    x, y, filteredItems.size(), kbStartY);
      
      // X button rechtsboven (exit)
      if (x > 190 && y < 60) {
        Serial.println("==> Exit search");
        exitSearch();
        return;
      }
      
      // Resultaten area (y: 60-140)
      if (hasResults && y >= 60 && y < 140) {
        int resultIdx = getResultItemAtPositionCompact(x, y);
        Serial.printf("==> Result tap: idx=%d\n", resultIdx);
        if (resultIdx >= 0) {
          handleResultClick(resultIdx);
          return;
        }
      }
      
      // Keyboard area - ALTIJD actief als y >= kbStartY
      if (y >= kbStartY) {
        int keyIndex = getKeyAtPositionDynamic(x, y, kbStartY);
        Serial.printf("==> Key tap: idx=%d\n", keyIndex);
        if (keyIndex >= 0) {
          handleKeyPress(keyIndex);
          return;
        }
      }
      
      Serial.printf("==> Touch not handled: y=%d < kbStartY=%d\n", y, kbStartY);
    } else if (event.type == EventType::SWIPE_LEFT) {
      Serial.println("==> Swipe LEFT in search");
      if (filteredItems.size() > 0 && resultScrollOffset + 2 < (int)filteredItems.size()) {
        resultScrollOffset++;
        needsRedraw = true;
      }
    } else if (event.type == EventType::SWIPE_RIGHT) {
      Serial.println("==> Swipe RIGHT in search");
      if (resultScrollOffset > 0) {
        resultScrollOffset--;
        needsRedraw = true;
      }
    }
  }

  // Compact result position (for small result area 60-140)
  int getResultItemAtPositionCompact(int x, int y) {
    int listStartY = 60;
    int itemHeight = 40;  // Smaller items
    
    if (y < listStartY || y >= 140) return -1;
    
    int touchY = y - listStartY;
    int rowIndex = touchY / itemHeight;
    int actualIndex = resultScrollOffset + rowIndex;
    
    if (actualIndex >= 0 && actualIndex < (int)filteredItems.size()) {
      return actualIndex;
    }
    return -1;
  }

  // Dynamic keyboard position calculator
  int getKeyAtPositionDynamic(int x, int y, int kbStartY) {
    int kbHeight = PORTRAIT_HEIGHT - kbStartY;
    
    if (y < kbStartY) return -1;
    
    int keyW = PORTRAIT_WIDTH / KB_COLS;
    int keyH = kbHeight / KB_ROWS;
    
    int col = x / keyW;
    int row = (y - kbStartY) / keyH;
    
    if (col >= KB_COLS) col = KB_COLS - 1;
    if (row >= KB_ROWS) row = KB_ROWS - 1;
    
    int keyIndex = row * KB_COLS + col;
    if (keyIndex >= 30) keyIndex = 29;
    
    return keyIndex;
  }

  void update() override {
    // Nothing needed here
  }

  void render() override {
    if (!needsRedraw) return;
    needsRedraw = false;
    
    TFT_eSPI* tft = display->getTFT();
    bool hasResults = (filteredItems.size() > 0);
    int kbStartY = hasResults ? 140 : 60;
    
    // Clear screen
    tft->fillScreen(COLOR_BG);
    
    // Search bar header (60px)
    tft->fillRect(0, 0, PORTRAIT_WIDTH, 60, COLOR_HEADER);
    
    // X button rechtsboven
    tft->fillRoundRect(PORTRAIT_WIDTH - 45, 10, 35, 40, 5, TFT_RED);
    tft->setTextColor(TFT_WHITE, TFT_RED);
    tft->setTextDatum(MC_DATUM);
    tft->drawString("X", PORTRAIT_WIDTH - 27, 30, 4);
    
    // Search input field
    tft->fillRoundRect(10, 10, PORTRAIT_WIDTH - 65, 40, 8, TFT_WHITE);
    tft->setTextColor(TFT_BLACK, TFT_WHITE);
    tft->setTextDatum(ML_DATUM);
    
    String displayText = searchQuery.length() > 0 ? searchQuery : "Zoeken...";
    tft->drawString(displayText.c_str(), 20, 30, 2);
    
    // Cursor
    if (searchQuery.length() > 0) {
      int textW = tft->textWidth(searchQuery.c_str(), 2);
      tft->drawFastVLine(22 + textW, 18, 24, TFT_BLACK);
    }
    
    // Show compact results if any (between search bar and keyboard)
    if (hasResults) {
      drawResultsCompact(tft);
    }
    
    // ALWAYS show keyboard
    drawKeyboardDynamic(tft, kbStartY);
    
    tft->setTextDatum(TL_DATUM);
  }

  void drawResultsCompact(TFT_eSPI* tft) {
    // Compact results area: Y 60-140 (80px height, 2 items of 40px each)
    int startY = 60;
    int itemHeight = 40;
    int maxVisible = 2;
    
    for (int i = 0; i < maxVisible && (resultScrollOffset + i) < (int)filteredItems.size(); i++) {
      const Item& item = filteredItems[resultScrollOffset + i];
      int y = startY + i * itemHeight;
      
      // Item background
      tft->fillRoundRect(5, y + 2, PORTRAIT_WIDTH - 10, itemHeight - 4, 6, item.color);
      
      // Item name
      tft->setTextColor(TFT_WHITE, item.color);
      tft->setTextDatum(ML_DATUM);
      
      String name = item.name;
      if (name.length() > 20) {
        name = name.substring(0, 18) + "..";
      }
      tft->drawString(name.c_str(), 15, y + itemHeight/2, 2);
    }
    
    // Show count
    tft->setTextColor(TFT_DARKGREY, COLOR_BG);
    tft->setTextDatum(MR_DATUM);
    char countStr[15];
    sprintf(countStr, "%d/%d", resultScrollOffset + 1, filteredItems.size());
    tft->drawString(countStr, PORTRAIT_WIDTH - 10, 135, 1);
  }

  void drawKeyboardDynamic(TFT_eSPI* tft, int kbStartY) {
    int kbHeight = PORTRAIT_HEIGHT - kbStartY;
    int keyW = PORTRAIT_WIDTH / KB_COLS;
    int keyH = kbHeight / KB_ROWS;
    
    for (int i = 0; i < 30; i++) {
      int col = i % KB_COLS;
      int row = i / KB_COLS;
      int x = col * keyW;
      int y = kbStartY + row * keyH;
      
      char c = keyboardChars[i];
      
      // Key background color
      uint16_t keyColor;
      if (i == 26) {
        keyColor = TFT_ORANGE;  // Backspace
      } else if (i == 27) {
        keyColor = TFT_RED;     // Clear/Exit
      } else if (i == 28 || i == 29) {
        keyColor = TFT_DARKGREY; // Space, dot
      } else {
        keyColor = TFT_NAVY;    // Letters
      }
      
      // Draw key
      tft->fillRoundRect(x + 2, y + 2, keyW - 4, keyH - 4, 4, keyColor);
      
      // Key label
      tft->setTextColor(TFT_WHITE, keyColor);
      tft->setTextDatum(MC_DATUM);
      
      char label[2] = {c, '\0'};
      if (i == 26) {
        tft->drawString("<", x + keyW/2, y + keyH/2, 2);
      } else if (i == 27) {
        tft->drawString("X", x + keyW/2, y + keyH/2, 2);
      } else if (i == 28) {
        tft->drawString("_", x + keyW/2, y + keyH/2, 2);
      } else {
        tft->drawString(label, x + keyW/2, y + keyH/2, 2);
      }
    }
  }

  void drawKeyboard(TFT_eSPI* tft) {
    int kbStartY = 60;
    int kbHeight = PORTRAIT_HEIGHT - kbStartY;  // 260
    int keyW = PORTRAIT_WIDTH / KB_COLS;        // 40
    int keyH = kbHeight / KB_ROWS;              // 52
    
    for (int i = 0; i < 30; i++) {
      int col = i % KB_COLS;
      int row = i / KB_COLS;
      int x = col * keyW;
      int y = kbStartY + row * keyH;
      
      char c = keyboardChars[i];
      
      // Key background color
      uint16_t keyColor;
      if (i == 26) {
        keyColor = TFT_ORANGE;  // Backspace
      } else if (i == 27) {
        keyColor = TFT_RED;     // Clear/Exit
      } else if (i == 28 || i == 29) {
        keyColor = TFT_DARKGREY; // Space, dot
      } else {
        keyColor = TFT_NAVY;    // Letters
      }
      
      // Draw key
      tft->fillRoundRect(x + 2, y + 2, keyW - 4, keyH - 4, 5, keyColor);
      tft->drawRoundRect(x + 2, y + 2, keyW - 4, keyH - 4, 5, TFT_WHITE);
      
      // Key label
      tft->setTextColor(TFT_WHITE, keyColor);
      tft->setTextDatum(MC_DATUM);
      
      char label[2] = {c, '\0'};
      if (i == 26) {
        tft->drawString("<-", x + keyW/2, y + keyH/2, 2);
      } else if (i == 27) {
        tft->drawString("X", x + keyW/2, y + keyH/2, 2);
      } else if (i == 28) {
        tft->drawString("_", x + keyW/2, y + keyH/2, 2);
      } else {
        tft->drawString(label, x + keyW/2, y + keyH/2, 2);
      }
    }
  }

  void drawResults(TFT_eSPI* tft) {
    int startY = 60;
    int itemHeight = 50;
    int maxVisible = (PORTRAIT_HEIGHT - startY) / itemHeight;  // ~5 items
    
    // Result count header
    tft->setTextColor(TFT_WHITE, COLOR_BG);
    tft->setTextDatum(TL_DATUM);
    char countStr[30];
    sprintf(countStr, "%d gevonden", filteredItems.size());
    tft->drawString(countStr, 10, startY + 5, 1);
    startY += 20;
    
    for (int i = 0; i < maxVisible && (resultScrollOffset + i) < (int)filteredItems.size(); i++) {
      const Item& item = filteredItems[resultScrollOffset + i];
      int y = startY + i * itemHeight;
      
      // Item background
      tft->fillRoundRect(5, y, PORTRAIT_WIDTH - 10, itemHeight - 5, 8, item.color);
      
      // Item name
      tft->setTextColor(TFT_WHITE, item.color);
      tft->setTextDatum(ML_DATUM);
      
      String name = item.name;
      if (name.length() > 22) {
        name = name.substring(0, 20) + "..";
      }
      tft->drawString(name.c_str(), 15, y + itemHeight/2, 2);
    }
    
    // Scroll indicator
    if (filteredItems.size() > maxVisible) {
      int scrollBarH = 60;
      int scrollTrackH = PORTRAIT_HEIGHT - startY - 10;
      int scrollPos = (resultScrollOffset * scrollTrackH) / filteredItems.size();
      
      tft->fillRect(PORTRAIT_WIDTH - 8, startY, 5, scrollTrackH, TFT_DARKGREY);
      tft->fillRect(PORTRAIT_WIDTH - 8, startY + scrollPos, 5, scrollBarH, TFT_WHITE);
    }
  }

  // Setters for external control
  void setSearchQuery(const String& query) {
    searchQuery = query;
    filterItems();
    needsRedraw = true;
  }

  const std::vector<Item>& getFilteredItems() const { return filteredItems; }
};

#endif
