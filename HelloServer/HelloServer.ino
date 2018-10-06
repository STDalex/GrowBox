#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <stdio.h>
#include <U8x8lib.h>
#include <string>
#include <sstream>
#include <limits>
#include <iomanip>
#include "DHT.h"
using namespace std;


#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

DHT dht(13, DHT22);

//const char* ssid = "AndroidAPA5A0";
//const char* password = "hbmt2459";
const char* ssid = "TP-LINK_7C1B64";
const char* password = "12161003";

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

WiFiServer server(80);

const int led = 25;
String LED_State = "off";

String header;

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 0, "Hello!");
  
  dht.begin();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
    u8x8.drawString(0, 0, "scaning...");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  u8x8.drawString(0, 0, WiFi.localIP().toString().c_str());

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {

  float temp = (temprature_sens_read() - 32) / 1.8;
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h, false);
  
  stringstream stream_internal_temp;
  stringstream stream_dht22_t;
  stringstream stream_dht22_h;
  
  stream_internal_temp << fixed << "int t = "<< setprecision(2) << temp << " C";
  stream_dht22_t << fixed << "ext t = "<< setprecision(2) << hic << " C";
  stream_dht22_h << fixed << "hum = "<< setprecision(2) << h << " %";
  
  string s_int_temp = stream_internal_temp.str();
  string s_dht22_t = stream_dht22_t.str();
  string s_dht22_h = stream_dht22_h.str();
  
  u8x8.drawString(0, 1, s_int_temp.c_str());
  u8x8.drawString(0, 2, s_dht22_t.c_str());
  u8x8.drawString(0, 3, s_dht22_h.c_str());


  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /LED/on") >= 0) {
              LED_State = "on";
              digitalWrite(led, HIGH);
            } else if (header.indexOf("GET /LED/off") >= 0) {
              LED_State = "off";
              digitalWrite(led, LOW);
            } 
            
            
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <meta http-equiv=\"refresh\" content=\"30\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            client.println("<body><h1>GROW BOX</h1>");
            
            
            client.println("<p>LED State " + LED_State + "</p>");
            if (LED_State=="off") {
              client.println("<p><a href=\"/LED/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/LED/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            client.println("</body></html>");
            
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
