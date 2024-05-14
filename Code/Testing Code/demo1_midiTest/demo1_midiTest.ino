/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 11/06/2023
  Purpose: To send MIDI signal to synthesizer such that channel 1 produces
            notes(48 to 68), then switching to channel 2 to produce sound,
            which then channel 1 produces pitch bend.
  Note: This file uses the old MIDIUSB and not the Control Surface Lib that is used later on.
*/          
#include "MIDIUSB.h"

uint8_t velocity = 90; // velocity values: 0-127
uint8_t channel = 1;
int numChannels = 2;
int channelArray[] = {1,2};  // used to determine order of channel to perform function
int i = 0;
int delayTime = 500;

void turnNoteOn(uint8_t note, uint8_t velocity, uint8_t channel)
{
  usbMIDI.sendNoteOn(note, velocity, channel);
  Serial.print("Note On, ch= ");
  Serial.print(channel, DEC); //prints channel in decimal format
  Serial.print(", note= ");
  Serial.print(note, DEC);
  Serial.print(", velocity= ");
  Serial.println(velocity, DEC); //prints newline after velocity value
}

void turnNoteOff(uint8_t note, uint8_t channel)
{
  usbMIDI.sendNoteOff(note, 0, channel);
  Serial.print("Note Off, ch= ");
  Serial.print(channel, DEC); //prints channel in decimal format
  Serial.print(", note= ");
  Serial.print(note, DEC);
  Serial.print(", velocity = 0 ");
}

void setup() {
  Serial.begin(115200);
//  usbMIDI.setHandleNoteOn(myNoteOn);
//  usbMIDI.setHandleNoteOff(myNoteOff);
//  usbMIDI.setHandleControlChange(myControlChange);
}
void loop() {
  // The handler functions are called when usbMIDI reads data.  They
  // will not be called automatically.  You must call usbMIDI.read()
  // regularly from loop() for usbMIDI to actually read incoming
  // data and run the handler functions as messages arrive.
  //usbMIDI.read();
  Serial.println("");
  Serial.println("------------------------------------Begin------------------------------------");

  for (int note = 48; note<70; note=note+2)
  {
    turnNoteOn(note, velocity, channel);
    delay(500);
    turnNoteOff(note, channel);
    delay(500);
  }

  for (int note = 40; note<70; note=note+2)
  {
    turnNoteOn(note, velocity, 2);
    delay(500);
    turnNoteOff(note, 2);
    delay(500);
  }

  //each channel plays entire note scale before switching to next channel
  for(int k=0; k<numChannels; k=k+1)
  {
    for(int note=48; note<70; note=note+2)
    {
      turnNoteOn(note, velocity, channelArray[k]);
      delay(delayTime);
      turnNoteOff(note, channelArray[k]);
      delay(delayTime);
    }
  }
}