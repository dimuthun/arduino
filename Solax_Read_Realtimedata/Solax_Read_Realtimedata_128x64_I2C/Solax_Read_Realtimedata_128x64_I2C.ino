/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  
  Code compatible with ESP8266 Boards Version 3.0.0 or above 
  (see in Tools > Boards > Boards Manager > ESP8266)
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "Dimuthu Home";
const char* password = "Zone@123";

  char *today = "Today: ";
  char *total = "Total: ";
  char *kwh = " kWh";
  char *pv1 = "PV1: ";
  char *pv2 = "Pv2: ";
  char *w = " W";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//Your Domain name with URL path or IP address with path
String serverName = "https://eu.solaxcloud.com/proxyApp/proxy/api/getRealtimeInfo.do?tokenId=202406120951433408735310&sn=SR8BVQYEHR";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 20000;

void setup() {
  Serial.begin(115200); 

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  displayInit();
  displayText("Initalizing");
  delay(1000);

  WiFi.begin(ssid, password);
  displayText("Connecting");

 // display.clearDisplay();
  //display.write("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    displayAppendText(".");
  }


  displayText("Connected!");
  displayAppendText(WiFi.localIP().toString(), 0, 12, 1);
  delay(500);
  
 // display.write(WiFi.localIP().toString().c_str());
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){

      displayText("Querying Cloud...", 0, 0, 1);
      WiFiClientSecure client;
      HTTPClient http;
      String serverPath = serverName;
      client.setInsecure();
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath.c_str());
  
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
        
      // Send HTTP GET request

            // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      String httpRequestData = "";           
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        DynamicJsonDocument doc(4000);

  // You can use a String as your JSON input.
  // WARNING: the string in the input  will be duplicated in the JsonDocument.
        String input = payload;
    //  "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";
        deserializeJson(doc, input);
        JsonObject obj = doc.as<JsonObject>();
        Serial.println(obj["result"]["acpower"].as<String>());
        
        writeInScreen(obj["result"]["acpower"].as<String>(),
                      obj["result"]["powerdc1"].as<String>(),
                      obj["result"]["powerdc2"].as<String>(),
                      obj["result"]["yieldtoday"].as<String>(),
                      obj["result"]["yieldtotal"].as<String>());
 // Serial.println(yieldtoday);
  // Serial.println(yieldtotal);

      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      displayText("WiFi Disconnected!");
    }
    lastTime = millis();
  }
}


void displayInit(){
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.write("Init....");  
  display.display();
}

void displayText(String text){
  display.clearDisplay();
  display.setCursor(0, 0); 
  display.write(text.c_str());  
  display.display();
}

void displayText(String text, int positionX, int positionY, int textSize){
  display.clearDisplay();
  display.setCursor(positionX, positionY);
  display.setTextSize(textSize); 
  display.write(text.c_str());  
  display.display();
}

void displayAppendText(String text){
  display.write(text.c_str());  
  display.display();
}

void displayAppendText(String text, int positionX, int positionY, int textSize){
  display.setCursor(positionX, positionY);
  display.setTextSize(textSize); 
  display.write(text.c_str());  
  display.display();
}

void displayClear(){
  display.clearDisplay(); 
  display.display();
}

void writeInScreen(String acPower, String pv1Power, String pv2Power, String dailyYield, String totalYield) {

  
  display.clearDisplay();
  delay(1000);
        // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true); 

  //Data 1
  display.setTextSize(2);         // Use full 256 char 'Code Page 437' font
  display.write((acPower + w).c_str());
  display.setTextSize(1);
  display.setCursor(0, 16);

  dailyYield = today + dailyYield + kwh;
  totalYield = total + totalYield + kwh;
  display.write(dailyYield.c_str());
  display.setCursor(0, 24);
  display.write(totalYield.c_str());
  //display.write("test");
  display.display();
  delay(timerDelay/2);

 //Small delay
  display.clearDisplay();
  delay(500);

 //Data 2
  display.setCursor(0, 0);  
  display.setTextSize(2);         // Use full 256 char 'Code Page 437' font
  display.write((acPower + w).c_str());
  display.setTextSize(1);
  display.setCursor(0, 16);

  pv1Power = pv1 + pv1Power + w;
  pv2Power = pv2 + pv2Power + w;
  display.write(pv1Power.c_str());
  display.setCursor(0, 24);
  display.write(pv2Power.c_str());
  //display.write("test");
  display.display();
  delay(5000);

  
}