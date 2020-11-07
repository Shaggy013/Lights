/* RF24SensorNet rxTest
   Uses an nRF24L01 radio module on pins 9 and 10,
   and allows remote control of an LED attached to pin 5.

   -- 
   Peter Hardy <peter@hardy.dropbear.id.au>
*/
#include <SPI.h>

#include <RF24.h>
#include <RF24Network.h>
#include <RF24SensorNet.h>
# include <EEPROM.h>

static uint16_t myAddr = 021; // RF24Network address of this node.
//static uint16_t myLedId = 1; // The ID of the switch that we're exposing.
#define devicesCount 4

//static int ledPin = 5; // Arduino pin LED is attached to.
uint8_t devicesPins[devicesCount] = {3, 4, 5, 6};


RF24 radio(9, 10);
RF24Network network(radio);
RF24SensorNet sensornet(network);

// Callback when a switch update request is received.
void setLed(uint16_t fromAddr, uint16_t id, bool state, uint32_t timer) {
  if (id < devicesCount) {
    digitalWrite(devicesPins[id], state);
    // Send a message back to the requester indicating new state
    sensornet.sendSwitch(fromAddr, id, state, 0);
     }
}

void setup() {
   // Initialise the SPI bus.
  SPI.begin();
  // Initialise the nRF24L01 radio.
  radio.begin();
  // Initialise the RF24Network.
  network.begin( /*channel*/ 90, /*node address*/ myAddr);
  // Initialise the RF24SensorNet
  sensornet.begin();
  Serial.println("Sensor");
  Serial.println(myAddr);
  Serial.println(myAddr,OCT);
  

  // Attach a function to the Switch callback
  sensornet.addSwitchWriteHandler(setLed);

  // set the digital pin as output
  //pinMode(ledPin, OUTPUT);
  for (uint8_t ch = 0; ch < devicesCount; ch++) {
    pinMode(devicesPins[ch], OUTPUT);
 }
}

void loop() {
  // Update the RF24SensorNet network
  sensornet.update();
   
  
}
