/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 04/11/2024
  Purpose: The purpose of this program is to produce different zones
            for the keypads to be able to switch between different
            octaves depending on the value of the rotary encoder.
*/

#include <SPI.h>
#include <RF24.h>
#include <Control_Surface.h>
#include <print.h>

RF24 radio(9,10); // pin assignments for nRF CE,CSN

USBMIDI_Interface midi; //MIDI over USB interface object

MIDIAddress noteAddr1;   //left most white key
MIDIAddress noteAddr2;  
MIDIAddress noteAddr3; 
MIDIAddress noteAddr4;  //right most white key
MIDIAddress noteAddr5;  //left most black key
MIDIAddress noteAddr6;  
MIDIAddress noteAddr7;  //right most black key
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
  int8_t rotEncoder;
  int16_t joystick;
  bool rotPlate1;
  bool rotPlate2;
  bool rotPlate3;
};
//initalize struct
struct DataSig dataSigReceived;

struct midiSig{   //these contain variables for used for each FSR
  int sentfsrValue;
  int prevVolume;
  int currVolume;
  int initVolume;
  int turnOffFirstTime;
  int firstTimeNoteOn;
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
midiSig fsrStruct = {0,0,0,0,1,1,{0,0,0}};    //temporary struct placer to be copied into other key#Data

uint32_t fsrReading; // This is the value that represents the fsr received from Tx

// Function to turn sendNoteOff depending on i
void chooseNoteOff(int i, int zone){
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
  else if(i == 0b0110 && zone%2 != 0){
    midi.sendNoteOff(noteAddr7, offvelocity);
  }
}

// Function assigns which noteAddr gets turned on
void chooseNoteOn(int i, int zone){
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
  else if(i == 0b0110 && (zone%2) != 0){     //since B sharp dne, do not turn on in even zones
    midi.sendNoteOn(noteAddr7, velocity);
  }
}

// Function assigns the appropriate struct to the for loop in the main depending on the i value
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

// Function copies all values from fsrStruct to key#Data
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

// Function maps the values received to the volume range
int mapFSRtoVolume(int fsrValue0, int fsrValue1, int fsrValue2, int fsrValue3, int fsrValue4, int fsrValue5, int fsrValue6, int i) {
  //maps fsrValue(0-420) to somewhere in the volume range (0-127)
  // constrain ensures that the value derived from map falls within the min and max which is 0 and 127
  if(i == 0b0000){ //maps to mux channel 0(C0)
    return constrain(map(fsrValue0, 10, 220, 20, 127), 20, 127);
  }
  else if(i == 0b0001){
    return constrain(map(fsrValue1, 10, 280, 20, 60), 20, 60);
  }
  else if(i == 0b0010){
    return constrain(map(fsrValue2, 10, 305, 20, 127), 20, 127);
  }
  else if(i == 0b0011){
    return constrain(map(fsrValue3, 10, 470, 20, 127), 20, 127);
  }
  else if(i == 0b0100){
    return constrain(map(fsrValue4, 10, 150, 20, 127), 20, 127);
  }
  else if(i == 0b0101){
    return constrain(map(fsrValue5, 10, 250, 20, 127), 20, 127);
  }
  else if(i == 0b0110){
    return constrain(map(fsrValue6, 10, 180, 20, 127), 20, 127);
  }
  return 0;
}

// Function to produce key fade in piano mode
void fadeVolume(int ith_Val, int zone){
  // manipulate the velocity for this
  // decrease fastly the velocity by a lot if the initial pressure is greater than following pressure
  // decrease slowly if the initial pressure is equal to, a little bit greater than or less than following pressure

  if(fsrStruct.initVolume > fsrStruct.currVolume){
    //then decrease volume very fast
    for(int i=fsrStruct.initVolume; i>=0; i-=20)
    {
      midi.sendControlChange(ccAddr, fsrStruct.initVolume); //changes CC to volume control option
                                                    // would be sending the prev. volume
      
      //control pitch bend -8192 and +8191
      uint16_t joystickRead = constrain(map(dataSigReceived.joystick,0, 10230, 0, 16383), 0, 16383);
      //Serial.println("joy: "+String(joystickRead));
      midi.sendPitchBend(Channel_1, joystickRead);
      chooseNoteOn(ith_Val, zone);
    }
  }
  else{
    // decrease volume slowly
    for(int i=fsrStruct.initVolume; i>=0; i-=5)
    {
      midi.sendControlChange(ccAddr, fsrStruct.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
      //control pitch bend
      uint16_t joystickRead = constrain(map(dataSigReceived.joystick,0, 10230, 0, 16383), 0, 16383);
      //Serial.println("joy: "+String(joystickRead));
      midi.sendPitchBend(Channel_1, joystickRead);
      chooseNoteOn(ith_Val, zone);
    }
  }
}

// Function updates array with new volume content and keeps the previous 2 volume's
void updateVolHistory(int newVolume){ 
  for(int i=0; i<SIZE-1; i++)
  {
    fsrStruct.volHistory[i] = fsrStruct.volHistory[i+1];
  }
  // update rightmost element
  fsrStruct.volHistory[SIZE-1] = newVolume;
}

// Function to determine appropriate notes keypad should produce dependent on value of rotary encoder
int pianoOctave(){
  int8_t rE = dataSigReceived.rotEncoder;
  if(rE >= -38 && rE <= -26){
    //set note for fsr6 to C and fsr0 to F
    noteAddr1 = {40, Channel_1};
    noteAddr2 = {41, Channel_1};
    noteAddr3 = {43, Channel_1};
    noteAddr4 = {45, Channel_1};
    noteAddr5 = {42, Channel_1};
    noteAddr6 = {44, Channel_1};
    noteAddr7 = {46, Channel_1};
    Serial.println("Zone 1");
    return 1;
  }
  else if(rE >= -25 && rE <= -13){
    //set note for fsr6 to C and fsr0 to F
    noteAddr1 = {47, Channel_1};
    noteAddr2 = {48, Channel_1};
    noteAddr3 = {50, Channel_1};
    noteAddr4 = {53, Channel_1};
    noteAddr5 = {49, Channel_1};
    noteAddr6 = {51, Channel_1};
    //noteAddr7 = {-999, Channel_1};
    Serial.println("Zone 2");
    return 2;
  }
  else if(rE >= -12 && rE <= 0){
    //set note for fsr6 to C and fsr0 to F
    noteAddr1 = {53, Channel_1};
    noteAddr2 = {55, Channel_1};
    noteAddr3 = {57, Channel_1};
    noteAddr4 = {59, Channel_1};
    noteAddr5 = {54, Channel_1};
    noteAddr6 = {56, Channel_1};
    noteAddr7 = {58,Channel_1};
    Serial.println("Zone 3");
    return 3;
  }
  else if(rE >= 1 && rE <= 13){
    //set note for fsr6 to C and fsr0 to F
    noteAddr1 = {60, Channel_1};
    noteAddr2 = {62, Channel_1};
    noteAddr3 = {64, Channel_1};
    noteAddr4 = {65, Channel_1};
    noteAddr5 = {61, Channel_1};
    noteAddr6 = {63, Channel_1};
    //noteAddr7 = {-999, Channel_1};
    Serial.println("Zone 4");
    return 4;
  }
  else if(rE >= 14 && rE <= 26){
    noteAddr1 = {67, Channel_1};
    noteAddr2 = {69, Channel_1};
    noteAddr3 = {71, Channel_1};
    noteAddr4 = {72, Channel_1};
    noteAddr5 = {66, Channel_1};
    noteAddr6 = {68, Channel_1};
    noteAddr7 = {70, Channel_1};
    Serial.println("Zone 5");
    return 5;
  }
  else if(rE >= 27 && rE <= 39){
    noteAddr1 = {74, Channel_1};
    noteAddr2 = {76, Channel_1};
    noteAddr3 = {77, Channel_1};
    noteAddr4 = {79, Channel_1};
    noteAddr5 = {73, Channel_1};
    noteAddr6 = {75, Channel_1};
    //noteAddr7 = {-999, Channel_1};
    Serial.println("Zone 6");
    return 6;
  }
  return 0;
}

void setup(void){
  Serial.begin(9600);
  radio.begin(); // Start the NRF24L01
  radio.openReadingPipe(1, 0xABCDEF12); // pipe #, addr of transmitter: 0001
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
  midi.begin(); //Initializes MIDI interface
  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  // use fsrReading as volume data signal to synthesizer 
  if(radio.available())
  {
    radio.read(&dataSigReceived, sizeof(dataSigReceived));
    Serial.println("2. dataSigReceived: "+String(dataSigReceived.fsr0)+", "+String(dataSigReceived.fsr1)+", "+String(dataSigReceived.fsr2)+", "+String(dataSigReceived.fsr3)+", "+String(dataSigReceived.fsr4)+", "+String(dataSigReceived.fsr5)+", "+String(dataSigReceived.fsr6));
    Serial.println("2. Rotary Encoder Val: "+ String(dataSigReceived.rotEncoder));
    Serial.println("2. Joystick: "+String(dataSigReceived.joystick));
    Serial.println("");

    // set notes to keys here and return a zone to prevent B sharp from turning on
    int zone = pianoOctave();

    for(byte i=0b0000; i<=0b0110; i++){

      //call pickStruct and assign every variable with a [struct_name].[variable]
      pickStruct(i);

      //update prevVolume with currVolume if 
      fsrStruct.prevVolume = fsrStruct.currVolume;
      fsrStruct.currVolume = mapFSRtoVolume(dataSigReceived.fsr0, dataSigReceived.fsr1, dataSigReceived.fsr2, dataSigReceived.fsr3, dataSigReceived.fsr4, dataSigReceived.fsr5, dataSigReceived.fsr6, i);    //change fsrReading to the struct.fsrReading
      //Serial.println("3. Key"+String(i)+" mapped with value of: "+String(fsrStruct.currVolume));

      // Set Initial Volume
      if(fsrStruct.prevVolume <= 24 && fsrStruct.currVolume >= 25){
        fsrStruct.initVolume = fsrStruct.currVolume;
      }

      // if note is released or just not pressed, then proceed, else turn note on
      if(fsrStruct.prevVolume <= 24 && fsrStruct.currVolume <= 24)
      {
        //call function to turn sendNoteOff depending on i
        chooseNoteOff(i, zone);
        //Serial.println("6. Key"+String(i)+" Note off VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV");
        fsrStruct.turnOffFirstTime = 0;
        fsrStruct.firstTimeNoteOn = 1;
      }
      else if(fsrStruct.currVolume >= 25 && fsrStruct.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here

        fadeVolume(i, zone);
        //Serial.println("Note on XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        //Serial.println("5. Key"+String(i)+"turned on with value of "+String(fsrStruct.initVolume));
        
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


