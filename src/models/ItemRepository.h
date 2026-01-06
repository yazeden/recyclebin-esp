#ifndef ITEM_REPOSITORY_H
#define ITEM_REPOSITORY_H

#include "Item.h"
#include <vector>
#include <algorithm>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../services/DatabaseService.h"

class ItemRepository {
private:
  std::vector<Item> items;
  bool dataFromDatabase = false;
  bool usingCachedData = false;
  String dataSource = "local";

  ItemRepository() {}

  // Sort items alphabetically by name
  void sortItemsAlphabetically() {
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
      return a.name < b.name;
    });
    // Update ledIndex after sorting
    for (size_t i = 0; i < items.size(); i++) {
      items[i].ledIndex = i;
    }
  }

public:
  static ItemRepository& getInstance() {
    static ItemRepository instance;
    return instance;
  }

  void addItem(const Item& item) {
    items.push_back(item);
  }

  std::vector<Item> getAllItems() const {
    return items;
  }

  // Get items by starting letter (for alphabetical sidebar)
  std::vector<Item> getItemsByLetter(char letter) const {
    std::vector<Item> filtered;
    char upperLetter = toupper(letter);
    for (const auto& item : items) {
      if (item.name.length() > 0 && toupper(item.name[0]) == upperLetter) {
        filtered.push_back(item);
      }
    }
    return filtered;
  }

  // Get all unique starting letters
  std::vector<char> getAvailableLetters() const {
    std::vector<char> letters;
    for (const auto& item : items) {
      if (item.name.length() > 0) {
        char letter = toupper(item.name[0]);
        bool found = false;
        for (char l : letters) {
          if (l == letter) { found = true; break; }
        }
        if (!found) letters.push_back(letter);
      }
    }
    std::sort(letters.begin(), letters.end());
    return letters;
  }

  std::vector<Item> getItemsByCategory(ItemCategory category) const {
    std::vector<Item> filtered;
    for (const auto& item : items) {
      if (item.category == category) {
        filtered.push_back(item);
      }
    }
    return filtered;
  }

  Item* getItemById(int id) {
    for (auto& item : items) {
      if (item.id == id) {
        return &item;
      }
    }
    return nullptr;
  }

  // Load items from JSON file
  bool loadFromJSON(const char* filename = "/catalogus.json") {
    items.clear();

    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS mount failed!");
      return false;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
      Serial.printf("Failed to open %s\n", filename);
      return false;
    }

    // Parse JSON (ArduinoJson v6)
    DynamicJsonDocument doc(32768);  // 32KB for 84 items
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      Serial.printf("JSON parse error: %s\n", error.c_str());
      return false;
    }

    JsonArray itemsArray = doc["items"];
    
    for (JsonObject obj : itemsArray) {
      Item item;
      item.id = obj["id"] | 0;
      item.name = String(obj["name"] | "Unknown");
      item.description = String(obj["description"] | "");
      
      // Parse category
      const char* cat = obj["category"] | "waste";
      item.category = Item::stringToCategory(cat);
      
      // Parse color (hex string like "0xFD20")
      const char* colorStr = obj["color"] | "0x6B4D";
      item.color = (uint16_t)strtol(colorStr, NULL, 16);
      
      item.isDirty = false;
      item.canBeDirty = obj["canBeDirty"] | false;
      item.ledIndex = items.size();

      items.push_back(item);
    }

    // Sort alphabetically after loading
    sortItemsAlphabetically();

    Serial.printf("Loaded %d items from JSON (sorted A-Z)\n", items.size());
    return true;
  }

  // Fallback: hardcoded items (for testing without filesystem)
  void loadHardcodedItems() {
    Serial.println("Loading items...");
    items.clear();
    
    // Try database first, then JSON, then hardcoded
    if (loadFromDatabase()) {
      return;
    }
    
    if (loadFromJSON()) {
      return;
    }
    
    Serial.println("JSON load failed, using hardcoded items");
    
    Item i1; i1.id = 1; i1.name = "Plastic Fles"; i1.category = ItemCategory::PLASTIC; 
    i1.color = 0xFD20; i1.isDirty = false; i1.canBeDirty = true; i1.ledIndex = 0; i1.description = "Leeg en gespoeld";
    items.push_back(i1);
    
    Item i2; i2.id = 2; i2.name = "Papier"; i2.category = ItemCategory::PAPER;
    i2.color = 0x001F; i2.isDirty = false; i2.canBeDirty = false; i2.ledIndex = 1; i2.description = "Onbeschadigd papier";
    items.push_back(i2);
    
    Item i3; i3.id = 3; i3.name = "Appel"; i3.category = ItemCategory::GREEN;
    i3.color = 0x07E0; i3.isDirty = false; i3.canBeDirty = false; i3.ledIndex = 2; i3.description = "Biologisch afval";
    items.push_back(i3);
    
    Item i4; i4.id = 4; i4.name = "Blikje"; i4.category = ItemCategory::WASTE;
    i4.color = 0x8410; i4.isDirty = false; i4.canBeDirty = false; i4.ledIndex = 3; i4.description = "Aluminium blikje";
    items.push_back(i4);
  }

  // Load items from remote database API
  bool loadFromDatabase() {
    DatabaseService& db = DatabaseService::getInstance();
    
    Serial.println("Attempting to load from database API...");
    
    std::vector<Item> dbItems;
    if (db.fetchItems(dbItems) && dbItems.size() > 0) {
      items = dbItems;
      dataFromDatabase = true;
      usingCachedData = db.isUsingCachedData();
      dataSource = db.getDataSource();
      
      // Sort alphabetically
      sortItemsAlphabetically();
      
      Serial.printf("Loaded %d items from %s\n", items.size(), dataSource.c_str());
      return true;
    }
    
    Serial.println("Database load failed, trying local JSON...");
    return false;
  }
  
  // Post item selection to database
  bool postItemSelection(const String& location, const Item& item) {
    DatabaseService& db = DatabaseService::getInstance();
    return db.postItemSelection(location, item.name, item.isDirty);
  }
  
  // Refresh data from database
  bool refreshFromDatabase() {
    return loadFromDatabase();
  }
  
  // Status getters
  bool isDataFromDatabase() const { return dataFromDatabase; }
  bool isUsingCachedData() const { return usingCachedData; }
  String getDataSource() const { return dataSource; }

  int getItemCount() const {
    return items.size();
  }
};

#endif