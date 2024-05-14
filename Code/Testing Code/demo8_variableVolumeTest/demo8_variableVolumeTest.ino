/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 12/06/2023
  Purpose: The purpose of this program is to demonstrate variable volume control using the
    Control Surface Library.
    Note: Make sure Tools>USB Port to Serial+ MIDI
*/

#include <Control_Surface.h>
#include <print.h>

USBMIDI_Interface midi; //MIDI over USB interface object

void setup() {
  // put your setup code here, to run once:
  Serial.println("Starting to initialize MIDI interface...");
  midi.begin(); //Initializes MIDI interface
  Serial.println("MIDI interface initialized.");
  Serial.println("-------------------Program Starts--------------------");
}

// MIDI note number, channel, and velocity to use
const MIDIAddress noteAddr1 {MIDI_Notes::C(4), Channel_1};
const MIDIAddress noteAddr2 {MIDI_Notes::C(4), Channel_2};
const MIDIAddress noteAddr3 {MIDI_Notes::C(4), Channel_3};
const uint8_t velocity = 0x7F;

const uint8_t ccAddr = MIDI_CC::Channel_Volume;  //changes control to ChannelVolume

void loop() {
  
  // volume control on channel 1
  for(int volume=40; volume<128; volume=volume+6)
  {
  // put your main code here, to run repeatedly:
  Serial.println("Channel 1: Note On  ");
  midi.sendNoteOn(noteAddr1, velocity);
  delay(250);
  Serial.println("Channel 1: Note off");
  midi.sendNoteOff(noteAddr1, velocity);
  delay(250);
 
  Serial.println("Changing Volume");
  midi.sendControlChange(ccAddr, volume); //changes CC to volume control option
  delay(200);
  }

  // Serial.print("starting to update MIDI.");
  midi.update(); // Handle or discard MIDI input
}




/*-----------------------------Pitch Bend-------------------------------------
  
  // volume control on channel 2
  for(int volume=40; volume<128; volume=volume+6)
  {
  // put your main code here, to run repeatedly:
  Serial.println("Channel 2: Note On  ");
  midi.sendNoteOn(noteAddr2, velocity);
  delay(250);
  Serial.println("Channel 2: Note Off  ");
  midi.sendNoteOff(noteAddr2, velocity);
  delay(250);
 
  Serial.println("Changing Volume");
  midi.sendControlChange(ccAddr, volume); //changes CC to volume control option
  delay(200);
  }

  // volume control on channel 3
  for(int volume=40; volume<128; volume=volume+6)
  {
  // put your main code here, to run repeatedly:
  Serial.println("Channel 3: Note On  ");
  midi.sendNoteOn(noteAddr3, velocity);
  delay(250);
  Serial.println("Channel 3: Note Off  ");
  midi.sendNoteOff(noteAddr3, velocity);
  delay(250);
 
  Serial.println("Changing Volume");
  midi.sendControlChange(ccAddr, volume); //changes CC to volume control option
  delay(200);
  }
  
#include <Control_Surface.h>
#include <print.h>

USBMIDI_Interface midi; //MIDI over USB interface object

void setup() {
  // put your setup code here, to run once:
  Serial.println("Starting to initialize MIDI interface...");
  midi.begin(); //Initializes MIDI interface
  Serial.println("MIDI interface initialized.");
  Serial.println("-------------------Program Starts--------------------");

}

// MIDI note number, channel, and velocity to use
const MIDIAddress noteAddr {MIDI_Notes::C(4), Channel_1};
const uint8_t velocity = 0x7F;

const uint8_t ccAddr = MIDI_CC::Channel_Volume;  //changes control to ChannelVolume

void loop() {

  //Serial.println("Sending Control Change");
  //midi.sendControlChange(ccAddr, 100);


  // put your main code here, to run repeatedly:
  Serial.println("Note: currently on  ");
  midi.sendNoteOn(noteAddr, ccAddr);
  delay(500);
  midi.sendPitchBend(Channel_1, 127);
  delay(2500);
  Serial.println("Note: currently off");
  midi.sendNoteOff(noteAddr, velocity);
  delay(500);
 
  //Serial.println("Sending Control Change");
  //midi.sendControlChange(ccAddr, 50);
  //delay(500);

  //Serial.print("starting to update MIDI.");
  //midi.update(); // Handle or discard MIDI input

}

//----------------------------------------Tutorial----------------------------------------------


#include <Control_Surface.h>
#include <print.h>

USBMIDI_Interface midi; //MIDI over USB interface object

void setup() {
  // put your setup code here, to run once:
  Serial.println("Starting to initialize MIDI interface...");
  midi.begin(); //Initializes MIDI interface
  Serial.println("MIDI interface initialized.");
  Serial.println("-------------------Program Starts--------------------");

}

// MIDI note number, channel, and velocity to use
const MIDIAddress address {MIDI_Notes::C(4), Channel_1};
const uint8_t velocity = 0x7F;

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Note: currently on  ");
  midi.sendNoteOn(address, velocity);
  delay(500);
  Serial.println("Note: currently off");
  midi.sendNoteOff(address, velocity);
  delay(500);
 
  //Serial.print("starting to update MIDI.");
  //midi.update(); // Handle or discard MIDI input

}

*/
