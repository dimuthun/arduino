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

// Wrapper function to fix 16x4 LCD addressing issue
// 16x4 LCDs have different memory layout than 20x4, causing 4-char shift on rows 2 & 3
void setLcdCursor(int col, int row) {
  // For 16x4 displays: rows 2 and 3 need column offset correction
  if (row == 2 || row == 3) {
    lcd.setCursor(col - 4, row);
  } else {
    lcd.setCursor(col, row);
  }
}

// Data storage
struct SolarData {
  // Power data
  float acPower;
  float acVoltage;
  float acCurrent;
  float acFrequency;
  
  // PV data
  float powerDC1;
  float powerDC2;
  float voltageDC1;
  float voltageDC2;
  float currentDC1;
  float currentDC2;
  
  // Yield data
  float yieldToday;
  float yieldTotal;
  
  // Grid import/export
  float exportPower;
  float totalExportEnergy;
  float totalImportEnergy;
  
  // System info
  float inverterTemperature;
  String inverterStatus;
  bool isLocalMode;  // true = local API, false = cloud API
  bool valid;
} solarData;

// Timing variables
unsigned long lastApiUpdate = 0;
unsigned long lastScreenSwitch = 0;
int currentScreen = 0;  // 0-3: Four screens rotation

// Retry logic variables
int retryCount = 0;
unsigned long nextRetryTime = 0;
bool isRetrying = false;
const int retryDelays[] = {5000, 10000, 20000};  // Retry delays: 5s, 10s, 20s
const int maxRetries = 3;

// Function to read operation mode from jumper pin
bool readOperationMode() {
  // Read pin state: LOW (connected to GND) = Cloud mode
  //                  HIGH (pull-up, open) = Local mode
  return digitalRead(MODE_SELECT_PIN) == HIGH;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSolaX Cloud/Local Data Display");
  Serial.println("==============================");
  
  // Configure mode select pin
  pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
  
  // Read operation mode
  bool isLocal = readOperationMode();
  solarData.isLocalMode = isLocal;
  
  Serial.print("Operation Mode: ");
  Serial.println(isLocal ? "LOCAL (Direct)" : "CLOUD (Internet)");
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Display startup message
  setLcdCursor(0, 0);
  lcd.print("SolaX Display");
  setLcdCursor(0, 1);
  lcd.print("Mode: ");
  lcd.print(isLocal ? "LOCAL" : "CLOUD");
  setLcdCursor(0, 2);
  lcd.print("Initializing...");
  delay(2000);
  
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
    setLcdCursor(0, 0);
    lcd.print("WiFi Lost");
    setLcdCursor(0, 1);
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
    currentScreen = (currentScreen + 1) % 4;  // Cycle through 4 screens (0-3)
    lastScreenSwitch = currentMillis;
  }
  
  // Display current screen
  if (solarData.valid) {
    switch (currentScreen) {
      case 0:
        displayScreen1();  // Power Summary
        break;
      case 1:
        displayScreen2();  // AC Details
        break;
      case 2:
        displayScreen3();  // PV Details
        break;
      case 3:
        displayScreen4();  // System Info
        break;
    }
  }
  
  delay(1000);  // Small delay to prevent excessive loop execution
}

void fetchLocalSolarData() {
  Serial.println("Fetching data from Local API...");
  
  if (retryCount > 0) {
    Serial.printf("Retry attempt %d of %d\n", retryCount, maxRetries);
  }
  
  lcd.clear();
  setLcdCursor(0, 0);
  lcd.print("Updating (Local)");
  if (retryCount > 0) {
    setLcdCursor(0, 1);
    lcd.print("Retry ");
    lcd.print(retryCount);
    lcd.print("/");
    lcd.print(maxRetries);
  }
  
  WiFiClient client;
  HTTPClient http;
  bool success = false;
  
  if (http.begin(client, LOCAL_API_URL)) {
    // Add header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Create POST data
    String postData = "%3FoptType=ReadRealTimeData&pwd=" + String(LOCAL_API_PASSWORD);
    
    Serial.print("Request to: ");
    Serial.println(LOCAL_API_URL);
    
    // Send POST request
    int httpCode = http.POST(postData);
    
    if (httpCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpCode);
      String response = http.getString();
      Serial.println("Response received:");
      Serial.println(response);
      
      // Parse JSON response
      bool parseSuccess = parseLocalJsonResponse(response);
      
      if (parseSuccess) {
        success = true;
        retryCount = 0;  // Reset retry counter on success
        Serial.println("Local API call successful");
      } else {
        Serial.println("Local API response parsing failed");
      }
      
    } else {
      Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
      
      lcd.clear();
      setLcdCursor(0, 0);
      lcd.print("API Error");
      setLcdCursor(0, 1);
      lcd.print("Code: ");
      lcd.print(httpCode);
      
      solarData.valid = false;
    }
    
    http.end();
  } else {
    Serial.println("Unable to connect to Local API");
    lcd.clear();
    setLcdCursor(0, 0);
    lcd.print("Connection");
    setLcdCursor(0, 1);
    lcd.print("Failed (Local)");
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
      
      setLcdCursor(0, 2);
      lcd.print("Retry in ");
      lcd.print(delayMs / 1000);
      lcd.print("s");
      delay(2000);
    } else {
      Serial.println("Max retries reached. Will try again at next update interval.");
      retryCount = 0;  // Reset for next regular update
      isRetrying = false;
      
      setLcdCursor(0, 2);
      lcd.print("Max retries");
      setLcdCursor(0, 3);
      lcd.print("reached");
      delay(3000);
    }
  }
}

void connectWiFi() {
  // Read current operation mode
  bool isLocal = readOperationMode();
  solarData.isLocalMode = isLocal;
  
  const char* ssid;
  const char* password;
  const char* modeText;
  
  if (isLocal) {
    ssid = LOCAL_WIFI_SSID;
    password = LOCAL_WIFI_PASSWORD;
    modeText = "LOCAL";
  } else {
    ssid = WIFI_SSID;
    password = WIFI_PASSWORD;
    modeText = "CLOUD";
  }
  
  Serial.print("Connecting to WiFi (");
  Serial.print(modeText);
  Serial.print("): ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  lcd.clear();
  setLcdCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print(modeText);
  setLcdCursor(0, 1);
  lcd.print("Connecting...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    setLcdCursor(0, 0);
    lcd.print(modeText);
    lcd.print(" Connected");
    setLcdCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\nWiFi connection failed!");
    lcd.clear();
    setLcdCursor(0, 0);
    lcd.print("WiFi Failed!");
    setLcdCursor(0, 1);
    lcd.print("Check config.h");
    delay(5000);
  }
}

void fetchCloudSolarData() {
  Serial.println("Fetching data from SolaX Cloud API...");
  
  if (retryCount > 0) {
    Serial.printf("Retry attempt %d of %d\n", retryCount, maxRetries);
  }
  
  lcd.clear();
  setLcdCursor(0, 0);
  lcd.print("Updating (Cloud)");
  if (retryCount > 0) {
    setLcdCursor(0, 1);
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
      setLcdCursor(0, 0);
      lcd.print("API Error");
      setLcdCursor(0, 1);
      lcd.print("Code: ");
      lcd.print(httpCode);
      
      solarData.valid = false;
    }
    
    https.end();
  } else {
    Serial.println("Unable to connect to Cloud API");
    lcd.clear();
    setLcdCursor(0, 0);
    lcd.print("Connection");
    setLcdCursor(0, 1);
    lcd.print("Failed (Cloud)");
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
      
      setLcdCursor(0, 2);
      lcd.print("Retry in ");
      lcd.print(delayMs / 1000);
      lcd.print("s");
      delay(2000);
    } else {
      Serial.println("Max retries reached. Will try again at next update interval.");
      retryCount = 0;  // Reset for next regular update
      isRetrying = false;
      
      setLcdCursor(0, 2);
      lcd.print("Max retries");
      setLcdCursor(0, 3);
      lcd.print("reached");
      delay(3000);
    }
  }
}

void fetchSolarData() {
  // Read operation mode from jumper pin
  bool isLocal = readOperationMode();
  solarData.isLocalMode = isLocal;
  
  // Route to appropriate fetch function
  if (isLocal) {
    fetchLocalSolarData();
  } else {
    fetchCloudSolarData();
  }
}

bool parseLocalJsonResponse(String jsonString) {
  // Allocate JSON document for local API array format
  DynamicJsonDocument doc(2048);
  
  // Parse JSON
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    
    lcd.clear();
    setLcdCursor(0, 0);
    lcd.print("Parse Error");
    
    solarData.valid = false;
    delay(2000);
    return false;
  }
  
  // Check if Data array exists
  if (!doc.containsKey("Data")) {
    Serial.println("No Data array in response");
    solarData.valid = false;
    return false;
  }
  
  JsonArray data = doc["Data"];
  
  // Extract and scale data according to local API format
  solarData.acVoltage = data[0].as<float>() / 10.0;
  solarData.acCurrent = data[1].as<float>() / 10.0;
  solarData.acFrequency = data[2].as<float>() / 100.0;
  solarData.acPower = data[3].as<float>();
  
  solarData.voltageDC1 = data[4].as<float>() / 10.0;
  solarData.voltageDC2 = data[5].as<float>() / 10.0;
  
  solarData.currentDC1 = data[8].as<float>() / 10.0;
  solarData.currentDC2 = data[9].as<float>() / 10.0;
  
  solarData.powerDC1 = data[13].as<float>();
  solarData.powerDC2 = data[14].as<float>();
  
  solarData.yieldTotal = data[19].as<float>() / 10.0;
  solarData.yieldToday = data[21].as<float>() / 10.0;
  
  solarData.inverterTemperature = data[39].as<float>();
  
  solarData.exportPower = (float)data[48].as<int16_t>();  // signed value
  solarData.totalExportEnergy = data[50].as<float>() / 100.0;
  solarData.totalImportEnergy = data[52].as<float>() / 100.0;
  
  solarData.inverterStatus = "Local";  // Local mode doesn't provide detailed status
  solarData.valid = true;
  solarData.isLocalMode = true;
  
  Serial.println("Local data parsed successfully:");
  Serial.printf("  AC: %.1f V, %.1f A, %.2f Hz, %.0f W\n", 
                solarData.acVoltage, solarData.acCurrent, 
                solarData.acFrequency, solarData.acPower);
  Serial.printf("  PV1: %.1f V, %.1f A, %.0f W\n", 
                solarData.voltageDC1, solarData.currentDC1, solarData.powerDC1);
  Serial.printf("  PV2: %.1f V, %.1f A, %.0f W\n", 
                solarData.voltageDC2, solarData.currentDC2, solarData.powerDC2);
  Serial.printf("  Today: %.1f kWh, Total: %.1f kWh\n", 
                solarData.yieldToday, solarData.yieldTotal);
  Serial.printf("  Temp: %.0f C\n", solarData.inverterTemperature);
  Serial.printf("  Export: %.0f W, Import: %.2f kWh, Export: %.2f kWh\n", 
                solarData.exportPower, solarData.totalImportEnergy, 
                solarData.totalExportEnergy);
  
  return true;
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
    setLcdCursor(0, 0);
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
    setLcdCursor(0, 0);
    lcd.print("API Error");
    setLcdCursor(0, 1);
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
  solarData.isLocalMode = false;
  
  // Initialize fields not available in cloud API
  solarData.acVoltage = 0;
  solarData.acCurrent = 0;
  solarData.acFrequency = 0;
  solarData.voltageDC1 = 0;
  solarData.voltageDC2 = 0;
  solarData.currentDC1 = 0;
  solarData.currentDC2 = 0;
  solarData.inverterTemperature = 0;
  solarData.exportPower = 0;
  solarData.totalExportEnergy = 0;
  solarData.totalImportEnergy = 0;
  
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
  // Line 0: AC: xxxxx W  [M]
  // Line 1: PV: xxxxx W
  // Line 2: Today: xx.x kWh
  // Line 3: Total: xxxx kWh
  
  lcd.clear();
  
  // Line 0: AC Power with mode indicator
  setLcdCursor(0, 0);
  lcd.print("AC:");
  lcd.print((int)solarData.acPower);
  lcd.print(" W");
  //setLcdCursor(15, 0);
  //lcd.print(solarData.isLocalMode ? "L" : "C");
  
  // Line 1: Total PV Power
  setLcdCursor(0, 1);
  lcd.print("PV:");
  lcd.print((int)(solarData.powerDC1 + solarData.powerDC2));
  lcd.print(" W");
  
  // Line 2: Today's Yield
  setLcdCursor(0, 2);
  lcd.print("Today:");
  lcd.print(solarData.yieldToday, 1);
  lcd.print(" kWh");
  
  // Line 3: Total Yield
  setLcdCursor(0, 3);
  lcd.print("Total:");
  lcd.print((int)solarData.yieldTotal);
  lcd.print(" kWh");
}

void displayScreen2() {
  // Screen 2: AC Details (only available in local mode)
  // Line 0: AC Volt:xxx.xV [M]
  // Line 1: AC Curr: xx.xA
  // Line 2: AC Freq:xx.xxHz
  // Line 3: AC Pwr: xxxx W
  
  lcd.clear();
  
  if (solarData.isLocalMode) {
    // Line 0: AC Voltage with mode indicator
    setLcdCursor(0, 0);
    lcd.print("AC Volt:");
    lcd.print(solarData.acVoltage, 1);
    lcd.print("V");
    //setLcdCursor(15, 0);
    //lcd.print("L");
    
    // Line 1: AC Current
    setLcdCursor(0, 1);
    lcd.print("AC Curr:");
    lcd.print(solarData.acCurrent, 1);
    lcd.print("A");
    
    // Line 2: AC Frequency
    setLcdCursor(0, 2);
    lcd.print("AC Freq:");
    lcd.print(solarData.acFrequency, 2);
    lcd.print("Hz");
    
    // Line 3: AC Power
    setLcdCursor(0, 3);
    lcd.print("AC Pwr:");
    lcd.print((int)solarData.acPower);
    lcd.print(" W");
  } else {
    // Cloud mode - show status instead
    setLcdCursor(0, 0);
    lcd.print("AC Details");
    setLcdCursor(15, 0);
    lcd.print("C");
    setLcdCursor(0, 1);
    lcd.print("Not available");
    setLcdCursor(0, 2);
    lcd.print("in Cloud mode");
    setLcdCursor(0, 3);
    lcd.print("Sts:");
    lcd.print(solarData.inverterStatus.substring(0, 11));
  }
}

void displayScreen3() {
  // Screen 3: PV Details
  // Line 0: PV1: 123V  1.2A
  // Line 1:       1234 W
  // Line 2: PV2: 123V  1.2A
  // Line 3:       1234 W
  
  lcd.clear();
  
  if (solarData.isLocalMode) {
    // Line 0: PV1 Voltage and Current
    setLcdCursor(0, 0);
    lcd.print("PV1:");
    lcd.print((int)solarData.voltageDC1);
    lcd.print("V ");
    
    // Right-align current value (format as X.XA with padding)
    setLcdCursor(10, 0);
    if (solarData.currentDC1 < 10) lcd.print(" ");
    lcd.print(solarData.currentDC1, 1);
    lcd.print("A");
    
    // Line 1: PV1 Power
    setLcdCursor(0, 1);
    lcd.print("      ");
    lcd.print((int)solarData.powerDC1);
    lcd.print(" W");
    
    // Line 2: PV2 Voltage and Current
    setLcdCursor(0, 2);
    lcd.print("PV2:");
    lcd.print((int)solarData.voltageDC2);
    lcd.print("V ");
    
    // Right-align current value
    setLcdCursor(10, 2);
    if (solarData.currentDC2 < 10) lcd.print(" ");
    lcd.print(solarData.currentDC2, 1);
    lcd.print("A");
    
    // Line 3: PV2 Power
    setLcdCursor(0, 3);
    lcd.print("      ");
    lcd.print((int)solarData.powerDC2);
    lcd.print(" W");
  } else {
    // Cloud mode - simpler display
    // Line 0: PV1 Power
    setLcdCursor(0, 0);
    lcd.print("PV1: ");
    lcd.print((int)solarData.powerDC1);
    lcd.print(" W");
    
    // Line 1: Status
    setLcdCursor(0, 1);
    lcd.print("Sts:");
    lcd.print(solarData.inverterStatus.substring(0, 12));
    
    // Line 2: PV2 Power
    setLcdCursor(0, 2);
    lcd.print("PV2: ");
    lcd.print((int)solarData.powerDC2);
    lcd.print(" W");
    
    // Line 3: Blank or additional info
  }
}

void displayScreen4() {
  // Screen 4: System Info (only available in local mode)
  // Line 0: Temp: xx C      [M]
  // Line 1: Export: xxxx W
  // Line 2: Import:xx.x kWh
  // Line 3: Export:xx.x kWh
  
  lcd.clear();
  
  if (solarData.isLocalMode) {
    // Line 0: Temperature with mode indicator
    setLcdCursor(0, 0);
    lcd.print("Temp:");
    lcd.print((int)solarData.inverterTemperature);
    lcd.print(" C");
    //setLcdCursor(15, 0);
    //lcd.print("L");
    
    // Line 1: Export Power
    setLcdCursor(0, 1);
    lcd.print("Export:");
    lcd.print((int)solarData.exportPower);
    lcd.print(" W");
    
    // Line 2: Total Import Energy
    setLcdCursor(0, 2);
    lcd.print("Import:");
    lcd.print(solarData.totalImportEnergy, 1);
    lcd.print("kWh");
    
    // Line 3: Total Export Energy
    setLcdCursor(0, 3);
    lcd.print("Export:");
    lcd.print(solarData.totalExportEnergy, 1);
    lcd.print("kWh");
  } else {
    // Cloud mode - show status
    setLcdCursor(0, 0);
    lcd.print("System Info");
    setLcdCursor(15, 0);
    lcd.print("C");
    setLcdCursor(0, 1);
    lcd.print("Not available");
    setLcdCursor(0, 2);
    lcd.print("in Cloud mode");
    setLcdCursor(0, 3);
    lcd.print("Sts:");
    lcd.print(solarData.inverterStatus.substring(0, 11));
  }
}


