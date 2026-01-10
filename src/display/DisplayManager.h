#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include <vector>
#include "../config.h"
#include "../models/Item.h"

// Layout constants - 2x2 grid, full width
#define GRID_ITEM_COLS 2
#define GRID_ITEM_ROWS 2
#define ITEMS_PER_PAGE (GRID_ITEM_COLS * GRID_ITEM_ROWS)

class DisplayManager {
private:
  TFT_eSPI* tft;

public:
  DisplayManager(TFT_eSPI* t) : tft(t) {}

  void init() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft->fillScreen(COLOR_BG);
    Serial.println("Display initialized!");
  }

  // ========== STATUS MESSAGE (for WiFi, loading, etc.) ==========
  void showMessage(const char* message) {
    tft->fillScreen(COLOR_BG);
    tft->setTextColor(TFT_WHITE, COLOR_BG);
    tft->setTextDatum(MC_DATUM);
    
    // Handle multi-line messages (split by \n)
    String msg = String(message);
    int lineHeight = 25;
    int startY = PORTRAIT_HEIGHT / 2 - lineHeight;
    
    int lineNum = 0;
    int lastIdx = 0;
    int idx;
    while ((idx = msg.indexOf('\n', lastIdx)) != -1) {
      String line = msg.substring(lastIdx, idx);
      tft->drawString(line.c_str(), PORTRAIT_WIDTH / 2, startY + lineNum * lineHeight, 2);
      lineNum++;
      lastIdx = idx + 1;
    }
    // Last line
    String lastLine = msg.substring(lastIdx);
    tft->drawString(lastLine.c_str(), PORTRAIT_WIDTH / 2, startY + lineNum * lineHeight, 2);
    
    tft->setTextDatum(TL_DATUM);
  }

  // ========== HEADER ==========
  void drawHeader(const char* title, int currentPage, int totalPages, bool showSearch = true) {
    tft->fillRect(0, 0, PORTRAIT_WIDTH, HEADER_HEIGHT, COLOR_HEADER);
    tft->setTextColor(TFT_WHITE, COLOR_HEADER);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(title, PORTRAIT_WIDTH / 2, HEADER_HEIGHT / 2, 2);
    
    // Page indicator (links)
    if (totalPages > 1) {
      char pageStr[10];
      sprintf(pageStr, "%d/%d", currentPage, totalPages);
      tft->setTextDatum(ML_DATUM);
      tft->drawString(pageStr, 5, HEADER_HEIGHT / 2, 1);
    }
    
    // Search icon (rechts) - vergrootglas
    if (showSearch) {
      int iconX = PORTRAIT_WIDTH - 30;
      int iconY = HEADER_HEIGHT / 2;
      
      // Cirkel van vergrootglas
      tft->drawCircle(iconX, iconY - 2, 8, TFT_WHITE);
      tft->drawCircle(iconX, iconY - 2, 7, TFT_WHITE);
      
      // Steel van vergrootglas
      tft->drawLine(iconX + 6, iconY + 4, iconX + 12, iconY + 10, TFT_WHITE);
      tft->drawLine(iconX + 7, iconY + 4, iconX + 13, iconY + 10, TFT_WHITE);
    }
    
    tft->setTextDatum(TL_DATUM);
  }

  // ========== ITEM GRID (2x2, full width) ==========
  void drawItemGrid(const std::vector<Item>& items, int scrollOffset = 0) {
    int contentHeight = PORTRAIT_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;
    int itemWidth = PORTRAIT_WIDTH / GRID_ITEM_COLS;   // 120px each
    int itemHeight = contentHeight / GRID_ITEM_ROWS;   // ~125px each
    int startY = HEADER_HEIGHT;

    // Clear content area
    tft->fillRect(0, startY, PORTRAIT_WIDTH, contentHeight, COLOR_BG);

    for (int i = 0; i < ITEMS_PER_PAGE && (scrollOffset + i) < items.size(); i++) {
      int idx = scrollOffset + i;
      const Item& item = items[idx];
      
      int col = i % GRID_ITEM_COLS;
      int row = i / GRID_ITEM_COLS;
      int x = col * itemWidth;
      int y = startY + row * itemHeight;

      drawItemBox(x, y, itemWidth, itemHeight, item);
    }
  }

  // ========== SINGLE ITEM BOX (groot vierkant) ==========
  void drawItemBox(int x, int y, int width, int height, const Item& item) {
    int padding = 5;
    int boxX = x + padding;
    int boxY = y + padding;
    int boxW = width - padding * 2;
    int boxH = height - padding * 2;

    // Item achtergrond met kleur
    tft->fillRoundRect(boxX, boxY, boxW, boxH, 10, item.color);
    
    // Witte rand
    tft->drawRoundRect(boxX, boxY, boxW, boxH, 10, TFT_WHITE);
    tft->drawRoundRect(boxX+1, boxY+1, boxW-2, boxH-2, 9, TFT_WHITE);

    // Item naam (wit op kleur)
    tft->setTextColor(TFT_WHITE, item.color);
    tft->setTextDatum(MC_DATUM);
    
    String name = item.name;
    int centerX = boxX + boxW/2;
    int centerY = boxY + boxH/2;
    
    // Check lengte en splits indien nodig
    if (name.length() <= 10) {
      // Kort - past in 1 regel
      tft->drawString(name.c_str(), centerX, centerY, 2);
    } else {
      // Lang - splits over meerdere regels
      int splitIdx = -1;
      // Zoek beste split punt (spatie, komma, haakje)
      for (int i = name.length() / 2; i >= 0; i--) {
        char c = name.charAt(i);
        if (c == ' ' || c == ',' || c == '(') {
          splitIdx = i;
          break;
        }
      }
      if (splitIdx < 0) {
        for (int i = name.length() / 2; i < (int)name.length(); i++) {
          char c = name.charAt(i);
          if (c == ' ' || c == ',' || c == '(') {
            splitIdx = i;
            break;
          }
        }
      }
      
      if (splitIdx > 0 && name.length() <= 26) {
        // Split in 2 regels
        String line1 = name.substring(0, splitIdx);
        String line2 = name.substring(splitIdx + 1);
        line1.trim();
        line2.trim();
        
        // Check of 2 regels genoeg is
        if (line2.length() > 14) {
          // 3 regels nodig
          int split2 = -1;
          for (int i = line2.length() / 2; i >= 0; i--) {
            char c = line2.charAt(i);
            if (c == ' ' || c == ',' || c == '(') {
              split2 = i;
              break;
            }
          }
          if (split2 > 0) {
            String line3 = line2.substring(split2 + 1);
            line2 = line2.substring(0, split2);
            line2.trim();
            line3.trim();
            tft->drawString(line1.c_str(), centerX, centerY - 15, 1);
            tft->drawString(line2.c_str(), centerX, centerY, 1);
            tft->drawString(line3.c_str(), centerX, centerY + 15, 1);
          } else {
            tft->drawString(line1.c_str(), centerX, centerY - 8, 1);
            tft->drawString(line2.c_str(), centerX, centerY + 8, 1);
          }
        } else {
          tft->drawString(line1.c_str(), centerX, centerY - 8, 1);
          tft->drawString(line2.c_str(), centerX, centerY + 8, 1);
        }
      } else {
        // Geen split punt - afkappen
        name = name.substring(0, 12) + "..";
        tft->drawString(name.c_str(), centerX, centerY, 1);
      }
    }
    
    // Vies/schoon indicator
    if (item.canBeDirty) {
      tft->fillCircle(boxX + boxW - 15, boxY + 15, 10, TFT_WHITE);
      tft->setTextColor(item.color, TFT_WHITE);
      tft->setTextDatum(MC_DATUM);
      tft->drawString("?", boxX + boxW - 15, boxY + 15, 2);
    }
    
    tft->setTextDatum(TL_DATUM);
  }

  // ========== DIRTY/CLEAN POPUP ==========
  void drawDirtyCleanPopup(const Item& item) {
    // Volledig scherm popup voor betere touch
    tft->fillScreen(COLOR_BG);
    
    // Header met naam (aangepast voor lange namen)
    tft->fillRect(0, 0, PORTRAIT_WIDTH, 60, item.color);
    tft->setTextColor(TFT_WHITE, item.color);
    tft->setTextDatum(MC_DATUM);
    
    String name = item.name;
    if (name.length() <= 10) {
      tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 4);
    } else if (name.length() <= 18) {
      tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 2);
    } else {
      // Split in 2 regels
      int splitIdx = -1;
      for (int i = name.length() / 2; i >= 0; i--) {
        char c = name.charAt(i);
        if (c == ' ' || c == ',' || c == '(') {
          splitIdx = i;
          break;
        }
      }
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
        tft->drawString(line1.c_str(), PORTRAIT_WIDTH / 2, 20, 2);
        tft->drawString(line2.c_str(), PORTRAIT_WIDTH / 2, 42, 2);
      } else {
        name = name.substring(0, 18) + "..";
        tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 2);
      }
    }
    
    // Vraag
    tft->setTextColor(COLOR_TEXT, COLOR_BG);
    tft->drawString("Is het schoon of vies?", PORTRAIT_WIDTH / 2, 100, 2);
    
    // GROTE buttons - hele breedte, makkelijk te raken
    int btnW = 200;
    int btnH = 60;
    int btnX = (PORTRAIT_WIDTH - btnW) / 2;  // 20
    
    // Schoon button (groen) - Y: 140-200
    tft->fillRoundRect(btnX, 140, btnW, btnH, 10, COLOR_GREEN);
    tft->setTextColor(TFT_WHITE, COLOR_GREEN);
    tft->drawString("SCHOON", PORTRAIT_WIDTH / 2, 170, 4);
    
    // Vies button (oranje) - Y: 220-280
    tft->fillRoundRect(btnX, 220, btnW, btnH, 10, COLOR_PLASTIC);
    tft->setTextColor(TFT_WHITE, COLOR_PLASTIC);
    tft->drawString("VIES", PORTRAIT_WIDTH / 2, 250, 4);
    
    tft->setTextDatum(TL_DATUM);
  }

  // ========== RESULT SCREEN ==========
  void drawResultScreen(const Item& item, bool isDirty) {
    tft->fillScreen(COLOR_BG);
    
    // Header with result - use proper Dutch names
    const char* resultText;
    if (isDirty) {
      resultText = "Restafval";
    } else {
      switch (item.category) {
        case ItemCategory::PLASTIC: resultText = "Plastic"; break;
        case ItemCategory::PAPER: resultText = "Papier"; break;
        case ItemCategory::GREEN: resultText = "Groen"; break;
        default: resultText = "Restafval"; break;
      }
    }
    uint16_t resultColor = isDirty ? COLOR_WASTE : item.color;
    
    tft->fillRect(0, 0, PORTRAIT_WIDTH, 60, resultColor);
    tft->setTextColor(TFT_WHITE, resultColor);
    tft->setTextDatum(MC_DATUM);
    
    // Handle long names in header
    String name = item.name;
    if (name.length() <= 10) {
      // Short name - use large font
      tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 4);
    } else if (name.length() <= 18) {
      // Medium name - use smaller font
      tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 2);
    } else {
      // Very long name - split into 2 lines
      int splitIdx = -1;
      for (int i = name.length() / 2; i >= 0; i--) {
        char c = name.charAt(i);
        if (c == ' ' || c == ',' || c == '(') {
          splitIdx = i;
          break;
        }
      }
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
        tft->drawString(line1.c_str(), PORTRAIT_WIDTH / 2, 20, 2);
        tft->drawString(line2.c_str(), PORTRAIT_WIDTH / 2, 42, 2);
      } else {
        // Truncate if no good split point
        name = name.substring(0, 20) + "..";
        tft->drawString(name.c_str(), PORTRAIT_WIDTH / 2, 30, 2);
      }
    }
    
    // Large icon area
    int iconSize = 120;
    int iconX = (PORTRAIT_WIDTH - iconSize) / 2;
    int iconY = 80;
    
    tft->fillRoundRect(iconX, iconY, iconSize, iconSize, 15, resultColor);
    
    // Category symbol
    tft->setTextColor(TFT_WHITE, resultColor);
    tft->setTextDatum(MC_DATUM);
    
    const char* symbol;
    if (isDirty) {
      symbol = "REST";
    } else {
      switch (item.category) {
        case ItemCategory::PLASTIC: symbol = "PLASTIC"; break;
        case ItemCategory::PAPER: symbol = "PAPIER"; break;
        case ItemCategory::GREEN: symbol = "GROEN"; break;
        default: symbol = "REST"; break;
      }
    }
    tft->drawString(symbol, PORTRAIT_WIDTH / 2, iconY + iconSize / 2, 4);  // Font 4 voor grotere tekst
    
    // Description
    tft->setTextColor(COLOR_TEXT, COLOR_BG);
    tft->drawString(item.description.c_str(), PORTRAIT_WIDTH / 2, iconY + iconSize + 30, 2);
    
    // Footer
    tft->fillRect(0, PORTRAIT_HEIGHT - FOOTER_HEIGHT, PORTRAIT_WIDTH, FOOTER_HEIGHT, COLOR_HEADER);
    tft->setTextColor(TFT_WHITE, COLOR_HEADER);
    tft->drawString("Tik om terug te gaan", PORTRAIT_WIDTH / 2, PORTRAIT_HEIGHT - FOOTER_HEIGHT/2, 2);
    
    tft->setTextDatum(TL_DATUM);
  }

  // ========== FOOTER met navigatie knoppen ==========
  void drawFooter(const char* status, int currentPage = 1, int totalPages = 1) {
    int footerY = PORTRAIT_HEIGHT - FOOTER_HEIGHT;
    tft->fillRect(0, footerY, PORTRAIT_WIDTH, FOOTER_HEIGHT, COLOR_HEADER);
    
    if (totalPages > 1) {
      int btnWidth = 70;
      int btnHeight = 40;
      int btnY = footerY + (FOOTER_HEIGHT - btnHeight) / 2;
      
      // Vorige button (links) - altijd actief (wrap-around)
      tft->fillRoundRect(5, btnY, btnWidth, btnHeight, 8, COLOR_ACCENT);
      tft->setTextColor(TFT_WHITE, COLOR_ACCENT);
      tft->setTextDatum(MC_DATUM);
      tft->drawString("<", 40, btnY + btnHeight/2, 4);
      
      // Pagina nummer in midden
      char pageText[15];
      sprintf(pageText, "%d / %d", currentPage, totalPages);
      tft->setTextColor(TFT_WHITE, COLOR_HEADER);
      tft->setTextDatum(MC_DATUM);
      tft->drawString(pageText, PORTRAIT_WIDTH / 2, footerY + FOOTER_HEIGHT / 2, 2);
      
      // Volgende button (rechts) - altijd actief (wrap-around)
      tft->fillRoundRect(PORTRAIT_WIDTH - btnWidth - 5, btnY, btnWidth, btnHeight, 8, COLOR_ACCENT);
      tft->setTextColor(TFT_WHITE, COLOR_ACCENT);
      tft->setTextDatum(MC_DATUM);
      tft->drawString(">", PORTRAIT_WIDTH - 40, btnY + btnHeight/2, 4);
    } else {
      tft->setTextColor(TFT_WHITE, COLOR_HEADER);
      tft->setTextDatum(MC_DATUM);
      tft->drawString(status, PORTRAIT_WIDTH / 2, footerY + FOOTER_HEIGHT / 2, 2);
    }
    tft->setTextDatum(TL_DATUM);
  }

  // ========== UTILITIES ==========
  void drawLoadingScreen(const char* message) {
    tft->fillScreen(COLOR_BG);
    tft->setTextColor(COLOR_TEXT, COLOR_BG);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(message, PORTRAIT_WIDTH / 2, PORTRAIT_HEIGHT / 2, 2);
    tft->setTextDatum(TL_DATUM);
  }

  void drawErrorScreen(const char* error) {
    tft->fillScreen(COLOR_BG);
    tft->setTextColor(COLOR_ACCENT, COLOR_BG);
    tft->setTextDatum(MC_DATUM);
    tft->drawString(error, PORTRAIT_WIDTH / 2, PORTRAIT_HEIGHT / 2, 2);
    tft->setTextDatum(TL_DATUM);
  }

  void clear() {
    tft->fillScreen(COLOR_BG);
  }

  void refresh() {}

  TFT_eSPI* getTFT() { 
    return tft; 
  }
};

#endif
