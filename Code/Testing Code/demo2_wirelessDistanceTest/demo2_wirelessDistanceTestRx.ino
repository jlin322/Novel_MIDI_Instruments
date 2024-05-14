/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 02/##/2024
  Purpose: The purpose of this program is to determine and max distance between 
            the transceiver/receiver modules such that their is still a connection.
            This program receives messages from the transceiver module.
*/

#include <SPI.h>
#include <RF24.h>

RF24 radio(9,10); // pin assignments for nRF CE,CSN

// Node addresses {transmitter, receiver}
// const byte addresses[][6] = {"00001", "00002"};

void setup(void){
  Serial.begin(9600);

  radio.begin(); // Start the NRF24L01
  radio.openReadingPipe(1, 0xABCDEF12); // pipe #, addr of transmitter: 0001
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  if(radio.available())
  {
    char msg[32] = "";
    radio.read(&msg, sizeof(msg));

    Serial.print("Received: ");
    Serial.print("Length: ");
    Serial.print(strlen(msg));
    Serial.print(" Message: ");
    Serial.println(msg);
  }
  else
  {
    Serial.println("No msg received ");
  }
  delay(23);

}