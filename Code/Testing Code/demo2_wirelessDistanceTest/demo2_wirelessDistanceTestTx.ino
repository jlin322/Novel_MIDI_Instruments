/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 02/08/2024
  Purpose: To send a simple message between nRF transceiver/receiver modules
            to test for functionality. This is the transceiver module.
*/  

#include <SPI.h>
#include <RF24.h>

RF24 radio(9,10); // pin assignments for nRF CE,CSN

// Node addresses {transmitter, receiver}
//const byte addresses[][6] = {"00001", "00002"};

void setup(void){
  Serial.begin(9600);

  Serial.println("Initializing radio...");
  radio.begin(); // Start the NRF24L01
  Serial.println("Radio initialized.");

  Serial.println("Setting up writing pipe...");
  radio.openWritingPipe(0xABCDEF12); 
  Serial.println("Writing pipe set up.");

  Serial.println("Setting up PA levels...");
  radio.setPALevel(RF24_PA_MIN);  // RF24_PA_MAX is default.
  Serial.println("PA levels set up.");
  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  const char* msg = "Hello World";

  Serial.print("Sending: ");
  Serial.println(msg);
  radio.write(msg, strlen(msg));

  //delay(500);  //send msg every sec

}