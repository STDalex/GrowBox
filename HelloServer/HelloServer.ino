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
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

#define led 25

DHTesp dht;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 10800);

WiFiServer server(80);
String header; // header for contain params for web client
//const char* ssid = "AndroidAPA5A0";
//const char* password = "hbmt2459";
const char* ssid = "TP-LINK_7C1B64";
const char* password = "12161003";

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

String LED_State = "off";
int m_phase; // growth stage 
int m_phase_state [4];
bool m_is_new_day = true;
int m_start_growth; // 0 - stop growth, 1 - start growth

Preferences preferences;

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 0, "Hello!");

  dht.setup(13, DHTesp::DHT22);
  timeClient.begin();
  preferences.begin("m_phase-save", false);
  m_phase = preferences.getInt("m_phase", 0);
  m_phase_state[0] = preferences.getInt("m_phase_state_0", 0);
  m_phase_state[1] = preferences.getInt("m_phase_state_1", 0);
  m_phase_state[2] = preferences.getInt("m_phase_state_2", 0);
  m_phase_state[3] = preferences.getInt("m_phase_state_3", 0);
  m_start_growth = preferences.getInt("m_start_growth", 0);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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
  
  float uC_temp = (temprature_sens_read() - 32) / 1.8;
  float dht22_temp = dht.getTemperature();
  float dht22_hum = dht.getHumidity();
  String s_dht22_temp = FloatToString(dht22_temp,2).c_str();
  String s_dht22_hum = FloatToString(dht22_hum,2).c_str();

  check_time();
  String s_count_phase_0 = FloatToString(m_phase_state[0],0).c_str();
  String s_count_phase_1 = FloatToString(m_phase_state[1],0).c_str();
  String s_count_phase_2 = FloatToString(m_phase_state[2],0).c_str();
  String s_count_phase_3 = FloatToString(m_phase_state[3],0).c_str();

  update_oled(uC_temp, dht22_temp, dht22_hum);

  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
     //   Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /action_set_m_phase?m_phase=0") >= 0 ) {
              m_phase = 0;
              preferences.putInt("m_phase", m_phase);
            }
            else if (header.indexOf("GET /action_set_m_phase?m_phase=1") >= 0 ) {
              m_phase = 1;
              preferences.putInt("m_phase", m_phase);
            }            
            else if (header.indexOf("GET /action_set_m_phase?m_phase=2") >= 0 ) {
              m_phase = 2;
              preferences.putInt("m_phase", m_phase); 
            }
            else if (header.indexOf("GET /action_set_m_phase?m_phase=3") >= 0 ) {
              m_phase = 3;
              preferences.putInt("m_phase", m_phase); 
            }
            if (header.indexOf("GET /new?psw=121") >= 0 ) {
              if (m_start_growth == 0){
                m_start_growth = 1;
                preferences.putInt("m_start_growth", 1);
              }
              else {
                m_start_growth = 0;
                preferences.putInt("m_start_growth", 0);
                for (int i =0; i<sizeof(m_phase_state); i++)
                  m_phase_state[i] = 0;
                preferences.putInt("m_phase_state_0", 0);
                preferences.putInt("m_phase_state_1", 0);
                preferences.putInt("m_phase_state_2", 0);
                preferences.putInt("m_phase_state_3", 0);
                m_phase = 0;
                preferences.putInt("m_phase", m_phase);
                setLED("off");
                break;
              }
            }
                     
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <meta http-equiv=\"refresh\" content=\"20; url=/\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white;");
            client.println("text-decoration: none; margin: 2px; padding: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            client.println("<body><h1><a href=\"/\">GROWTH BOX</a></h1>");

            client.println("<form action=\"/new\">\n");
            if (m_start_growth == 0)            
              client.println("<input type=\"submit\" class=\"button\" value=\"START GROWTH\">\n");
            else
              client.println("<input type=\"submit\" class=\"button button2\" value=\"STOP GROWTH\">\n");
            client.println("<input type=\"password\" name=\"psw\">\n");
            client.println("</form>");
            
            client.println("<h3> Select growth stage</h3>");
            client.println("<form action=\"/action_set_m_phase\">");
            if (m_phase==0) {
            client.println("<input type=\"radio\" name=\"m_phase\" value=0 checked> Germination<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=1> Seedling<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=2> Vegetative<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=3> Flowering<br>");
            }
            if (m_phase==1) {
            client.println("<input type=\"radio\" name=\"m_phase\" value=0> Germination<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=1 checked> Seedling<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=2> Vegetative<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=3> Flowering<br>");
            }
            if (m_phase==2) {
            client.println("<input type=\"radio\" name=\"m_phase\" value=0> Germination<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=1> Seedling<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=2 checked> Vegetative<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=3> Flowering<br>");
            }
            if (m_phase==3) {
            client.println("<input type=\"radio\" name=\"m_phase\" value=0> Germination<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=1> Seedling<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=2> Vegetative<br>");
            client.println("<input type=\"radio\" name=\"m_phase\" value=3 checked> Flowering<br>");
            }    
            client.println("<input type=\"submit\" value=\"Submit\">");
            client.println("</form><br>");

            client.println("<h3> Curent state</h3>");
            client.println("<p>LED State " + LED_State + "</p>");
            client.println("<p>Germination " + s_count_phase_0 + " days</p>");
            client.println("<p>Seedling " + s_count_phase_1 + " days</p>");
            client.println("<p>Vegetative " + s_count_phase_2 + " days</p>");
            client.println("<p>Flowering " + s_count_phase_3 + " days</p>");
            client.println("<p>Temperature is " + s_dht22_temp + " C</p>");
            client.println("<p>Humidity is " + s_dht22_hum + " %</p>");

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

void update_oled(float uC_temp, float dht_temp, float dht_hyg) {
  u8x8.drawString(0, 1, ("uC t = " + FloatToString(uC_temp, 2) + " C").c_str());
  u8x8.drawString(0, 2, ("ext t = " + FloatToString(dht_temp, 2) + " C").c_str());
  u8x8.drawString(0, 3, ("hum = " + FloatToString(dht_hyg, 2) + " %").c_str());
}

void check_time(){
   timeClient.update();
   unsigned long ep = timeClient.getEpochTime();
   int s = timeClient.getSeconds();
   if (m_start_growth == 1) {
   switch (m_phase){
    case 0 : {s < 0 ? setLED("on") : setLED("off"); break;}
    case 1 : {s < 20 ? setLED("on") : setLED("off"); break;}
    case 2 : {s < 45 ? setLED("on") : setLED("off"); break;}
    case 3 : {s < 30 ? setLED("on") : setLED("off"); break;}
    default : {s < 45 ? setLED("on") : setLED("off"); Serial.println("strange phase detected on check_time()"); break;}
   }
   if (s == 0 && m_is_new_day) { 
    m_phase_state[m_phase]++;
    m_is_new_day = false;
    switch (m_phase) {
      case 0 : {preferences.putInt("m_phase_state_0", m_phase_state[m_phase]); break;}
      case 1 : {preferences.putInt("m_phase_state_1", m_phase_state[m_phase]); break;}
      case 2 : {preferences.putInt("m_phase_state_2", m_phase_state[m_phase]); break;}
      case 3 : {preferences.putInt("m_phase_state_3", m_phase_state[m_phase]); break;}
      default : {Serial.println("strange phase detected on check_time()"); break;}
    }
   }
   if (s == 1)
    m_is_new_day = true;
   }
   Serial.print("m_phase is ");
   Serial.println(m_phase);
   Serial.print("Second is ");
   Serial.println(s);
   Serial.print("epohal is ");
   Serial.println(ep);
}

void setLED(string state) {
  if (state == "on") {
    LED_State = "on";
    digitalWrite(led, HIGH); 
    Serial.print("LED set to ");
    Serial.println(state.c_str());
  }
  if (state == "off") {
    LED_State = "off";
    digitalWrite(led, LOW);
    Serial.print("LED set to ");
    Serial.println(state.c_str());
  }
}

string FloatToString(float value, uint precision) {
  stringstream stream;
  stream << fixed << setprecision(precision) << value;
  string s_value = stream.str();
  return s_value;
}
