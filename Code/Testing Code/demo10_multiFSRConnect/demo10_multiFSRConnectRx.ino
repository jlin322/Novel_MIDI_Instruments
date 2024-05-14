/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 03/13/2024
  Purpose: The purpose of this program is to receive all 7 FSR values from 1 keypad
            and produce the corresponding MIDI signal to send to the synthesizer. 
*/

#include <SPI.h>
#include <RF24.h>
#include <Control_Surface.h>
#include <print.h>

RF24 radio(9,10); // pin assignments for nRF CE,CSN

USBMIDI_Interface midi; //MIDI over USB interface object

const MIDIAddress noteAddr1 {MIDI_Notes::C(4), Channel_1};   //for the purposes of this experiment, will set constant notes for each noteAddr
const MIDIAddress noteAddr2 {MIDI_Notes::D(4), Channel_1}; 
const MIDIAddress noteAddr3 {MIDI_Notes::C(4), Channel_1}; 
const MIDIAddress noteAddr4 {MIDI_Notes::C(4), Channel_1}; 
const MIDIAddress noteAddr5 {MIDI_Notes::C(4), Channel_1}; 
const MIDIAddress noteAddr6 {MIDI_Notes::C(4), Channel_1}; 
const MIDIAddress noteAddr7 {MIDI_Notes::C(4), Channel_1}; 
const uint8_t velocity = 0x7F;  //0x7f is 127 in decimal
const uint8_t offvelocity = 0x00;
const uint8_t ccAddr = MIDI_CC::Channel_Volume;  //changes control to ChannelVolume

#define SIZE 3

// struct for 7 FSR data sent to base station
struct DataSig{
  uint32_t fsr0;
  uint32_t fsr1;
  uint32_t fsr2;
  uint32_t fsr3;
  uint32_t fsr4;
  uint32_t fsr5;
  uint32_t fsr6;
};
//initalize struct
struct DataSig muxData;

struct midiSig{   //these contain variables for used for each FSR
  int sentfsrValue;
  int prevVolume;
  int currVolume;
  int initVolume;
  int turnOffFirstTime;
  int firstTimeNoteOn;
  //const MIDIAddress noteAddr;
  int volHistory[SIZE];
};

// initialize structs to keep track of each individual key
midiSig key1Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key2Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key3Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key4Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key5Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key6Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key7Data = {0,0,0,0,1,1,{0, 0, 0}};
midiSig fsrStruct = {0,0,0,0,1,1,{0,0,0}};

uint32_t fsrReading; // This is the value that represents the fsr received from Tx

//call function to turn sendNoteOff depending on i
void chooseNoteOff(int i){
  if(i == 0b0000){
    midi.sendNoteOff(noteAddr1, velocity);
  }
  else if(i == 0b0001){
    midi.sendNoteOff(noteAddr2, velocity);
  }
  else if(i == 0b0010){
    midi.sendNoteOff(noteAddr3, offvelocity);
  }
  else if(i == 0b0011){
    midi.sendNoteOff(noteAddr4, offvelocity);
  }
  else if(i == 0b0100){
    midi.sendNoteOff(noteAddr5, offvelocity);
  }
  else if(i == 0b0101){
    midi.sendNoteOff(noteAddr6, offvelocity);
  }
  else if(i == 0b0110){
    midi.sendNoteOff(noteAddr7, offvelocity);
  }
}

//this function assigns which noteAddr gets turned on
void chooseNoteOn(int i){
  if(i == 0b0000){
    midi.sendNoteOn(noteAddr1, velocity);
  }
  else if(i == 0b0001){
    midi.sendNoteOn(noteAddr2, velocity);
  }
  else if(i == 0b0010){
    midi.sendNoteOn(noteAddr3, velocity);
  }
  else if(i == 0b0011){
    midi.sendNoteOn(noteAddr4, velocity);
  }
  else if(i == 0b0100){
    midi.sendNoteOn(noteAddr5, velocity);
  }
  else if(i == 0b0101){
    midi.sendNoteOn(noteAddr6, velocity);
  }
  else if(i == 0b0110){
    midi.sendNoteOn(noteAddr7, velocity);
  }
}

//this function assigns the appropriate struct to the for loop in the main depending on the i value
void pickStruct(int i){
  if(i == 0){
    //fsrStruct = key1Data;
    fsrStruct.prevVolume = key1Data.prevVolume;
    fsrStruct.currVolume = key1Data.currVolume;
    fsrStruct.initVolume = key1Data.initVolume;
    fsrStruct.turnOffFirstTime = key1Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key1Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key1Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key1Data.volHistory[i];
    }
  }
  else if(i == 1){
    //fsrStruct = key2Data;
    fsrStruct.prevVolume = key2Data.prevVolume;
    fsrStruct.currVolume = key2Data.currVolume;
    fsrStruct.initVolume = key2Data.initVolume;
    fsrStruct.turnOffFirstTime = key2Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key2Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key2Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key2Data.volHistory[i];
    }
  }
  else if(i == 2){
    //fsrStruct = key3Data;
    fsrStruct.prevVolume = key3Data.prevVolume;
    fsrStruct.currVolume = key3Data.currVolume;
    fsrStruct.initVolume = key3Data.initVolume;
    fsrStruct.turnOffFirstTime = key3Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key3Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key3Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key3Data.volHistory[i];
    }
  }
  else if(i == 3){
    //fsrStruct = key4Data;
    fsrStruct.prevVolume = key4Data.prevVolume;
    fsrStruct.currVolume = key4Data.currVolume;
    fsrStruct.initVolume = key4Data.initVolume;
    fsrStruct.turnOffFirstTime = key4Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key4Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key4Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key4Data.volHistory[i];
    }
  }
  else if(i == 4){
    //fsrStruct = key5Data;
    fsrStruct.prevVolume = key5Data.prevVolume;
    fsrStruct.currVolume = key5Data.currVolume;
    fsrStruct.initVolume = key5Data.initVolume;
    fsrStruct.turnOffFirstTime = key5Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key5Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key5Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key5Data.volHistory[i];
    }
  }
  else if(i == 5){
    //fsrStruct = key6Data;
    fsrStruct.prevVolume = key6Data.prevVolume;
    fsrStruct.currVolume = key6Data.currVolume;
    fsrStruct.initVolume = key6Data.initVolume;
    fsrStruct.turnOffFirstTime = key6Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key6Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key6Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key6Data.volHistory[i];
    }
  }
  else if(i == 6){
    //fsrStruct = key7Data;
    fsrStruct.prevVolume = key7Data.prevVolume;
    fsrStruct.currVolume = key7Data.currVolume;
    fsrStruct.initVolume = key7Data.initVolume;
    fsrStruct.turnOffFirstTime = key7Data.turnOffFirstTime;
    fsrStruct.firstTimeNoteOn = key7Data.firstTimeNoteOn;
    //fsrStruct.noteAddr = key7Data.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      fsrStruct.volHistory[i] = key7Data.volHistory[i];
    }
  }
}

//this function copies all values from fsrStruct to appropriate struct for key
void copytoStruct(int i){
  if(i == 0){
    //key1Data = fsrStruct;
    key1Data.prevVolume = fsrStruct.prevVolume;
    key1Data.currVolume = fsrStruct.currVolume;
    key1Data.initVolume = fsrStruct.initVolume;
    key1Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key1Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key1Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key1Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 1){
    //key2Data = fsrStruct;
    key2Data.prevVolume = fsrStruct.prevVolume;
    key2Data.currVolume = fsrStruct.currVolume;
    key2Data.initVolume = fsrStruct.initVolume;
    key2Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key2Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key2Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key2Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 2){
    //key3Data = fsrStruct;
    key3Data.prevVolume = fsrStruct.prevVolume;
    key3Data.currVolume = fsrStruct.currVolume;
    key3Data.initVolume = fsrStruct.initVolume;
    key3Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key3Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key3Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key3Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 3){
    //key4Data = fsrStruct;
    key4Data.prevVolume = fsrStruct.prevVolume;
    key4Data.currVolume = fsrStruct.currVolume;
    key4Data.initVolume = fsrStruct.initVolume;
    key4Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key4Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key4Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key4Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 4){
    //key5Data = fsrStruct;
    key5Data.prevVolume = fsrStruct.prevVolume;
    key5Data.currVolume = fsrStruct.currVolume;
    key5Data.initVolume = fsrStruct.initVolume;
    key5Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key5Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key5Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key5Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 5){
    //key6Data = fsrStruct;
    key6Data.prevVolume = fsrStruct.prevVolume;
    key6Data.currVolume = fsrStruct.currVolume;
    key6Data.initVolume = fsrStruct.initVolume;
    key6Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key6Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key6Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key6Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
  else if(i == 6){
    //key7Data = fsrStruct;
    key7Data.prevVolume = fsrStruct.prevVolume;
    key7Data.currVolume = fsrStruct.currVolume;
    key7Data.initVolume = fsrStruct.initVolume;
    key7Data.turnOffFirstTime = fsrStruct.turnOffFirstTime;
    key7Data.firstTimeNoteOn = fsrStruct.firstTimeNoteOn;
    //key7Data.noteAddr = fsrStruct.noteAddr;
    for (int i = 0; i < SIZE; i++) {
      key7Data.volHistory[i] = fsrStruct.volHistory[i];
    }
  }
}

// will have to change this function to map values for all 7 FSR's-----------------------------------------------------------------//
// this function maps the values received to the volume range
int mapFSRtoVolume(int fsrValue0, int fsrValue1, int fsrValue2, int fsrValue3, int fsrValue4, int fsrValue5, int fsrValue6, int i) {
  //maps fsrValue(0-420) to somewhere in the volume range (0-127)
  // constrain ensures that the value derived from map falls within the min and max which is 0 and 127
  if(i == 0b0000){ //maps to mux channel 0(C0)
    return constrain(map(fsrValue0, 0, 320, 20, 127), 20, 127);
  }
  else if(i == 0b0001){
    return constrain(map(fsrValue1, 0, 300, 20, 60), 20, 60);
  }
  else if(i == 0b0010){
    return constrain(map(fsrValue2, 0, 150, 20, 127), 20, 127);
  }
  else if(i == 0b0011){
    return constrain(map(fsrValue3, 0, 150, 20, 127), 20, 127);
  }
  else if(i == 0b0100){
    return constrain(map(fsrValue4, 0, 150, 20, 127), 20, 127);
  }
  else if(i == 0b0101){
    return constrain(map(fsrValue5, 0, 150, 20, 127), 20, 127);
  }
  else if(i == 0b0110){
    return constrain(map(fsrValue6, 0, 150, 20, 127), 20, 127);
  }
  return 0;
}

void fadeVolume(int ith_Val){
  // manipulate the velocity for this
  // decrease fastly the velocity by a lot if the initial pressure is greater than following pressure
  // decrease slowly if the initial pressure is equal to, a little bit greater than or less than following pressure
  if(fsrStruct.initVolume > fsrStruct.currVolume){
    //then decrease volume very fast
    for(int i=fsrStruct.initVolume; i>=0; i-=20)
    {
      midi.sendControlChange(ccAddr, fsrStruct.initVolume); //changes CC to volume control option
                                                    // would be sending the prev. volume
      chooseNoteOn(ith_Val);
    }
  }
  else{
    // decrease volume slowly
    for(int i=fsrStruct.initVolume; i>=0; i-=5)
    {
      midi.sendControlChange(ccAddr, fsrStruct.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
      chooseNoteOn(ith_Val);
    }
  }
}

void updateVolHistory(int newVolume){ 
  for(int i=0; i<SIZE-1; i++)
  {
    fsrStruct.volHistory[i] = fsrStruct.volHistory[i+1];
  }

  // update rightmost element
  fsrStruct.volHistory[SIZE-1] = newVolume;
  //Serial.print("volHistory Here:-------------------------------");
  /*for(int i=0; i<SIZE;i++)
  {
    Serial.print(fsrStruct.volHistory[i]);
    Serial.print(" , ");
  }
  Serial.println("");*/
}

void setup(void){
  Serial.begin(51200);

  radio.begin(); // Start the NRF24L01
  radio.openReadingPipe(1, 0xABCDEF12); // pipe #, addr of transmitter: 0001
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();


  midi.begin(); //Initializes MIDI interface
  Serial.println("MIDI interface initialized.");

  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  // use fsrReading as volume data signal to synthesizer 
  if(radio.available())
  {
    radio.read(&muxData, sizeof(muxData));
    Serial.println("2. muxData: "+String(muxData.fsr0)+", "+String(muxData.fsr1)+", "+String(muxData.fsr2)+", "+String(muxData.fsr3)+", "+String(muxData.fsr4)+", "+String(muxData.fsr5)+", "+String(muxData.fsr6));
    Serial.println("");

    for(byte i=0b0000; i<=0b0110; i++){                                                                 //changed for testing purposes---------------------

      //call pickStruct and assign every variable with a [struct_name].[variable]
      pickStruct(i);

      //update prevVolume with currVolume if 
      fsrStruct.prevVolume = fsrStruct.currVolume;
      fsrStruct.currVolume = mapFSRtoVolume(muxData.fsr0, muxData.fsr1, muxData.fsr2, muxData.fsr3, muxData.fsr4, muxData.fsr5, muxData.fsr6, i);    //change fsrReading to the struct.fsrReading
      Serial.println("3. Key"+String(i)+" mapped with value of: "+String(fsrStruct.currVolume));

      // Set Initial Volume
      if(fsrStruct.prevVolume == 20 && fsrStruct.currVolume != 20){
        fsrStruct.initVolume = fsrStruct.currVolume;
      }

      // if note is released or just not pressed, then proceed, else turn note on
      if(fsrStruct.prevVolume == 20 && fsrStruct.currVolume == 20 && fsrStruct.turnOffFirstTime == 1)
      {
        //call function to turn sendNoteOff depending on i
        chooseNoteOff(i);
        Serial.println("6. Key"+String(i)+" Note off VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");
        fsrStruct.turnOffFirstTime = 0;
        fsrStruct.firstTimeNoteOn = 1;
      }
      else if(fsrStruct.currVolume != 20 && fsrStruct.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here

        fadeVolume(i);
        Serial.println("Note on XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("5. Key"+String(i)+"turned on with value of "+String(fsrStruct.initVolume));
        
        fsrStruct.turnOffFirstTime = 1;
        fsrStruct.firstTimeNoteOn = 0;
      }
      else{
        Serial.println("Do Nothing");
      }
      //call function to copy all values from fsrReading to struct
      copytoStruct(i);
      
    }

  }
  else
  {
    Serial.println("No msg received "); 
  }

  midi.update(); // Handle or discard MIDI input
  delay(23);
}