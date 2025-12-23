/*
 * SolaX Cloud Data Display
 * NodeMCU ESP8266 with 16x4 I2C LCD
 * 
 * Displays SolaX inverter data from cloud API
 * Alternates between two screens every 5 seconds
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "statusMapper.h"

// Initialize LCD
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Data storage
struct SolarData {
  float acPower;
  float yieldToday;
  float yieldTotal;
  float powerDC1;
  float powerDC2;
  String inverterStatus;
  bool valid;
} solarData;

// Timing variables
unsigned long lastApiUpdate = 0;
unsigned long lastScreenSwitch = 0;
bool currentScreen = 0;  // 0 = Screen 1 (Power Summary), 1 = Screen 2 (PV Details)

// Retry logic variables
int retryCount = 0;
unsigned long nextRetryTime = 0;
bool isRetrying = false;
const int retryDelays[] = {5000, 10000, 20000};  // Retry delays: 5s, 10s, 20s
const int maxRetries = 3;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSolaX Cloud Data Display");
  Serial.println("========================");
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Display startup message
  lcd.setCursor(0, 0);
  lcd.print("SolaX Could Display");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Connect to WiFi
  connectWiFi();
  
  // Initial data fetch
  fetchSolarData();
  
  // Initialize timing
  lastApiUpdate = millis();
  lastScreenSwitch = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Lost");
    lcd.setCursor(0, 1);
    lcd.print("Reconnecting...");
    connectWiFi();
  }
  
  // Update data from API every UPDATE_INTERVAL or handle retries
  if (isRetrying) {
    // Check if it's time to retry
    if (currentMillis >= nextRetryTime) {
      fetchSolarData();
      isRetrying = false;  // Will be set again if fetch fails
    }
  } else if (currentMillis - lastApiUpdate >= UPDATE_INTERVAL) {
    fetchSolarData();
    lastApiUpdate = currentMillis;
  }
  
  // Switch between screens every SCREEN_SWITCH_INTERVAL
  if (currentMillis - lastScreenSwitch >= SCREEN_SWITCH_INTERVAL) {
    currentScreen = !currentScreen;  // Toggle screen
    lastScreenSwitch = currentMillis;
  }
  
  // Display current screen
  if (solarData.valid) {
    if (currentScreen == 0) {
      displayScreen1();  // Power Summary
    } else {
      displayScreen2();  // PV Details
    }
  }
  
  delay(1000);  // Small delay to prevent excessive loop execution
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\nWiFi connection failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check config.h");
    delay(5000);
  }
}

void fetchSolarData() {
  Serial.println("Fetching data from SolaX Cloud API...");
  
  if (retryCount > 0) {
    Serial.printf("Retry attempt %d of %d\n", retryCount, maxRetries);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Updating...");
  if (retryCount > 0) {
    lcd.setCursor(0, 1);
    lcd.print("Retry ");
    lcd.print(retryCount);
    lcd.print("/");
    lcd.print(maxRetries);
  }
  
  WiFiClientSecure client;
  client.setInsecure();  // Skip SSL certificate verification
  
  HTTPClient https;
  bool success = false;
  
  if (https.begin(client, SOLAX_API_URL)) {
    // Add headers
    https.addHeader("Content-Type", "application/json");
    https.addHeader("tokenid", SOLAX_TOKEN);
    
    // Create JSON payload
    String payload = "{\"wifiSn\":\"" + String(SOLAX_WIFI_SN) + "\"}";
    
    Serial.print("Request payload: ");
    Serial.println(payload);
    
    // Send POST request
    int httpCode = https.POST(payload);
    
    if (httpCode == HTTP_CODE_OK) {
      String response = https.getString();
      Serial.println("Response received:");
      Serial.println(response);
      
      // Parse JSON response
      bool parseSuccess = parseJsonResponse(response);
      
      if (parseSuccess) {
        success = true;
        retryCount = 0;  // Reset retry counter on success
        Serial.println("API call successful, retry counter reset");
      } else {
        Serial.println("API response parsing failed");
      }
      
    } else {
      Serial.printf("HTTP request failed, error: %s\n", https.errorToString(httpCode).c_str());
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("API Error");
      lcd.setCursor(0, 1);
      lcd.print("Code: ");
      lcd.print(httpCode);
      
      solarData.valid = false;
    }
    
    https.end();
  } else {
    Serial.println("Unable to connect to API");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connection");
    lcd.setCursor(0, 1);
    lcd.print("Failed");
    solarData.valid = false;
  }
  
  // Handle retry logic if API call failed
  if (!success) {
    if (retryCount < maxRetries) {
      int delayMs = retryDelays[retryCount];
      nextRetryTime = millis() + delayMs;
      isRetrying = true;
      retryCount++;
      
      Serial.printf("Will retry in %d seconds (attempt %d of %d)\n", 
                    delayMs / 1000, retryCount, maxRetries);
      
      lcd.setCursor(0, 2);
      lcd.print("Retry in ");
      lcd.print(delayMs / 1000);
      lcd.print("s");
      delay(2000);
    } else {
      Serial.println("Max retries reached. Will try again at next update interval.");
      retryCount = 0;  // Reset for next regular update
      isRetrying = false;
      
      lcd.setCursor(0, 2);
      lcd.print("Max retries");
      lcd.setCursor(0, 3);
      lcd.print("reached");
      delay(3000);
    }
  }
}

bool parseJsonResponse(String jsonString) {
  // Allocate JSON document
  DynamicJsonDocument doc(2048);
  
  // Parse JSON
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Parse Error");
    
    solarData.valid = false;
    delay(2000);
    return false;
  }
  
  // Check if request was successful
  bool success = doc["success"] | false;
  
  if (!success) {
    String exception = doc["exception"] | "Unknown error";
    Serial.print("API error: ");
    Serial.println(exception);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("API Error");
    lcd.setCursor(0, 1);
    lcd.print(exception.substring(0, 16));
    
    solarData.valid = false;
    delay(2000);
    return false;
  }
  
  // Extract data from result object
  JsonObject result = doc["result"];
  
  solarData.acPower = result["acpower"] | 0.0;
  solarData.yieldToday = result["yieldtoday"] | 0.0;
  solarData.yieldTotal = result["yieldtotal"] | 0.0;
  solarData.powerDC1 = result["powerdc1"] | 0.0;
  solarData.powerDC2 = result["powerdc2"] | 0.0;
  
  String statusCode = result["inverterStatus"] | "0";
  solarData.inverterStatus = mapInverterStatus(statusCode);
  
  solarData.valid = true;
  
  Serial.println("Data parsed successfully:");
  Serial.printf("  AC Power: %.0f W\n", solarData.acPower);
  Serial.printf("  Today: %.1f kWh\n", solarData.yieldToday);
  Serial.printf("  Total: %.1f kWh\n", solarData.yieldTotal);
  Serial.printf("  PV1: %.0f W\n", solarData.powerDC1);
  Serial.printf("  PV2: %.0f W\n", solarData.powerDC2);
  Serial.printf("  Status: %s\n", solarData.inverterStatus.c_str());
  
  return true;
}

void displayScreen1() {
  // Screen 1: Power Summary
  // Line 0: AC:xxxxx W
  // Line 1: Today:xx.x kWh
  // Line 2: Total:xxxx kWh
  // Line 3: Sts:StatusText
  
  lcd.clear();
  
  // Line 0: AC Power
  lcd.setCursor(0, 0);
  lcd.print("AC:");
  lcd.print((int)solarData.acPower);
  lcd.print(" W");
  
  // Line 1: Today's Yield
  lcd.setCursor(0, 1);
  lcd.print("Today:");
  lcd.print(solarData.yieldToday, 1);
  lcd.print(" kWh");
  
  // Line 2: Total Yield
  lcd.setCursor(0, 2);
  lcd.print("Total:");
  lcd.print((int)solarData.yieldTotal);
  lcd.print(" kWh");
  
  // Line 3: Status
  lcd.setCursor(0, 3);
  lcd.print("Sts:");
  lcd.print(solarData.inverterStatus.substring(0, 12));  // Max 12 chars for status
}

void displayScreen2() {
  // Screen 2: PV Details
  // Line 0: PV1:xxxxx W
  // Line 1: PV2:xxxxx W
  // Line 2: (blank)
  // Line 3: Sts:StatusText
  
  lcd.clear();
  
  // Line 0: PV1 Power
  lcd.setCursor(0, 0);
  lcd.print("PV1:");
  lcd.print((int)solarData.powerDC1);
  lcd.print(" W");
  
  // Line 1: PV2 Power
  lcd.setCursor(0, 1);
  lcd.print("PV2:");
  lcd.print((int)solarData.powerDC2);
  lcd.print(" W");
  
  // Line 2: Blank
  
  // Line 3: Status
  lcd.setCursor(0, 3);
  lcd.print("Sts:");
  lcd.print(solarData.inverterStatus.substring(0, 12));  // Max 12 chars for status
}

