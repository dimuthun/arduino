#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);  // Set the LCD address to 0x27 for a 16 chars and 2 line display
int displayWidth = 16;
int row = 0;
const char *ssid = "Dimuthu Home";
const char *password = "Zone@123";
const char *serverName = "https://eu.solaxcloud.com/proxyApp/proxy/api/getRealtimeInfo.do?tokenId=202406120951433408735310&sn=SR8BVQYEHR";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 35000;

void setup() {
  Serial.begin(115200);
  
  initLCD();
  delay(500);
  initWiFi();

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) 
  {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClientSecure client;
      HTTPClient http;
      client.setInsecure();
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String httpRequestData = "";
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        DynamicJsonDocument doc(4000);
        deserializeJson(doc, payload);
        JsonObject obj = doc.as<JsonObject>();

        lcd.clear();
        delay(500);
        //Line 1
        row = 0;
        String acPower = obj["result"]["acpower"].as<String>() + " W";
        lcd.setCursor((displayWidth - strlen(acPower.c_str())) / 2, row);  // Set the cursor to the first column and first row
        lcd.print(acPower);
        //Line2
        row = 1;
        String p1Andp2Power = obj["result"]["powerdc1"].as<String>() + "W + " + obj["result"]["powerdc2"].as<String>() + "W";
        lcd.setCursor((displayWidth - strlen(p1Andp2Power.c_str())) / 2, row);
        lcd.print(p1Andp2Power);
        //Line3
        lcd.setCursor(-4, 2);
        lcd.print("Today:" + obj["result"]["yieldtoday"].as<String>() + " kWh");
        //Line4
        lcd.setCursor(-4, 3);
        lcd.print("Total:" + obj["result"]["yieldtotal"].as<String>() + " kWh");

      } else {
       // lcd.clear();
        delay(1000);
        lcd.setCursor(15, 0);
        lcd.print("!");
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    } else {
      Serial.print("WiFi Disconnected!");
      lcd.setCursor(0, 0);
      lcd.print("No WiFi");
        delay(30000);
      if (WiFi.status() == WL_CONNECTED) {
        initWiFi();
      }
    }
    lastTime = millis();
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  //LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    lcd.print(".");
    delay(1000);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(2, 1);
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());
}

void initLCD() {
  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on the backlight
  lcd.clear();      // Clear the LCD screen
  lcd.setCursor(0, 0);
}
