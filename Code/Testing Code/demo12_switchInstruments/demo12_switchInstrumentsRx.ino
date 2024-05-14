/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 04/12/2024
  Purpose: The purpose of this program is to switch between instruments.
            In order to do this, multi-Tx communication must be performed.
  Notes:
    - rotational plate1/2/3: 100,011,110,001 (0 deg, 90 deg CCW, 180 deg, 270 deg CCW)
    - tilt sensor(h,v,a): horizontal(101), vertical(0,1,0), angled(0,0,1)
    - volHistory is never used
*/

#include <SPI.h>
#include <RF24.h>
#include <Control_Surface.h>
#include <print.h>

#define SIZE 3  //size of volume history
#define TILTSIZE 10

RF24 radio(9,10); // pin assignments for nRF CE,CSN
USBMIDI_Interface midi; //MIDI over USB interface object

MIDIAddress noteAddr1L;   //left most white key
MIDIAddress noteAddr2L;  
MIDIAddress noteAddr3L; 
MIDIAddress noteAddr4L;  //right most white key
MIDIAddress noteAddr5L;  //left most black key
MIDIAddress noteAddr6L;  
MIDIAddress noteAddr7L;  //right most black key
MIDIAddress noteAddr1R;   //left most white key
MIDIAddress noteAddr2R;  
MIDIAddress noteAddr3R; 
MIDIAddress noteAddr4R;  //right most white key
MIDIAddress noteAddr5R;  //left most black key
MIDIAddress noteAddr6R;  
MIDIAddress noteAddr7R;  //right most black key
const uint8_t velocity = 0x7F;  //0x7f is 127 in decimal
const uint8_t offvelocity = 0x00;
const uint8_t ccAddr = MIDI_CC::Channel_Volume;  //changes control to ChannelVolume

//digital pin setup for tilt sensors
const int sensorVPin = 5;
const int sensorAPin = 19;
const int sensorHPin = 20;

//global variables associated with tilt sensor noise elimination
int8_t tiltSensor = 0b000; //signal from tilt sensor
int cmpTilt; // assume you are starting at 100
int8_t sameValCount = 0;
int secondTiltVal = 0;
//create an array with size 10 to collect tiltSensor History that updates with every loop
int tiltSensHist[TILTSIZE];
int8_t iteratetiltHist= -1;
int8_t i_collectData = 0;

int8_t startingMuxSel = 0;

// struct for 7 FSR data sent to base station
struct DataSig{
  bool lOrR;  
  uint16_t fsr0;      //Force Sensitive Resistor Values
  uint16_t fsr1;
  uint16_t fsr2;
  uint16_t fsr3;
  uint16_t fsr4;
  uint16_t fsr5;
  uint16_t fsr6;
  int16_t rotEncoder;  //Rotary Encoder value
  bool rotPlate1;     //Rotational Plate Sensors
  bool rotPlate2;
  bool rotPlate3;
  int16_t joystick;   //move joystick in one direction (left/right)
};

//initalize struct
struct DataSig dataSigReceivedL;
struct DataSig dataSigReceivedR;
struct DataSig tempSig;

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
midiSig key1DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key2DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key3DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key4DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key5DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key6DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key7DataL = {0,0,0,0,1,1,{0, 0, 0}};
midiSig fsrStructL = {0,0,0,0,1,1,{0,0,0}};    //temporary struct placer to be copied into other key#Data
midiSig key1DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key2DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key3DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key4DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key5DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key6DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig key7DataR = {0,0,0,0,1,1,{0, 0, 0}};
midiSig fsrStructR = {0,0,0,0,1,1,{0,0,0}};    //temporary struct placer to be copied into other key#Data

// Function to turn sendNoteOff depending on i
void chooseNoteOff(int i, int zone, char whichKeypad){
  if(i == startingMuxSel){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr1L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr1R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0001)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr2L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr2R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0010)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr3L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr3R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0011)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr4L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr4R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0100)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr5L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr5R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0101)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr6L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr6R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0110)){
    if(whichKeypad == 'L'){
      midi.sendNoteOff(noteAddr7L, velocity);
    }
    else{
      midi.sendNoteOff(noteAddr7R, velocity);
    }
  }
}

// Function assigns which noteAddr gets turned on
void chooseNoteOn(int i, int zone, char whichKeypad){
  if(i == (startingMuxSel)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr1L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr1R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0001)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr2L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr2R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0010)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr3L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr3R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0011)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr4L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr4R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0100)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr5L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr5R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0101)){
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr6L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr6R, velocity);
    }
  }
  else if(i == (startingMuxSel+0b0110)){     //since B sharp dne, do not turn on in even zones
    if(whichKeypad == 'L'){
      midi.sendNoteOn(noteAddr7L, velocity);
    }
    else{
      midi.sendNoteOn(noteAddr7R, velocity);
    }
  }
}

// Function assigns the appropriate struct to the for loop in the main depending on the i value
void pickStruct(int i){
  if(i == (startingMuxSel)){
    memcpy(&fsrStructL, &key1DataL, sizeof(key1DataL));
    memcpy(&fsrStructR, &key1DataR, sizeof(key1DataR));
  }
  else if(i == (startingMuxSel+0b0001)){
    memcpy(&fsrStructL, &key2DataL, sizeof(key2DataL));
    memcpy(&fsrStructR, &key2DataR, sizeof(key2DataR));
  }
  else if(i == (startingMuxSel+0b0010)){
    memcpy(&fsrStructL, &key3DataL, sizeof(key3DataL));
    memcpy(&fsrStructR, &key3DataR, sizeof(key3DataR));
  }
  else if(i == (startingMuxSel+0b0011)){
    memcpy(&fsrStructL, &key4DataL, sizeof(key4DataL));
    memcpy(&fsrStructR, &key4DataR, sizeof(key4DataR));
  }
  else if(i == (startingMuxSel+0b0100)){
    memcpy(&fsrStructL, &key5DataL, sizeof(key5DataL));
    memcpy(&fsrStructR, &key5DataR, sizeof(key5DataR));
  }
  else if(i == (startingMuxSel+0b0101)){
    memcpy(&fsrStructL, &key6DataL, sizeof(key6DataL));
    memcpy(&fsrStructR, &key6DataR, sizeof(key6DataR));
  }
  else if(i == (startingMuxSel+0b0110)){
    memcpy(&fsrStructL, &key7DataL, sizeof(key7DataL));
    memcpy(&fsrStructR, &key7DataR, sizeof(key7DataR));
  }
}

// Function copies all values from fsrStruct to key#Data
void copytoStruct(int i){
  if(i == (startingMuxSel)){
    memcpy(&key1DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key1DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0001)){
    memcpy(&key2DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key2DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0010)){
    memcpy(&key3DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key3DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0011)){
    memcpy(&key4DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key4DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0100)){
    memcpy(&key5DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key5DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0101)){
    memcpy(&key6DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key6DataR, &fsrStructR, sizeof(fsrStructR));
  }
  else if(i == (startingMuxSel+0b0110)){
    memcpy(&key7DataL, &fsrStructL, sizeof(fsrStructL));
    memcpy(&key7DataR, &fsrStructR, sizeof(fsrStructR));
  }
}

// Function maps the values received to the volume range
int mapFSRtoVolume(int i, char whichKeypad) {
  //maps fsrValue(0-420) to somewhere in the volume range (0-127)
  // constrain ensures that the value derived from map falls within the min and max which is 0 and 127
  if(i == (startingMuxSel)){ //maps to mux channel 0(C0)
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr0, 10, 290, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr0, 10, 250, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0001)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr1, 10, 150, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr1, 10, 150, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0010)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr2, 10, 200, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr2, 10, 300, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0011)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr3, 10, 360, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr3, 10, 300, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0100)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr4, 10, 90, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr4, 10, 200, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0101)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr5, 10, 120, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr5, 10, 300, 20, 127), 20, 127);
    }
  }
  else if(i == (startingMuxSel+0b0110)){
    if(whichKeypad == 'L'){
      return constrain(map(dataSigReceivedL.fsr6, 10, 140, 20, 127), 20, 127);
    }
    else{
      return constrain(map(dataSigReceivedR.fsr6, 10, 200, 20, 127), 20, 127);
    }
  }
  return 0;
}

//setup or intializatiion
uint16_t joystickRead_last = 0;
uint16_t joystickRead = 0;
#define KEYNUM 7

//set up a struct to keep track of which note is on
struct noteOnHis{
  bool ith_Val[KEYNUM];
  char whichKeypad[KEYNUM];
};
struct noteOnHis noteOnSig = {{0,0,0,0,0,0,0}, {'N','N','N','N','N','N','N'}};

// Function to produce key fade in piano mode
void fadeVolume(int ith_Val, int zone, char whichKeypad){
  // manipulate the velocity for this
  // decrease faster if the velocity by a lot if the initial pressure is greater than following pressure
  // decrease slowly if the initial pressure is equal to, a little bit greater than or less than following pressure
  if(whichKeypad == 'L'){
    if(fsrStructL.initVolume > fsrStructL.currVolume){
      //decrease volume very fast
      for(int i=fsrStructL.initVolume; i>=0; i-=10)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
        //piano pitch bend////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joystickRead = constrain(map(dataSigReceivedL.joystick,0, 10230, 0, 16383), 0, 16383);
        if(joystickRead != joystickRead_last){
          midi.sendPitchBend(Channel_1, joystickRead);
        }
        joystickRead_last = joystickRead;       
        chooseNoteOn(ith_Val, zone, 'L');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructL.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, zone, 'L');
      }
    }
  }
  else{
    if(fsrStructR.initVolume > fsrStructR.currVolume){
      //decrease volume very fast
      for(int i=fsrStructR.initVolume; i>=0; i-=10)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
        
        //piano pitch bend///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joystickRead = constrain(map(dataSigReceivedL.joystick,0, 10230, 0, 16383), 0, 16383);
        if(joystickRead != joystickRead_last){
          midi.sendPitchBend(Channel_1, joystickRead);
        }
        joystickRead_last = joystickRead;
       
        chooseNoteOn(ith_Val, zone, 'R');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructR.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, zone, 'R');
      }
    }
  }
  //update noteOnHis struct here
  noteOnSig.ith_Val[ith_Val] = 1;
  noteOnSig.whichKeypad[ith_Val] = whichKeypad;
}

// Define the MIDI note names and their corresponding values
using namespace MIDI_Notes;

// Function to determine appropriate notes keypad should produce dependent on value of rotary encoder
int pianoOctave(int8_t tiltSensor, char whichKeypad){
  int16_t rE_L = dataSigReceivedL.rotEncoder;
  int16_t rE_R = dataSigReceivedR.rotEncoder;
  int maxRE = 385;
  int maxReRange = maxRE/6;

  if(whichKeypad == 'L'){
    if(rE_L <= maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::E(2), Channel_1};
      noteAddr2L = {41, Channel_1};
      noteAddr5L = {MIDI_Notes::G(2), Channel_1};
      noteAddr7L = {MIDI_Notes::A(2), Channel_1};
      noteAddr3L = {MIDI_Notes::Eb(2), Channel_1};
      noteAddr4L = {MIDI_Notes::Gb(2), Channel_1};
      noteAddr6L = {MIDI_Notes::Ab(2), Channel_1};
      Serial.println("Zone 1");
      return 1;
    }
    else if(rE_L >= maxReRange+1 && rE_L <= 2*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::B(2), Channel_1};
      noteAddr2L = {MIDI_Notes::C(3), Channel_1};
      noteAddr5L = {MIDI_Notes::D(3), Channel_1};
      noteAddr7L = {MIDI_Notes::E(3), Channel_1};
      noteAddr3L = {MIDI_Notes::Bb(2), Channel_1};
      noteAddr4L = {MIDI_Notes::Db(3), Channel_1};
      noteAddr6L = {MIDI_Notes::Eb(3), Channel_1};
      Serial.println("Zone 2");
      return 2;
    }
    else if(rE_L >= 2*maxReRange+1 && rE_L <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      //MIDI_Notes::note	(	int8_t 	note,int8_t 	numOctave )	;
      noteAddr1L = {53, Channel_1};
      //noteAddr1L = {53, Channel_1};
      noteAddr2L = {MIDI_Notes::G(3), Channel_1};
      noteAddr5L = {MIDI_Notes::A(3), Channel_1};
      noteAddr7L = {MIDI_Notes::B(3), Channel_1};
      noteAddr3L = {MIDI_Notes::Gb(3), Channel_1};
      noteAddr4L = {MIDI_Notes::Ab(3), Channel_1};
      noteAddr6L = {MIDI_Notes::Bb(3), Channel_1};
      Serial.println("Zone 3");
      return 3;
    }
    else if(rE_L >= 3*maxReRange+1 && rE_L <= 4*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::C(4), Channel_1};
      noteAddr2L = {MIDI_Notes::D(4), Channel_1};
      noteAddr5L = {MIDI_Notes::E(4), Channel_1};
      noteAddr7L = {65, Channel_1};
      noteAddr3L = {MIDI_Notes::Db(4), Channel_1};
      noteAddr4L = {MIDI_Notes::Eb(4), Channel_1};
      noteAddr6L = {MIDI_Notes::Gb(4), Channel_1};
      Serial.println("Zone 4");
      return 4;
    }
    else if(rE_L >= 4*maxReRange+1 && rE_L <= 5*maxReRange){
      noteAddr1L = {MIDI_Notes::G(4), Channel_1};
      noteAddr2L = {MIDI_Notes::A(4), Channel_1};
      noteAddr5L = {MIDI_Notes::B(4), Channel_1};
      noteAddr7L = {MIDI_Notes::C(5), Channel_1};
      noteAddr3L = {MIDI_Notes::Ab(4), Channel_1};
      noteAddr4L = {MIDI_Notes::Bb(4), Channel_1};
      noteAddr6L = {MIDI_Notes::Db(5), Channel_1};
      Serial.println("Zone 5");
      return 5;
    }
    else if(rE_L >= 5*maxReRange+1){
      noteAddr1L = {MIDI_Notes::D(5), Channel_1};
      noteAddr2L = {MIDI_Notes::E(5), Channel_1};
      noteAddr5L = {77, Channel_1};
      noteAddr7L = {MIDI_Notes::G(5), Channel_1};
      noteAddr3L = {MIDI_Notes::Eb(5), Channel_1};
      noteAddr4L = {MIDI_Notes::Gb(5), Channel_1};
      noteAddr6L = {MIDI_Notes::Ab(5), Channel_1};
      Serial.println("Zone 6");
      return 6;
    }
  }
  else{
    if(rE_R <= maxReRange){
      noteAddr1R = {MIDI_Notes::D(5), Channel_1};
      noteAddr3R = {MIDI_Notes::E(5), Channel_1};
      noteAddr5R = {77, Channel_1};
      noteAddr7R = {MIDI_Notes::G(5), Channel_1};
      noteAddr2R = {MIDI_Notes::Eb(5), Channel_1};
      noteAddr4R = {MIDI_Notes::Gb(5), Channel_1};
      noteAddr6R = {MIDI_Notes::Ab(5), Channel_1};
      Serial.println("R: Zone 6");
      return 16;
    }
    else if(rE_R >= maxReRange+1 && rE_R <= 2*maxReRange){
      noteAddr1R = {MIDI_Notes::G(4), Channel_1};
      noteAddr3R = {MIDI_Notes::A(4), Channel_1};
      noteAddr5R = {MIDI_Notes::B(4), Channel_1};
      noteAddr7R = {MIDI_Notes::C(5), Channel_1};
      noteAddr2R = {MIDI_Notes::Ab(4), Channel_1};
      noteAddr4R = {MIDI_Notes::Bb(4), Channel_1};
      noteAddr6R = {MIDI_Notes::Db(5), Channel_1};
      Serial.println("R: Zone 5");
      return 15;
    }
    else if(rE_R >= 2*maxReRange+1 && rE_R <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::C(4), Channel_1};
      noteAddr3R = {MIDI_Notes::D(4), Channel_1};
      noteAddr5R = {MIDI_Notes::E(4), Channel_1};
      noteAddr7R = {65, Channel_1};
      noteAddr2R = {MIDI_Notes::Db(4), Channel_1};
      noteAddr4R = {MIDI_Notes::Eb(4), Channel_1};
      noteAddr6R = {MIDI_Notes::Gb(4), Channel_1};
      Serial.println("R: Zone 4");
      return 14;
    }
    else if(rE_R >= 3*maxReRange+1 && rE_R <= 4*maxReRange){
      noteAddr1R = {53, Channel_1};
      noteAddr3R = {MIDI_Notes::G(3), Channel_1};
      noteAddr5R = {MIDI_Notes::A(3), Channel_1};
      noteAddr7R = {MIDI_Notes::B(3), Channel_1};
      noteAddr2R = {MIDI_Notes::Gb(3), Channel_1};
      noteAddr4R = {MIDI_Notes::Ab(3), Channel_1};
      noteAddr6R = {MIDI_Notes::Bb(3), Channel_1};
      Serial.println("R: Zone 3");
      return 13;
    }
    else if(rE_R >= 4*maxReRange+1 && rE_R <= 5*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::B(2), Channel_1};
      noteAddr3R = {MIDI_Notes::C(3), Channel_1};
      noteAddr5R = {MIDI_Notes::D(3), Channel_1};
      noteAddr7R = {MIDI_Notes::E(3), Channel_1};
      noteAddr2R = {MIDI_Notes::Bb(2), Channel_1};
      noteAddr4R = {MIDI_Notes::Db(3), Channel_1};
      noteAddr6R = {MIDI_Notes::Eb(3), Channel_1};
      Serial.println("R: Zone 2");
      return 12;
    }
    else if(rE_R >= 5*maxReRange+1){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::E(2), Channel_1};
      noteAddr3R = {41, Channel_1};
      noteAddr5R = {MIDI_Notes::G(2), Channel_1};
      noteAddr7R = {MIDI_Notes::A(2), Channel_1};
      noteAddr2R = {MIDI_Notes::Eb(2), Channel_1};
      noteAddr4R= {MIDI_Notes::Gb(2), Channel_1};
      noteAddr6R = {MIDI_Notes::Ab(2), Channel_1};
      Serial.println("R: Zone 1");
      return 11;
    }
  }
  return 0;
}

// Function turns piano on and off
void pianoOnOff(int i, int zoneL, int zoneR){
  
  if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume <= 24 && fsrStructL.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'L'){
      chooseNoteOff(i, zoneL, 'L');

      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';   
    }
    fsrStructL.turnOffFirstTime = 0;
    fsrStructL.firstTimeNoteOn = 1;
  }
  else if(fsrStructL.currVolume >= 25 && fsrStructL.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolume(i, zoneL, 'L');
    fsrStructL.turnOffFirstTime = 1;
    fsrStructL.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("L: Do Nothing");
  }

  if(fsrStructR.prevVolume <= 24 && fsrStructR.currVolume <= 24 && fsrStructR.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'R'){
      chooseNoteOff(i, zoneR, 'R');
      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';  
    }
    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;
  }
  else if(fsrStructR.currVolume >= 25 && fsrStructR.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolume(i, zoneR, 'R');
    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("R: Do Nothing");
  }
}

//NOTE: THIS FUNCTION DOES NOT WORK UNTIL BOTH KEYPADS ARE CONNECTED-----------------------------------------------------------------------------------------------
// Function switches instruments given rotEncoder value, tilt sensor, and rotational sensor
/*
int switchInstruments(int8_t tiltSensor, int16_t rE_L){
  char tilt = 'A';
  int8_t rotPlate_L = (dataSigReceivedL.rotPlate1<<2) | (dataSigReceivedL.rotPlate2<<1) | dataSigReceivedL.rotPlate3;  //left rotation Plate
  int8_t rotPlate_R = (dataSigReceivedR.rotPlate1<<2) | (dataSigReceivedR.rotPlate2<<1) | dataSigReceivedR.rotPlate3;  //right rotation Plate
  //Serial.println("s tiltSensor: "+String(tiltSensor));
  //Serial.println("s rotPlate_L: "+String(rotPlate_L));
  //Serial.println("s rotPlate_R: "+String(rotPlate_R));
  //Serial.println("s re_L: "+String(rE_L));
  //Serial.println("s re_R: "+String(rE_R));

  //determine orientation of tilt sensor
  if(tiltSensor == 0b011 || tiltSensor == 0b110){
    tilt = 'V';
  }
  else if(tiltSensor == 0b101 || tiltSensor == 0b001){
    tilt = 'A';
  }
  else{
    tilt = 'H';
  }

  //vertical if 010,011; h if 100 or 000; a if 101 001
  //channel 4 to clarinet         - as long as rotEncoder left is in zone 2, then proceed
  if(rE_L >= -25 && rE_L <= -13 && tilt == 'V' && rotPlate_L == 0b110 && rotPlate_R == 0b100){
    return 4;
  }
  //channel 5 to saxophone        - as long as rotEncoder left is in zone 1, then proceed
  else if(rE_L >= -38 && rE_L <= -26 && tilt == 'A' && rotPlate_L == 0b110 && rotPlate_R == 0b100){
    return 5;
  }
  //channel 1 to piano
  //else if((tiltSensor == 0b001 || tiltSensor == 0b101) && (rotPlate_L == 0b110|| rotPlate_L == 0b111) && (rotPlate_R == 0b111 || rotPlate_R == 0b111)){
  else if(tilt == 'H'){
    return 1;
  }
  //channel 2 to bass guitar
  else if(tilt == 'A' && rotPlate_L == 0b001 && rotPlate_R == 0b011){
    return 2;
  }
  //channel 3 to flute
  else if(tilt == 'H' && rotPlate_L == 0b110 && rotPlate_R == 0b100){
    return 3;
  }
  //channel 6 to bassoon
  else if(tilt == 'V' && rotPlate_L == 0b110 && rotPlate_R == 0b100){
    return 6;
  }
  else{
    //Serial.println("No Channel Selected");
    return 0;
  }
}
*/

// Final Demo Function///////////////////////////////////////////////////////////////////////////////////////////
int switchInstruments(int8_t tiltSensor, int16_t rE_L){
  char tilt = 'A';
  //int8_t rotPlate_L = (dataSigReceivedL.rotPlate1<<2) | (dataSigReceivedL.rotPlate2<<1) | dataSigReceivedL.rotPlate3;  //left rotation Plate
  //int8_t rotPlate_R = (dataSigReceivedR.rotPlate1<<2) | (dataSigReceivedR.rotPlate2<<1) | dataSigReceivedR.rotPlate3;  //right rotation Plate
  //Serial.println("s tiltSensor: "+String(tiltSensor));
  //Serial.println("s rotPlate_L: "+String(rotPlate_L));
  //Serial.println("s rotPlate_R: "+String(rotPlate_R));
  //Serial.println("s re_L: "+String(rE_L));
  //Serial.println("s re_R: "+String(rE_R));

  //determine orientation of tilt sensor
  if(tiltSensor == 0b011 || tiltSensor == 0b110){
    tilt = 'V';
  }
  else if(tiltSensor == 0b101 || tiltSensor == 0b001){
    tilt = 'A';
  }
  else{
    tilt = 'H';
  }
  //channel 1 to piano
  if(tilt == 'H'){
    return 1;
  }
  //channel 2 to sax
  else if(tilt == 'A'){
    return 2;
  }
  else{
    //Serial.println("No Channel Selected");
    return 0;
  }
}

// Function to produce key fade in piano mode
void fadeVolumeSax(int ith_Val, int zone, char whichKeypad){
  // manipulate the velocity for this
  // decrease faster if the velocity by a lot if the initial pressure is greater than following pressure
  // decrease slowly if the initial pressure is equal to, a little bit greater than or less than following pressure
  if(whichKeypad == 'L'){
    if(fsrStructL.initVolume > fsrStructL.currVolume){
      //decrease volume very fast
      for(int i=fsrStructL.initVolume; i>=0; i-=10)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
        //piano pitch bend////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joystickRead = constrain(map(dataSigReceivedL.joystick,0, 10230, 0, 16383), 0, 16383);
        if(joystickRead != joystickRead_last){
          midi.sendPitchBend(Channel_2, joystickRead);
        }
        joystickRead_last = joystickRead;       
        chooseNoteOn(ith_Val, zone, 'L');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructL.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, zone, 'L');
      }
    }
  }
  else{
    if(fsrStructR.initVolume > fsrStructR.currVolume){
      //decrease volume very fast
      for(int i=fsrStructR.initVolume; i>=0; i-=10)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
        
        //piano pitch bend///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        joystickRead = constrain(map(dataSigReceivedL.joystick,0, 10230, 0, 16383), 0, 16383);
        if(joystickRead != joystickRead_last){
          midi.sendPitchBend(Channel_2, joystickRead);
        }
        joystickRead_last = joystickRead;
       
        chooseNoteOn(ith_Val, zone, 'R');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructR.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, zone, 'R');
      }
    }
  }
  //update noteOnHis struct here
  noteOnSig.ith_Val[ith_Val] = 1;
  noteOnSig.whichKeypad[ith_Val] = whichKeypad;
}

// Function to determine appropriate notes keypad should produce dependent on value of rotary encoder
int saxOctave(int8_t tiltSensor, char whichKeypad){
  int16_t rE_L = dataSigReceivedL.rotEncoder;
  int16_t rE_R = dataSigReceivedR.rotEncoder;
  int maxRE = 385;
  int maxReRange = maxRE/6;

  if(whichKeypad == 'L'){
    if(rE_L <= maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::E(2), Channel_2};
      noteAddr2L = {41, Channel_2};
      noteAddr5L = {MIDI_Notes::G(2), Channel_2};
      noteAddr7L = {MIDI_Notes::A(2), Channel_2};
      noteAddr3L = {MIDI_Notes::Eb(2), Channel_2};
      noteAddr4L = {MIDI_Notes::Gb(2), Channel_2};
      noteAddr6L = {MIDI_Notes::Ab(2), Channel_2};
      Serial.println("Zone 1");
      return 1;
    }
    else if(rE_L >= maxReRange+1 && rE_L <= 2*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::B(2), Channel_2};
      noteAddr2L = {MIDI_Notes::C(3), Channel_2};
      noteAddr5L = {MIDI_Notes::D(3), Channel_2};
      noteAddr7L = {MIDI_Notes::E(3), Channel_2};
      noteAddr3L = {MIDI_Notes::Bb(2), Channel_2};
      noteAddr4L = {MIDI_Notes::Db(3), Channel_2};
      noteAddr6L = {MIDI_Notes::Eb(3), Channel_2};
      Serial.println("Zone 2");
      return 2;
    }
    else if(rE_L >= 2*maxReRange+1 && rE_L <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      //MIDI_Notes::note	(	int8_t 	note,int8_t 	numOctave )	;
      noteAddr1L = {53, Channel_2};
      //noteAddr1L = {53, Channel_2};
      noteAddr2L = {MIDI_Notes::G(3), Channel_2};
      noteAddr5L = {MIDI_Notes::A(3), Channel_2};
      noteAddr7L = {MIDI_Notes::B(3), Channel_2};
      noteAddr3L = {MIDI_Notes::Gb(3), Channel_2};
      noteAddr4L = {MIDI_Notes::Ab(3), Channel_2};
      noteAddr6L = {MIDI_Notes::Bb(3), Channel_2};
      Serial.println("Zone 3");
      return 3;
    }
    else if(rE_L >= 3*maxReRange+1 && rE_L <= 4*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::C(4), Channel_2};
      noteAddr2L = {MIDI_Notes::D(4), Channel_2};
      noteAddr5L = {MIDI_Notes::E(4), Channel_2};
      noteAddr7L = {65, Channel_2};
      noteAddr3L = {MIDI_Notes::Db(4), Channel_2};
      noteAddr4L = {MIDI_Notes::Eb(4), Channel_2};
      noteAddr6L = {MIDI_Notes::Gb(4), Channel_2};
      Serial.println("Zone 4");
      return 4;
    }
    else if(rE_L >= 4*maxReRange+1 && rE_L <= 5*maxReRange){
      noteAddr1L = {MIDI_Notes::G(4), Channel_2};
      noteAddr2L = {MIDI_Notes::A(4), Channel_2};
      noteAddr5L = {MIDI_Notes::B(4), Channel_2};
      noteAddr7L = {MIDI_Notes::C(5), Channel_2};
      noteAddr3L = {MIDI_Notes::Ab(4), Channel_2};
      noteAddr4L = {MIDI_Notes::Bb(4), Channel_2};
      noteAddr6L = {MIDI_Notes::Db(5), Channel_2};
      Serial.println("Zone 5");
      return 5;
    }
    else if(rE_L >= 5*maxReRange+1){
      noteAddr1L = {MIDI_Notes::D(5), Channel_2};
      noteAddr2L = {MIDI_Notes::E(5), Channel_2};
      noteAddr5L = {77, Channel_2};
      noteAddr7L = {MIDI_Notes::G(5), Channel_2};
      noteAddr3L = {MIDI_Notes::Eb(5), Channel_2};
      noteAddr4L = {MIDI_Notes::Gb(5), Channel_2};
      noteAddr6L = {MIDI_Notes::Ab(5), Channel_2};
      Serial.println("Zone 6");
      return 6;
    }
  }
  else{
    if(rE_R <= maxReRange){
      noteAddr1R = {MIDI_Notes::D(5), Channel_2};
      noteAddr3R = {MIDI_Notes::E(5), Channel_2};
      noteAddr5R = {77, Channel_2};
      noteAddr7R = {MIDI_Notes::G(5), Channel_2};
      noteAddr2R = {MIDI_Notes::Eb(5), Channel_2};
      noteAddr4R = {MIDI_Notes::Gb(5), Channel_2};
      noteAddr6R = {MIDI_Notes::Ab(5), Channel_2};
      Serial.println("R: Zone 6");
      return 16;
    }
    else if(rE_R >= maxReRange+1 && rE_R <= 2*maxReRange){
      noteAddr1R = {MIDI_Notes::G(4), Channel_2};
      noteAddr3R = {MIDI_Notes::A(4), Channel_2};
      noteAddr5R = {MIDI_Notes::B(4), Channel_2};
      noteAddr7R = {MIDI_Notes::C(5), Channel_2};
      noteAddr2R = {MIDI_Notes::Ab(4), Channel_2};
      noteAddr4R = {MIDI_Notes::Bb(4), Channel_2};
      noteAddr6R = {MIDI_Notes::Db(5), Channel_2};
      Serial.println("R: Zone 5");
      return 15;
    }
    else if(rE_R >= 2*maxReRange+1 && rE_R <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::C(4), Channel_2};
      noteAddr3R = {MIDI_Notes::D(4), Channel_2};
      noteAddr5R = {MIDI_Notes::E(4), Channel_2};
      noteAddr7R = {65, Channel_2};
      noteAddr2R = {MIDI_Notes::Db(4), Channel_2};
      noteAddr4R = {MIDI_Notes::Eb(4), Channel_2};
      noteAddr6R = {MIDI_Notes::Gb(4), Channel_2};
      Serial.println("R: Zone 4");
      return 14;
    }
    else if(rE_R >= 3*maxReRange+1 && rE_R <= 4*maxReRange){
      noteAddr1R = {53, Channel_2};
      noteAddr3R = {MIDI_Notes::G(3), Channel_2};
      noteAddr5R = {MIDI_Notes::A(3), Channel_2};
      noteAddr7R = {MIDI_Notes::B(3), Channel_2};
      noteAddr2R = {MIDI_Notes::Gb(3), Channel_2};
      noteAddr4R = {MIDI_Notes::Ab(3), Channel_2};
      noteAddr6R = {MIDI_Notes::Bb(3), Channel_2};
      Serial.println("R: Zone 3");
      return 13;
    }
    else if(rE_R >= 4*maxReRange+1 && rE_R <= 5*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::B(2), Channel_2};
      noteAddr3R = {MIDI_Notes::C(3), Channel_2};
      noteAddr5R = {MIDI_Notes::D(3), Channel_2};
      noteAddr7R = {MIDI_Notes::E(3), Channel_2};
      noteAddr2R = {MIDI_Notes::Bb(2), Channel_2};
      noteAddr4R = {MIDI_Notes::Db(3), Channel_2};
      noteAddr6R = {MIDI_Notes::Eb(3), Channel_2};
      Serial.println("R: Zone 2");
      return 12;
    }
    else if(rE_R >= 5*maxReRange+1){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::E(2), Channel_2};
      noteAddr3R = {41, Channel_2};
      noteAddr5R = {MIDI_Notes::G(2), Channel_2};
      noteAddr7R = {MIDI_Notes::A(2), Channel_2};
      noteAddr2R = {MIDI_Notes::Eb(2), Channel_2};
      noteAddr4R= {MIDI_Notes::Gb(2), Channel_2};
      noteAddr6R = {MIDI_Notes::Ab(2), Channel_2};
      Serial.println("R: Zone 1");
      return 11;
    }
  }
  return 0;
}

// Function turns sax on and off
void saxOnOff(int i, int zoneL, int zoneR){
  //Note: FSR for sax does not control volume, what controls volume is joystick////////////////////////////////////////
  if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume <= 24 && fsrStructL.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'L'){
      chooseNoteOff(i, zoneL, 'L');

      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';   
    }
    fsrStructL.turnOffFirstTime = 0;
    fsrStructL.firstTimeNoteOn = 1;
  }
  else if(fsrStructL.currVolume >= 25 && fsrStructL.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolumeSax(i, zoneL, 'L');
    fsrStructL.turnOffFirstTime = 1;
    fsrStructL.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("L: Do Nothing");
  }

  if(fsrStructR.prevVolume <= 24 && fsrStructR.currVolume <= 24 && fsrStructR.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'R'){
      chooseNoteOff(i, zoneR, 'R');
      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';  
    }
    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;
  }
  else if(fsrStructR.currVolume >= 25 && fsrStructR.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolumeSax(i, zoneR, 'R');
    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("R: Do Nothing");
  }
}


void initializeVol(){
  // Set Initial Volume
  if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume >= 25){
    fsrStructL.initVolume = fsrStructL.currVolume;
  }
  if(fsrStructR.prevVolume <= 24 && fsrStructR.currVolume >= 25){
    fsrStructR.initVolume = fsrStructR.currVolume;
  }
}

// Function to eliminate noise on tiltSensor
int tiltSensorNoise(){
  //read in values for tiltSensor
  int sensor_HValue = digitalRead(sensorHPin);
  int sensor_VValue = digitalRead(sensorVPin);
  int sensor_AValue = digitalRead(sensorAPin);
  //int tiltSensor[3] = {sensor_HValue, sensor_VValue, sensor_AValue};
  int tiltSensor = (sensor_HValue << 2) | (sensor_VValue << 1) | sensor_AValue;  //input tilt sensor value, not to be confused with the cmpTilt value to be used
  Serial.println("Tilt Sensor: "+ String(sensor_HValue)+String(sensor_VValue)+ String(sensor_AValue));

  //Update tiltSensHist with tiltSensor by shifting items to the left
  iteratetiltHist += 1;
  if(iteratetiltHist >= 9){iteratetiltHist = 0; }
  
  tiltSensHist[iteratetiltHist+1] = tiltSensHist[iteratetiltHist];
  tiltSensHist[0] = tiltSensor;

  cmpTilt = tiltSensor;
  //have a compare value which is essentially the currenttiltSensor value
  if(cmpTilt == tiltSensHist[iteratetiltHist]){
    sameValCount++;
  }
  else{ //store tiltSensHist element in another value
    secondTiltVal = tiltSensHist[iteratetiltHist];
  }
  
  if(sameValCount < 10 && (iteratetiltHist == 0)){ // if identical value is less than 5 update cmpTilt
    cmpTilt = secondTiltVal;
  }
  //Serial.println("tiltSensHist:"+String(tiltSensHist[0])+","+String(tiltSensHist[1])+","+String(tiltSensHist[2])+","+String(tiltSensHist[3])+","+String(tiltSensHist[4])+","+String(tiltSensHist[5])+","+String(tiltSensHist[6])+","+String(tiltSensHist[7])+","+String(tiltSensHist[8])+","+String(tiltSensHist[9]));
  //Serial.println("cmpTilt: "+String(cmpTilt));
  return cmpTilt;
}

uint8_t pipe1 = 1;  // pipe values may go from 0-6 and represents the individual 
uint8_t pipe2 = 2;  //  paths available for 1 nRF module to be able to receive data from
int iterateFlush = 0;

void setup(void){
  Serial.begin(9600);
  pinMode(sensorHPin, INPUT);
  pinMode(sensorVPin, INPUT);
  pinMode(sensorAPin, INPUT);
  //radio.flush_rx();
  radio.begin(); // Start the NRF24L01
  radio.openReadingPipe(1, 0xABCDEF12); //left keypad Tx
  radio.openReadingPipe(2, 0xABCDEF23); //right keypad Tx
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
  midi.begin(); //Initializes MIDI interface
  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  tiltSensorNoise();

  if(radio.available()){
    radio.read(&tempSig, sizeof(tempSig));
    //if left or right is 0, then copy all values from tempSig to dataSigReceivedL, else dataSigReceivedR
    if(tempSig.lOrR == 0){
      memcpy(&dataSigReceivedL, &tempSig, sizeof(tempSig));
    }
    else{
      memcpy(&dataSigReceivedR, &tempSig, sizeof(tempSig));
    }
    
    //dataSigReceivedL.rotEncoder = dataSigReceivedL.rotEncoder/4;
    //dataSigReceivedR.rotEncoder = dataSigReceivedR.rotEncoder/4;
    Serial.println("2. dataSigReceivedL: "+String(dataSigReceivedL.fsr0)+", "+String(dataSigReceivedL.fsr1)+", "+String(dataSigReceivedL.fsr2)+", "+String(dataSigReceivedL.fsr3)+", "+String(dataSigReceivedL.fsr4)+", "+String(dataSigReceivedL.fsr5)+", "+String(dataSigReceivedL.fsr6));
    Serial.println("2. dataSigReceivedR: "+String(dataSigReceivedR.fsr0)+", "+String(dataSigReceivedR.fsr1)+", "+String(dataSigReceivedR.fsr2)+", "+String(dataSigReceivedR.fsr3)+", "+String(dataSigReceivedR.fsr4)+", "+String(dataSigReceivedR.fsr5)+", "+String(dataSigReceivedR.fsr6));

    for(int8_t i=startingMuxSel; i<=(startingMuxSel + 0b0110); i++){
      //copy dataSigReceived to temp struct
      pickStruct(i);

      //copy fsr# sent from Tx and map it to a value before sending to currVolume
      fsrStructL.prevVolume = fsrStructL.currVolume;
      fsrStructR.prevVolume = fsrStructR.currVolume;
      fsrStructL.currVolume = mapFSRtoVolume(i, 'L');
      fsrStructR.currVolume = mapFSRtoVolume(i, 'R');

      // initalize volume: fsrStuct(L/R).initVolume;
      initializeVol();

      int16_t rE_L = dataSigReceivedL.rotEncoder;

      //only needs to be called once since both keypads should be on the same channel
      int which_channel = switchInstruments(tiltSensor, rE_L);
      Serial.println("Channel: "+String(which_channel));
      if(which_channel == 1){
        // set notes to keys here and return a zone to prevent B sharp from turning on
        int zoneL = pianoOctave(cmpTilt, 'L');
        int zoneR = pianoOctave(cmpTilt, 'R');
        // if note is released or just not pressed, then proceed, else turn note on
        pianoOnOff(i, zoneL, zoneR);
      }
      else if(which_channel == 2){
        //call function for sax...
        int sax_zoneL = saxOctave(cmpTilt, 'L');
        int sax_zoneR = saxOctave(cmpTilt, 'R');
        saxOnOff(i, sax_zoneL, sax_zoneR);
      }
      /*
      else if(which_channel == 2){
        //call function for bass guitar...
      }
      else if(which_channel == 3){
        //call function for flute...
      }
      else if(which_channel == 4){
        //call function for clarinet...
      }
      else if(which_channel == 5){
        //call function for sax...
      }
      else if(which_channel == 6){
        //call function for bassoon...
      }*/

      //call function to copy all values from fsrReading(L/R) to struct
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