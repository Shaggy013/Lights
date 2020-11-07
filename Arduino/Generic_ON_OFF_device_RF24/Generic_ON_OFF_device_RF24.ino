

/*
  This can control Generic Power Outlets with Remote Control over RF24 .
  Maximum a lot.
  
  Showing a lot Individual Hue Bulbs

*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <RF24.h>
#include <RF24Network.h>
#include <RF24SensorNet.h>
#include "printf.h"



//############ CONFIG ############

#define light_name "On-OFF Hue Nrf24"  //default light name

#define devicesCount 19 // 4 or 8 --> maximum a lot 
/***********************************************************************
************* Set the Node Address *************************************
/***********************************************************************/

// These are the Octal addresses that will be assigned pay attention to the devicecout !
const uint16_t node_address_set[devicesCount] = { 01, 011,02,012,031,041,051,03,03,03,03,0112,0112,0112,0112,013,013,013,013};
// switch id's 
uint8_t remote_switch_pins[devicesCount] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};

// node adresses is 00 master 01-05 ic child of master 011,021 are child of 01 etc octal format
// in this way you can cover your whole house or area 
// remote_switch_pins is what pin to switch at the node 
// node 01   switch 0   Huiskamer
// node 011  switch 0   Huiskamer achtter
// node 02   switch 0   Keuken
// node 021  switch 0   Berging keuken
// node 031  switch 0   Badkamer
// node 041  switch 0   Slaapkamer
// node 051  switch 0   Gang 
// node 012  switch 0   Senseo
// node 012  switch 1   Wasmachine
// node 012  switch 2   Vaatwasser
// node 012  switch 3   Frietpan
// node 0111 switch 0   Printer
// node 0111 switch 1   Reserve
// node 0111 switch 2   Bureaulamp
// node 0111 switch 3   Microscoop
// node 0121 switch 0   Soldeer Station 24v
// node 0121 switch 1   3D Printer 24v
// node 0121 switch 2   CNC 3016 24v
// node 0121 switch 3   Led Plaat
//
// RF24SensorNet is a libary wich i used with RF24 and RF24Network 
// https://github.com/nRF24/RF24 / https://github.com/nRF24/RF24Network / https://github.com/szaffarano/RF24SensorNet
// Why, i have a mysensor network and why not use things you already have , and besides that it makes the network not that crowded with esp8266 devices :)

uint8_t NODE_ADDRESS = 0;  // Use numbers 0 through to select an address from the array

/***********************************************************************/
/***********************************************************************/

static uint16_t myAddr = 00; // RF24Network address of this node.
int ledState = LOW; // The currrent state of the remote LED


//##########END OF CONFIG ##############

// RF24 Setup
RF24 radio(4,15);                    // nRF24L01(+) radio attached using Getting Started board 
RF24Network network(radio);          // Network uses that radio
RF24SensorNet sensornet(network);
//

uint8_t devicesPins[devicesCount] = {12, 13, 14, 5, 12, 13, 14, 5}; //irrelevant
int c;

// Callback when a switch status packet is received.
void getStatus(uint16_t fromAddr, uint16_t id, bool state, uint32_t timer) {
     if (state) {
          
      ledState = HIGH;
    } else {
      
      ledState = LOW;
    }
}


//#define USE_STATIC_IP //! uncomment to enable Static IP Adress
#ifdef USE_STATIC_IP
IPAddress strip_ip ( 192,  168,   0,  95); // choose an unique IP Adress
IPAddress gateway_ip ( 192,  168,   0,   1); // Router IP
IPAddress subnet_mask(255, 255, 255,   0);
#endif


bool device_state[devicesCount];
byte mac[6];

ESP8266WebServer server(80);

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void SwitchOn_RF24(uint8_t c) {

   //Serial.println("Sending switch on command.");
      sensornet.writeSwitch(node_address_set[c], remote_switch_pins[c], true, 0);
      delay(50);
             }

void SwitchOff_RF24(uint8_t c) {
 
    //Serial.println("Sending switch off command.");
      sensornet.writeSwitch(node_address_set[c], remote_switch_pins[c], false, 0);
      delay(50);
             }

void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);
  

  for (uint8_t ch = 0; ch < devicesCount; ch++) {
    pinMode(devicesPins[ch], OUTPUT);
  }
  

 #ifdef USE_STATIC_IP
  WiFi.config(strip_ip, gateway_ip, subnet_mask);
#endif


  if (EEPROM.read(1) == 1 || (EEPROM.read(1) == 0 && EEPROM.read(0) == 1)) {
    for (uint8_t ch = 0; ch < devicesCount; ch++) {
      digitalWrite(devicesPins[ch], OUTPUT);
    }

  }

  WiFiManager wifiManager;

  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect(light_name);

  WiFi.macAddress(mac);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.begin();
 
  server.on("/set", []() {
    uint8_t device;

    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "light") {
        device = server.arg(i).toInt() - 1;
      }
      else if (server.argName(i) == "on") {
        if (server.arg(i) == "True" || server.arg(i) == "true") {
          if (EEPROM.read(1) == 0 && EEPROM.read(0) != 1) {
            EEPROM.write(0, 1);
            EEPROM.commit();
          }
          device_state[device] = true;
          digitalWrite(devicesPins[device], HIGH);
          SwitchOn_RF24(device);
        }
        else {
          if (EEPROM.read(1) == 0 && EEPROM.read(0) != 0) {
            EEPROM.write(0, 0);
            EEPROM.commit();
          }
          device_state[device] = false;
          digitalWrite(devicesPins[device], LOW);
          SwitchOff_RF24(device);
        }
      }
    }
    server.send(200, "text/plain", "OK, state:" + device_state[device]);
  });

  server.on("/get", []() {
    uint8_t light;
    if (server.hasArg("light"))
      light = server.arg("light").toInt() - 1;
    String power_status;
    power_status = device_state[light] ? "true" : "false";
    server.send(200, "text/plain", "{\"on\": " + power_status + "}");
  });

  server.on("/detect", []() {
    server.send(200, "text/plain", "{\"hue\": \"bulb\",\"lights\": " + String(devicesCount) + ",\"name\": \"" light_name "\",\"modelid\": \"Plug 01\",\"mac\": \"" + String(mac[5], HEX) + ":"  + String(mac[4], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[0], HEX) + "\"}");
  
    
    });

  server.on("/", []() {
    float transitiontime = 100;
    if (server.hasArg("startup")) {
      if (  EEPROM.read(1) != server.arg("startup").toInt()) {
        EEPROM.write(1, server.arg("startup").toInt());
        EEPROM.commit();
      }
    }

    for (uint8_t device = 0; device < devicesCount; device++) {

      if (server.hasArg("on")) {
        if (server.arg("on") == "true") {
          device_state[device] = true;
          digitalWrite(devicesPins[device], HIGH);

          SwitchOn_RF24(device);

          if (EEPROM.read(1) == 0 && EEPROM.read(0) != 1) {
            EEPROM.write(0, 1);
            EEPROM.commit();
          }
        } else {
          device_state[device] = false;
          digitalWrite(devicesPins[device], LOW);
          SwitchOff_RF24(device);
          if (EEPROM.read(1) == 0 && EEPROM.read(0) != 0) {
            EEPROM.write(0, 0);
            EEPROM.commit();
          }
        }
      }
    }
    if (server.hasArg("reset")) {
      ESP.reset();
    }


    String http_content = "<!doctype html>";
    http_content += "<html>";
    http_content += "<head>";
    http_content += "<meta charset=\"utf-8\">";
    http_content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    http_content += "<title>Light Setup</title>";
    http_content += "<link rel=\"stylesheet\" href=\"https://unpkg.com/purecss@0.6.2/build/pure-min.css\">";
    http_content += "</head>";
    http_content += "<body>";
    http_content += "<fieldset>";
    http_content += "<h3>Light Setup</h3>";
    http_content += "<form class=\"pure-form pure-form-aligned\" action=\"/\" method=\"post\">";
    http_content += "<div class=\"pure-control-group\">";
    http_content += "<label for=\"power\"><strong>Power</strong></label>";
    http_content += "<a class=\"pure-button"; if (device_state[0]) http_content += "  pure-button-primary"; http_content += "\" href=\"/?on=true\">ON</a>";
    http_content += "<a class=\"pure-button"; if (!device_state[0]) http_content += "  pure-button-primary"; http_content += "\" href=\"/?on=false\">OFF</a>";
    http_content += "</div>";
    http_content += "</fieldset>";
    http_content += "</form>";
    http_content += "</body>";
    http_content += "</html>";

    server.send(200, "text/html", http_content);

  });


  server.onNotFound(handleNotFound);

  server.begin();

   // Initialise the RF24SensorNet
  
  SPI.begin();
  radio.begin();
  network.begin( /*channel*/ 90, /*node address*/ myAddr);
  
  sensornet.begin();

  // RF24SensorNet initialized

  // Attach the switch get handler callback to listen for responses.
  
  sensornet.addSwitchRcvHandler(getStatus);
}

void loop() {
   // Pump the RF24SensorNet
  sensornet.update();
  ArduinoOTA.handle();
  server.handleClient();
 delay(50); 
  
}
