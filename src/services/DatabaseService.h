#ifndef DATABASE_SERVICE_H
#define DATABASE_SERVICE_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../models/Item.h"
#include "../config.h"
#include <vector>

class DatabaseService {
private:
    bool wifiConnected = false;
    bool usingCachedData = false;
    String lastUpdateTime = "never";
    String dataSource = "unknown";
    
    // Singleton
    DatabaseService() {}
    
public:
    static DatabaseService& getInstance() {
        static DatabaseService instance;
        return instance;
    }
    
    // Prevent copying
    DatabaseService(const DatabaseService&) = delete;
    void operator=(const DatabaseService&) = delete;
    
    // Initialize WiFi connection
    bool connectWiFi(unsigned long timeoutMs = 10000) {
        Serial.println("Connecting to WiFi...");
        Serial.printf("SSID: %s\n", WIFI_SSID);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeoutMs) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.println("\nWiFi connected!");
            Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        
        Serial.println("\nWiFi connection failed!");
        wifiConnected = false;
        return false;
    }
    
    bool isWiFiConnected() {
        wifiConnected = (WiFi.status() == WL_CONNECTED);
        return wifiConnected;
    }
    
    // Fetch items from API
    bool fetchItems(std::vector<Item>& items) {
        if (!isWiFiConnected()) {
            Serial.println("WiFi not connected, using cached data");
            return loadCachedItems(items);
        }
        
        HTTPClient http;
        String url = String("http://") + API_HOST + ":" + API_PORT + "/items";
        
        Serial.printf("Fetching items from: %s\n", url.c_str());
        
        http.begin(url);
        http.setTimeout(5000);  // 5 second timeout
        
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            http.end();
            
            // Parse JSON response
            DynamicJsonDocument doc(32768);
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.printf("JSON parse error: %s\n", error.c_str());
                return loadCachedItems(items);
            }
            
            // Check source (database or cache from server)
            dataSource = doc["source"] | "unknown";
            lastUpdateTime = doc["last_updated"] | "now";
            usingCachedData = (dataSource == "cache");
            
            Serial.printf("Data source: %s\n", dataSource.c_str());
            if (usingCachedData) {
                Serial.printf("Server using cached data from: %s\n", lastUpdateTime.c_str());
            }
            
            // Parse items array
            items.clear();
            JsonArray itemsArray = doc["items"];
            
            for (JsonVariant v : itemsArray) {
                Item item;
                
                // API returns tuple: [id, name, category, dirty]
                if (v.is<JsonArray>()) {
                    JsonArray arr = v.as<JsonArray>();
                    item.id = arr[0] | 0;
                    item.name = String(arr[1] | "Unknown");
                    
                    // Parse category from string
                    const char* cat = arr[2] | "waste";
                    item.category = Item::stringToCategory(cat);
                    
                    item.isDirty = arr[3] | false;
                    item.canBeDirty = true;
                    item.color = getCategoryColor(item.category);
                    item.description = "";
                    item.ledIndex = items.size();
                }
                // Or API returns object
                else if (v.is<JsonObject>()) {
                    JsonObject obj = v.as<JsonObject>();
                    item.id = obj["id"] | 0;
                    item.name = String(obj["name"] | "Unknown");
                    
                    const char* cat = obj["category"] | "waste";
                    item.category = Item::stringToCategory(cat);
                    
                    item.isDirty = obj["dirty"] | false;
                    item.canBeDirty = obj["canBeDirty"] | true;
                    item.color = getCategoryColor(item.category);
                    item.description = String(obj["description"] | "");
                    item.ledIndex = items.size();
                }
                
                items.push_back(item);
            }
            
            Serial.printf("Fetched %d items from API\n", items.size());
            
            // Cache the data locally
            saveCachedItems(items);
            
            return true;
        }
        
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        
        // Fallback to cached data
        return loadCachedItems(items);
    }
    
    // Post item selection to API
    bool postItemSelection(const String& location, const String& itemName, bool dirty) {
        if (!isWiFiConnected()) {
            Serial.println("WiFi not connected, queueing post for later");
            return queuePostForLater(location, itemName, dirty);
        }
        
        HTTPClient http;
        String url = String("http://") + API_HOST + ":" + API_PORT + 
                     "/sentData/" + location + "/" + itemName + "/" + (dirty ? "true" : "false");
        
        Serial.printf("Posting to: %s\n", url.c_str());
        
        http.begin(url);
        http.setTimeout(5000);
        
        int httpCode = http.POST("");
        
        if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            http.end();
            
            // Parse response to check source
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, response);
            
            const char* source = doc["source"] | "unknown";
            Serial.printf("Post result - source: %s\n", source);
            
            if (String(source) == "queued") {
                Serial.println("Server queued post (DB offline)");
            }
            
            return true;
        }
        
        Serial.printf("POST failed: %d\n", httpCode);
        http.end();
        
        // Queue for later
        return queuePostForLater(location, itemName, dirty);
    }
    
    // Check API/DB status
    bool checkApiStatus(bool& dbOnline, int& pendingPosts) {
        if (!isWiFiConnected()) {
            return false;
        }
        
        HTTPClient http;
        String url = String("http://") + API_HOST + ":" + API_PORT + "/status";
        
        http.begin(url);
        http.setTimeout(3000);
        
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            http.end();
            
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            
            dbOnline = doc["database_online"] | false;
            pendingPosts = doc["pending_posts_count"] | 0;
            
            Serial.printf("API Status - DB: %s, Pending: %d\n", 
                          dbOnline ? "online" : "offline", pendingPosts);
            
            return true;
        }
        
        http.end();
        return false;
    }
    
    // Getters for status
    bool isUsingCachedData() const { return usingCachedData; }
    String getDataSource() const { return dataSource; }
    String getLastUpdateTime() const { return lastUpdateTime; }
    
private:
    uint16_t getCategoryColor(ItemCategory cat) {
        switch (cat) {
            case ItemCategory::PLASTIC: return 0xFC00;  // Orange
            case ItemCategory::PAPER:   return 0x03BF;  // Blue
            case ItemCategory::GREEN:   return 0x07E0;  // Green
            case ItemCategory::WASTE:   
            default:                    return 0x6B4D;  // Grey
        }
    }
    
    // Save items to local cache
    bool saveCachedItems(const std::vector<Item>& items) {
        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS mount failed for cache save");
            return false;
        }
        
        DynamicJsonDocument doc(32768);
        JsonArray arr = doc.createNestedArray("items");
        
        for (const auto& item : items) {
            JsonObject obj = arr.createNestedObject();
            obj["id"] = item.id;
            obj["name"] = item.name;
            obj["category"] = Item::categoryToString(item.category);
            obj["color"] = String("0x") + String(item.color, HEX);
            obj["description"] = item.description;
            obj["canBeDirty"] = item.canBeDirty;
            obj["isDirty"] = item.isDirty;
        }
        
        File file = LittleFS.open("/db_cache.json", "w");
        if (!file) {
            Serial.println("Failed to open cache file for writing");
            return false;
        }
        
        serializeJson(doc, file);
        file.close();
        
        Serial.printf("Cached %d items to LittleFS\n", items.size());
        return true;
    }
    
    // Load items from local cache
    bool loadCachedItems(std::vector<Item>& items) {
        Serial.println("Loading from local cache...");
        
        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS mount failed");
            return false;
        }
        
        File file = LittleFS.open("/db_cache.json", "r");
        if (!file) {
            Serial.println("No cache file found");
            return false;
        }
        
        DynamicJsonDocument doc(32768);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            Serial.printf("Cache JSON error: %s\n", error.c_str());
            return false;
        }
        
        items.clear();
        JsonArray itemsArray = doc["items"];
        
        for (JsonObject obj : itemsArray) {
            Item item;
            item.id = obj["id"] | 0;
            item.name = String(obj["name"] | "Unknown");
            item.description = String(obj["description"] | "");
            
            const char* cat = obj["category"] | "waste";
            item.category = Item::stringToCategory(cat);
            
            const char* colorStr = obj["color"] | "0x6B4D";
            item.color = (uint16_t)strtol(colorStr, NULL, 16);
            
            item.isDirty = obj["isDirty"] | false;
            item.canBeDirty = obj["canBeDirty"] | false;
            item.ledIndex = items.size();
            
            items.push_back(item);
        }
        
        usingCachedData = true;
        dataSource = "local_cache";
        Serial.printf("Loaded %d items from local cache\n", items.size());
        
        return items.size() > 0;
    }
    
    // Queue failed POST for later
    bool queuePostForLater(const String& location, const String& itemName, bool dirty) {
        if (!LittleFS.begin(true)) {
            return false;
        }
        
        // Load existing queue
        DynamicJsonDocument doc(8192);
        File file = LittleFS.open("/pending_posts.json", "r");
        if (file) {
            deserializeJson(doc, file);
            file.close();
        }
        
        if (!doc.containsKey("posts")) {
            doc.createNestedArray("posts");
        }
        
        JsonArray posts = doc["posts"];
        JsonObject newPost = posts.createNestedObject();
        newPost["location"] = location;
        newPost["item"] = itemName;
        newPost["dirty"] = dirty;
        newPost["timestamp"] = millis();
        
        // Save queue
        file = LittleFS.open("/pending_posts.json", "w");
        if (file) {
            serializeJson(doc, file);
            file.close();
            Serial.println("Post queued for later");
            return true;
        }
        
        return false;
    }
    
public:
    // Process queued posts when connection is restored
    int processPendingPosts() {
        if (!isWiFiConnected()) {
            return 0;
        }
        
        if (!LittleFS.begin(true)) {
            return 0;
        }
        
        File file = LittleFS.open("/pending_posts.json", "r");
        if (!file) {
            return 0;
        }
        
        DynamicJsonDocument doc(8192);
        deserializeJson(doc, file);
        file.close();
        
        if (!doc.containsKey("posts")) {
            return 0;
        }
        
        JsonArray posts = doc["posts"];
        int processed = 0;
        
        for (JsonObject post : posts) {
            String location = post["location"] | "";
            String item = post["item"] | "";
            bool dirty = post["dirty"] | false;
            
            HTTPClient http;
            String url = String("http://") + API_HOST + ":" + API_PORT + 
                         "/sentData/" + location + "/" + item + "/" + (dirty ? "true" : "false");
            
            http.begin(url);
            http.setTimeout(5000);
            
            if (http.POST("") == HTTP_CODE_OK) {
                processed++;
            }
            
            http.end();
        }
        
        // Clear the queue
        if (processed > 0) {
            LittleFS.remove("/pending_posts.json");
            Serial.printf("Processed %d pending posts\n", processed);
        }
        
        return processed;
    }
};

#endif
