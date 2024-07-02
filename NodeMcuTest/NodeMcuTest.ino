#include <ESP8266WiFi.h>
#include <HTTPClient.h>;
#include<ArduinoJson.h>;


const char* ssid ="Dimuthu Home";
const char* password ="Zone@123";

void setup()
{
 Serial.begin(115200);
 Wifi.begin(ssid, password);
 Serial.print("Connecting to wifi");

while(WiFi.status() != WL_CONNECTED){
Serial.print(".");
delay(500);

Serial.println("\nConnected to Wi Fi");
Serial.print("IP:")
Serial.println(WiFi.localIP());
}


void loop()
{
if(WiFi.status() == WL_CONNECTED)
{
  Serial.println("You can try to ping me");
  delay(5000);

  HTTPClient client;
  client.begin("http:jsonplaceholder.typicode.com/comments?id=5");
  int httpCode = client.GET();
  if(httpCode>0)
  {
    String payload = client.getString();
    Serial.println("StatusCode:" + String(httpCode));
    Serial.println(payload);
  }
  else
  {
    Serial.println("Error sending the request. Code:" + String(httpCode));
  }

}
else
{
  Serial.println("Connection lost");
}

delay(10000);

}

}