//PIN D2 -> SDA
//PIN D1 -> SCL
//PIN G -> Ground
//PIN VIN -> 5V

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#define TYPE 4
#define MIN_DATA_LENGTH 200
#define MAX_DATA_LENGTH 200
#define MIN_INFO_LENGTH 10
#define MAX_INFO_LENGTH 10

bool isLocalMode = true;

LiquidCrystal_I2C lcd(0x27, 16, 4);  // Set the LCD address to 0x27 for a 16 chars and 2 line display
int displayWidth = 16;
int row = 0;
const char *ssidClould = "Dimuthu Home";
const char *passwordClould = "Zone@123";
const char *serverNameClould = "https://eu.solaxcloud.com/proxyApp/proxy/api/getRealtimeInfo.do?tokenId=202406120951433408735310&sn=SR8BVQYEHR";

const char *ssidLocal = "Wifi_SR8BVQYEHR";
const char *passwordLocal = "";
const char *serverNameLocal = "http://192.168.10.10";



// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
unsigned long timerDelay = 35000;

void setup() {
  Serial.begin(115200);

  initLCD();
  delay(500);
  initWiFi(ssidLocal, passwordLocal);

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      // client.setInsecure();
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverNameLocal);
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String httpRequestData = "%3FoptType=ReadRealTimeData&pwd=SR8BVQYEHR";
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        Serial.println("Sucess!");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        parseData(payload);


      } else {
        Serial.println("Failed!");
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
        initWiFi(ssidLocal, passwordLocal);
      }
    }
    lastTime = millis();
  }
}

void initWiFi(const char *ssid, const char *password) {
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

void displayClouldData(String payload) {
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
}

void displayFailedResponse() {
  delay(1000);
  lcd.setCursor(15, 0);
  lcd.print("!");
}


bool parseData(String json) {
 Serial.println(json);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // if (doc["type"] != TYPE) {
  //   Serial.println("Invalid type");
  //   return false;
  // }

  JsonArray data = doc["Data"];
  // if (data.size() != MIN_DATA_LENGTH) {
  //   Serial.println("Invalid data length");
  //   return false;
  // }

   float acVoltage = data[0].as<float>() / 10.0; // ok
  float acOutputCurrent = data[1].as<float>() / 10.0; //ok
 float acFrequency = data[2].as<float>()/100.0; //ok
 float acPower = data[3].as<float>();
 float pv1Voltage = data[4].as<float>() / 10.0;
 float pv2Voltage = data[5].as<float>() / 10.0;
 float a = data[6].as<float>() / 10.0;
 float b = data[7].as<float>();
 float pv1Current = data[8].as<float>()/10.0;
 float pv2Current = data[9].as<float>()/10.0 ;
 float c = data[10].as<float>() ;
 float d = data[11].as<float>() ;
 float e = data[12].as<float>();
 float pv1Power = data[13].as<float>();
float pv2Power = data[14].as<float>();
float f = data[15].as<float>();
float g = data[16].as<float>();
float h = data[17].as<float>();
float i = data[18].as<float>();
float inverterTotal = data[19].as<float>()/10.0;
float inverterToday = data[21].as<float>()/10.0;

 float inverterTemperature = data[39].as<float>();
 float exportedPower = (float)data[48].as<int16_t>();  // signed value
 float totalExportEnergy = data[50].as<float>() / 100.0;
 float totalImportEnergy = data[52].as<float>() / 100.0;
Serial.println("");
  Serial.print("acVoltage:"); Serial.println(acVoltage);
  Serial.print("acOutputCurrent:"); Serial.println(acOutputCurrent);
  Serial.print("acFrequency:"); Serial.println(acFrequency);
  Serial.print("acPower:"); Serial.println(acPower);
  Serial.print("pv1Voltage:"); Serial.println(pv1Voltage);
  Serial.print("pv2Voltage:"); Serial.println(pv2Voltage);
  Serial.print("pv1Current:"); Serial.println(pv1Current);
  Serial.print("pv2Current:"); Serial.println(pv2Current);
  Serial.print("pv1Power:"); Serial.println(pv1Power);
  Serial.print("pv2Power:"); Serial.println(pv2Power);
  Serial.print("inverterTotal:"); Serial.println(inverterTotal);
  Serial.print("inverterToday:"); Serial.println(inverterToday);

  Serial.print("f:"); Serial.println(f);
   //Serial.print("pv1Power:"); Serial.println(pv1Power);
  Serial.print("a:"); Serial.println(a);
   Serial.print("b:"); Serial.println(b);
  Serial.print("c:"); Serial.println(c);
   Serial.print("d:"); Serial.println(d);
 Serial.print("e:"); Serial.println(e);
  Serial.print("f:"); Serial.println(f);   
 Serial.print("g:"); Serial.println(g);
  Serial.print("h:"); Serial.println(h);
    Serial.print("i:"); Serial.println(i);
     // Serial.print("j:"); Serial.println(j);
 // Serial.print("pv2Power:"); Serial.println(pv2Power);
  Serial.print("acFrequency:"); Serial.println(acFrequency);







//  Serial.println("acOutputCurrent:" + acOutputCurrent);
//   Serial.println("acOutputPower:" + acOutputPower);
//    Serial.println("pv1Voltage:" + pv1Voltage);
//     Serial.println("pv2Voltage:" + pv2Voltage);
//      Serial.println("pv1Current:" + pv1Current);
//       Serial.println("pv2Current:" + pv2Current);
//        Serial.println("pv1Power:" + pv1Power);
//         Serial.println("pv1Power:" + pv1Power);
//          Serial.println("acFrequency:" + acFrequency);



  return true;
}

