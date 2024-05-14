/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 04/24/2024
  Purpose: This is the program to ran during the Expo(04/26/2024).
            The purpose of this file is to be uploaded to the receiver/base station.
            It takes in data signals sent from both transmitters/keypads to switch btw
            instruments in the synthesizer, have variable volume and polyphony.
  Notes:
*/

#include <SPI.h>
#include <RF24.h>
#include <Control_Surface.h>
#include <print.h>

#define SIZE 3  //size of volume history
#define TILTSIZE 10 //size of tilt sensor history
#define KEYNUM 7 //size of number of keys on a keypad
#define JOYSTICKUPPERLIMIT 560
#define JOYSTICKLOWERLIMIT 470

RF24 radio(9,10); // pin assignments for nRF CE,CSN
USBMIDI_Interface midi; //MIDI over USB interface object

//---------------------------------------Setup----------------------------------------------------------
const int sensorVPin = 5;
const int sensorAPin = 19;
const int sensorHPin = 20;
uint8_t pipe1 = 1;  // pipe values may go from 0-6 and represents the individual 
uint8_t pipe2 = 2;  //  paths available for 1 nRF module to be able to receive data from

//---------------------------------------Global Variables------------------------------------------------
const uint8_t velocity = 0x7F;
const uint8_t offvelocity = 0x00;
const uint8_t ccAddr = MIDI_CC::Channel_Volume;  //changes control channel to ChannelVolume

int8_t tiltSensor = 0b000; //global variables associated with tilt sensor noise elimination
int cmpTilt;
int8_t sameValCount = 0;
int secondTiltVal = 0;
int tiltSensHist[TILTSIZE]; //create an array with size 10 to collect tiltSensor History that updates with every loop
int8_t iteratetiltHist= -1;
int8_t i_collectData = 0;
int8_t rotPlateHist[2] = {0,0};
int8_t startingMuxSel = 0;
int16_t maxRE = 385;
int16_t maxReRange = maxRE/6;
int16_t borderValue_L = 25; //boundary for rotary encoder to switch between clarinet and sax

//---------------------------------------Structs and Initalization----------------------------------------
//Piano and Bass Guitar: Note Addr for each Key on the Keypad
MIDIAddress noteAddr1L;   //left most white key
MIDIAddress noteAddr2L;  
MIDIAddress noteAddr3L; 
MIDIAddress noteAddr4L;   //right most white key
MIDIAddress noteAddr5L;   //left most black key
MIDIAddress noteAddr6L;  
MIDIAddress noteAddr7L;   //right most black key
MIDIAddress noteAddr1R;   //left most white key
MIDIAddress noteAddr2R;  
MIDIAddress noteAddr3R; 
MIDIAddress noteAddr4R;   //right most white key
MIDIAddress noteAddr5R;   //left most black key
MIDIAddress noteAddr6R;  
MIDIAddress noteAddr7R;   //right most black key

//Sax Mode: Only 1 note addr needed for sax
MIDIAddress saxNoteAddr;
MIDIAddress clarinetNoteAddr;
MIDIAddress fluteNoteAddr;

// struct for 7 FSR data sent to base station
struct DataSig{
  bool lOrR;  
  uint16_t fsr0;       //Force Sensitive Resistor Values
  uint16_t fsr1;
  uint16_t fsr2;
  uint16_t fsr3;
  uint16_t fsr4;
  uint16_t fsr5;
  uint16_t fsr6;
  int16_t rotEncoder;  //Rotary Encoder value
  bool rotPlate1;      //Rotational Plate Sensors
  bool rotPlate2;
  bool rotPlate3;
  int16_t joystick;
};

//initalize struct for DataSig
struct DataSig dataSigReceivedL;
struct DataSig dataSigReceivedR;
struct DataSig tempSig;

// this struct holds variables for each key and determines which notes
// to send to the synthesizer
struct midiSig{   
  int sentfsrValue;
  int prevVolume;
  int currVolume;
  int initVolume;
  int turnOffFirstTime;
  int firstTimeNoteOn;
};

// initialize struct for individual keys
midiSig key1DataL = {0,0,0,0,1,1};
midiSig key2DataL = {0,0,0,0,1,1};
midiSig key3DataL = {0,0,0,0,1,1};
midiSig key4DataL = {0,0,0,0,1,1};
midiSig key5DataL = {0,0,0,0,1,1};
midiSig key6DataL = {0,0,0,0,1,1};
midiSig key7DataL = {0,0,0,0,1,1};
midiSig fsrStructL ={0,0,0,0,1,1};    //temp struct for left keys
midiSig key1DataR = {0,0,0,0,1,1};
midiSig key2DataR = {0,0,0,0,1,1};
midiSig key3DataR = {0,0,0,0,1,1};
midiSig key4DataR = {0,0,0,0,1,1};
midiSig key5DataR = {0,0,0,0,1,1};
midiSig key6DataR = {0,0,0,0,1,1};
midiSig key7DataR = {0,0,0,0,1,1};
midiSig fsrStructR = {0,0,0,0,1,1};    //temp struct for right keys

// a struct to keep track of which note is on at any given time
struct noteOnHis{
  bool ith_Val[KEYNUM];
  char whichKeypad[KEYNUM];
};
struct noteOnHis noteOnSig = {{0,0,0,0,0,0,0}, {'N','N','N','N','N','N','N'}};
using namespace MIDI_Notes; // Define the MIDI note names and their corresponding values

//---------------------------------------Functions Start Here------------------------------------------------
// Piano: Function to produce key fade in piano mode
void fadeVolume(int ith_Val, char whichKeypad){
  if(whichKeypad == 'L'){
    if(fsrStructL.initVolume > fsrStructL.currVolume){
      //decrease volume very fast
      for(int i=fsrStructL.initVolume; i>=0; i-=10)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume     
        chooseNoteOn(ith_Val, 'L');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructL.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, 'L');
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
        chooseNoteOn(ith_Val, 'R');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructR.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, 'R');
      }
    }
  }
  //update noteOnHis struct here
  noteOnSig.ith_Val[ith_Val] = 1;
  noteOnSig.whichKeypad[ith_Val] = whichKeypad;
}

// Piano: Function assigns which noteAddr gets turned on
void chooseNoteOn(int i, char whichKeypad){
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

// Piano: Function to turn sendNoteOff depending on i
void chooseNoteOff(int i, char whichKeypad){
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

// Function to eliminate noise on tiltSensor
int tiltSensorNoise(int8_t tiltSensor){
  //Update tiltSensHist with tiltSensor by shifting items to the left
  iteratetiltHist += 1;
  if(iteratetiltHist >= 9){iteratetiltHist = 0; }
  tiltSensHist[iteratetiltHist+1] = tiltSensHist[iteratetiltHist];
  tiltSensHist[0] = tiltSensor;

  cmpTilt = tiltSensor;
  if(cmpTilt == tiltSensHist[iteratetiltHist]){
    sameValCount++;
  }
  else{
    secondTiltVal = tiltSensHist[iteratetiltHist];
  }
  
  if(sameValCount < 10 && (iteratetiltHist == 0)){ // if identical value is less than 5 update cmpTilt
    cmpTilt = secondTiltVal;
  }
  return cmpTilt;
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

// Piano: Function to determine appropriate notes keypad should produce dependent on value of rotary encoder
void setPianoNotes(int8_t tiltSensor, char whichKeypad){
  int16_t rE_L = dataSigReceivedL.rotEncoder;
  int16_t rE_R = dataSigReceivedR.rotEncoder;

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
      //Serial.println("Zone 1");
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
      //Serial.println("Zone 2");
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
      //Serial.println("Zone 3");
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
      //Serial.println("Zone 4");
    }
    else if(rE_L >= 4*maxReRange+1 && rE_L <= 5*maxReRange){
      noteAddr1L = {MIDI_Notes::G(4), Channel_1};
      noteAddr2L = {MIDI_Notes::A(4), Channel_1};
      noteAddr5L = {MIDI_Notes::B(4), Channel_1};
      noteAddr7L = {MIDI_Notes::C(5), Channel_1};
      noteAddr3L = {MIDI_Notes::Ab(4), Channel_1};
      noteAddr4L = {MIDI_Notes::Bb(4), Channel_1};
      noteAddr6L = {MIDI_Notes::Db(5), Channel_1};
      //Serial.println("Zone 5");
    }
    else if(rE_L >= 5*maxReRange+1){
      noteAddr1L = {MIDI_Notes::D(5), Channel_1};
      noteAddr2L = {MIDI_Notes::E(5), Channel_1};
      noteAddr5L = {77, Channel_1};
      noteAddr7L = {MIDI_Notes::G(5), Channel_1};
      noteAddr3L = {MIDI_Notes::Eb(5), Channel_1};
      noteAddr4L = {MIDI_Notes::Gb(5), Channel_1};
      noteAddr6L = {MIDI_Notes::Ab(5), Channel_1};
      //Serial.println("Zone 6");
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
      //Serial.println("R: Zone 6");
    }
    else if(rE_R >= maxReRange+1 && rE_R <= 2*maxReRange){
      noteAddr1R = {MIDI_Notes::G(4), Channel_1};
      noteAddr3R = {MIDI_Notes::A(4), Channel_1};
      noteAddr5R = {MIDI_Notes::B(4), Channel_1};
      noteAddr7R = {MIDI_Notes::C(5), Channel_1};
      noteAddr2R = {MIDI_Notes::Ab(4), Channel_1};
      noteAddr4R = {MIDI_Notes::Bb(4), Channel_1};
      noteAddr6R = {MIDI_Notes::Db(5), Channel_1};
      //Serial.println("R: Zone 5");
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
      //Serial.println("R: Zone 4");
    }
    else if(rE_R >= 3*maxReRange+1 && rE_R <= 4*maxReRange){
      noteAddr1R = {53, Channel_1};
      noteAddr3R = {MIDI_Notes::G(3), Channel_1};
      noteAddr5R = {MIDI_Notes::A(3), Channel_1};
      noteAddr7R = {MIDI_Notes::B(3), Channel_1};
      noteAddr2R = {MIDI_Notes::Gb(3), Channel_1};
      noteAddr4R = {MIDI_Notes::Ab(3), Channel_1};
      noteAddr6R = {MIDI_Notes::Bb(3), Channel_1};
      //Serial.println("R: Zone 3");
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
      //Serial.println("R: Zone 2");
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
      //Serial.println("R: Zone 1");
    }
  }
}

// Piano: Function turns piano on and off
void pianoMode(int i){
  if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume <= 24 && fsrStructL.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'L'){
      chooseNoteOff(i,'L');

      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';   
    }
    fsrStructL.turnOffFirstTime = 0;
    fsrStructL.firstTimeNoteOn = 1;
  }
  else if(fsrStructL.currVolume >= 25 && fsrStructL.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolume(i, 'L');
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
      chooseNoteOff(i, 'R');
      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';  
    }
    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;
  }
  else if(fsrStructR.currVolume >= 25 && fsrStructR.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    fadeVolume(i, 'R');
    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("R: Do Nothing");
  }
}

// Piano: Function to set initial volume: if prev volume off, curr Volume on
void initializeVol(int channel){
  if(channel == 1 || channel == 6 || channel == 5){
    // Set Initial Volume
    if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume >= 25){
      fsrStructL.initVolume = fsrStructL.currVolume;
    }
    if(fsrStructR.prevVolume <= 24 && fsrStructR.currVolume >= 25){
      fsrStructR.initVolume = fsrStructR.currVolume;
    }
  }
  else if(channel == 2 || channel == 3 || channel == 4){//dataSigReceivedR.joystick >= 760 || dataSigReceivedR.joystick <= 280
    if((fsrStructR.prevVolume <= JOYSTICKUPPERLIMIT && fsrStructR.prevVolume >= JOYSTICKLOWERLIMIT) && (fsrStructR.currVolume >= JOYSTICKUPPERLIMIT || fsrStructR.currVolume <= JOYSTICKLOWERLIMIT)){
      fsrStructR.initVolume = fsrStructR.currVolume;
    }
    else{//initial vol stays exactly the same
    }
  }
}

// Piano: Function maps the values received to the volume range
int mapFSRtoVolume(int i, char whichKeypad){
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

// Sax: Function determines which note to turn on when in sax mode
void saxMode(){
  bool keys[14];
    keys[0] = (dataSigReceivedR.fsr0 >= 5) ? 1 : 0; //right keypad
    keys[1] = (dataSigReceivedR.fsr2 >= 5) ? 1 : 0;
    keys[2] = (dataSigReceivedR.fsr4 >= 5) ? 1 : 0;
    keys[3] = (dataSigReceivedR.fsr6 >= 5) ? 1 : 0;
    keys[4] = (dataSigReceivedR.fsr1 >= 5) ? 1 : 0;
    keys[5] = (dataSigReceivedR.fsr3 >= 5) ? 1 : 0;
    keys[6] = (dataSigReceivedR.fsr5 >= 5) ? 1 : 0;
    keys[7] =  (dataSigReceivedL.fsr0 >= 5) ? 1 : 0; //left keypad
    keys[8] =  (dataSigReceivedL.fsr1 >= 5) ? 1 : 0;
    keys[9] =  (dataSigReceivedL.fsr4 >= 5) ? 1 : 0;
    keys[10] = (dataSigReceivedL.fsr6 >= 5) ? 1 : 0;
    keys[11] = (dataSigReceivedL.fsr2 >= 5) ? 1 : 0;
    keys[12] = (dataSigReceivedL.fsr3 >= 5) ? 1 : 0;
    keys[13] = (dataSigReceivedL.fsr5 >= 5) ? 1 : 0;

  Serial.println("Right Keys[]: "+String(keys[0])+","+String(keys[1])+","+String(keys[2])+","+String(keys[3])+","+String(keys[4])+","+String(keys[5])+","+String(keys[6]));
  Serial.println("Left Keys[]: "+String(keys[7])+","+String(keys[8])+","+String(keys[9])+","+String(keys[10])+","+String(keys[11])+","+String(keys[12])+","+String(keys[13]));
  //Serial.println("if condition result: "+String(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && keys[9]));
  if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && keys[9]){
    //Bb3 to Eb4, D5 and Eb5
    if(keys[13]){
      //Bb3 to Db4
      if(keys[0]){
        saxNoteAddr = {MIDI_Notes::Bb(3), Channel_2};
      }
      else if(keys[4]){
        saxNoteAddr = {MIDI_Notes::Db(4), Channel_2};     
      }
      else{
        saxNoteAddr = {MIDI_Notes::C(4), Channel_2};   
      }
    }
    else if(keys[10]){
      //Eb4, Eb5
      if(!keys[6]){
        saxNoteAddr = {MIDI_Notes::Eb(4), Channel_2}; 
      }
      else{
        saxNoteAddr = {MIDI_Notes::Eb(5), Channel_2}; 
      }
    }
    else{
      //D4,D5
      if(!keys[6]){
        saxNoteAddr = {MIDI_Notes::D(4), Channel_2}; 
      }
      else{
        saxNoteAddr = {MIDI_Notes::D(5), Channel_2}; 
      }
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && !keys[9]  && !keys[0] && !keys[10]){
    //E4,E5
    if(!keys[6]){
      saxNoteAddr = {MIDI_Notes::E(4), Channel_2}; 
    }
    else{
      saxNoteAddr = {MIDI_Notes::E(5), Channel_2}; 
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && !keys[8] && !keys[9] ){
    //F4,F5
    if(!keys[6]){
      saxNoteAddr = {65, Channel_2}; 
    }
    else{
      saxNoteAddr = {77, Channel_2}; 
    }
  }
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && keys[8] && !keys[9]){
    //Gb4,Gb5
    if(!keys[6]){
      saxNoteAddr = {MIDI_Notes::Gb(4), Channel_2}; 
    }
    else{
      saxNoteAddr = {MIDI_Notes::Gb(5), Channel_2}; 
    }
  } 
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //G4,Ab4,G5,Ab5
    if(keys[0]){
      if(keys[6]){
        saxNoteAddr = {MIDI_Notes::Ab(5), Channel_2}; 
      }
      else{
        saxNoteAddr = {MIDI_Notes::Ab(4), Channel_2}; 
      }
    }
    else{
      if(keys[6]){
        saxNoteAddr = {MIDI_Notes::G(5), Channel_2}; 
      }
      else{
        saxNoteAddr = {MIDI_Notes::G(4), Channel_2}; 
      }
    }
  }
  else if(keys[3] && keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //A4,A5
    if(!keys[6]){
      saxNoteAddr = {MIDI_Notes::A(4), Channel_2}; 
    }
    else{
      saxNoteAddr = {MIDI_Notes::A(5), Channel_2}; 
    }
  }
  else if(keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //B4,B5
    if(keys[6]){
      if(keys[5]){
        saxNoteAddr = {MIDI_Notes::Bb(5), Channel_2}; 
      }
      else{
        saxNoteAddr = {MIDI_Notes::B(5), Channel_2}; 
      }
    }
    else{
      saxNoteAddr = {MIDI_Notes::B(4), Channel_2}; 
    }
  }
  else if(!keys[3] && keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //C5,C6
    if(!keys[6]){
      saxNoteAddr = {MIDI_Notes::C(5), Channel_2}; 
    }
    else{
      saxNoteAddr = {MIDI_Notes::C(6), Channel_2}; 
    }
  }
  else if(!keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //Db5,Db6
    if(!keys[6]){
      saxNoteAddr = {MIDI_Notes::Db(5), Channel_2}; 
    }
    else{
      saxNoteAddr = {MIDI_Notes::Db(6), Channel_2}; 
    }
  }

  //call function to send notes on/off
  saxSendNotes();
}

// Sax: Function signals synthesizer to turn sax note w/ variable volume on when right joystick is pressed
void saxSendNotes(){
  bool turnOff = ((fsrStructR.prevVolume <= JOYSTICKUPPERLIMIT && fsrStructR.prevVolume >= JOYSTICKLOWERLIMIT)&&(fsrStructR.currVolume <= JOYSTICKUPPERLIMIT && fsrStructR.currVolume >= JOYSTICKLOWERLIMIT)) ? 1:0;
  bool turnOn  = (fsrStructR.currVolume >= JOYSTICKUPPERLIMIT || fsrStructR.currVolume <= JOYSTICKLOWERLIMIT) ? 1:0;

  if(turnOff && fsrStructR.turnOffFirstTime == 1){
    //turn note off 
    midi.sendNoteOff(saxNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;    
  }
  else if(turnOn && fsrStructR.firstTimeNoteOn == 1){
    int16_t mapped_joystick = 0;
    if(dataSigReceivedR.joystick >= JOYSTICKUPPERLIMIT){
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKUPPERLIMIT, 1023, 20, 127), 20, 127);
    }
    else{
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKLOWERLIMIT, 0, 20, 127), 20, 127);
    }
    //Serial.println("Mapped Joystick: "+String(mapped_joystick));

    midi.sendControlChange(ccAddr, mapped_joystick); //control volume then send note on

    // Send pitch bend message
    int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
    pitchBendValue = constrain(pitchBendValue, 0, 16383); // Ensure the value stays within bounds
    midi.sendPitchBend(Channel_2, pitchBendValue);
                        
    midi.sendNoteOn(saxNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
}

// Clarinet: Function determines which note to turn on when in clarinet mode
void clarinetMode(){
  bool keys[14];
    keys[0] = (dataSigReceivedR.fsr0 >= 5) ? 1 : 0; //right keypad
    keys[1] = (dataSigReceivedR.fsr2 >= 5) ? 1 : 0;
    keys[2] = (dataSigReceivedR.fsr4 >= 5) ? 1 : 0;
    keys[3] = (dataSigReceivedR.fsr6 >= 5) ? 1 : 0;
    keys[4] = (dataSigReceivedR.fsr1 >= 5) ? 1 : 0;
    keys[5] = (dataSigReceivedR.fsr3 >= 5) ? 1 : 0;
    keys[6] = (dataSigReceivedR.fsr5 >= 5) ? 1 : 0;
    keys[7] =  (dataSigReceivedL.fsr0 >= 5) ? 1 : 0; //left keypad
    keys[8] =  (dataSigReceivedL.fsr1 >= 5) ? 1 : 0;
    keys[9] =  (dataSigReceivedL.fsr4 >= 5) ? 1 : 0;
    keys[10] = (dataSigReceivedL.fsr6 >= 5) ? 1 : 0;
    keys[11] = (dataSigReceivedL.fsr2 >= 5) ? 1 : 0;
    keys[12] = (dataSigReceivedL.fsr3 >= 5) ? 1 : 0;
    keys[13] = (dataSigReceivedL.fsr5 >= 5) ? 1 : 0;

  Serial.println("Right Keys[]: "+String(keys[0])+","+String(keys[1])+","+String(keys[2])+","+String(keys[3])+","+String(keys[4])+","+String(keys[5])+","+String(keys[6]));
  Serial.println("Left Keys[]: "+String(keys[7])+","+String(keys[8])+","+String(keys[9])+","+String(keys[10])+","+String(keys[11])+","+String(keys[12])+","+String(keys[13]));
  
  // if statements to change clarinetNoteAddr
  if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //G3
      clarinetNoteAddr = {MIDI_Notes::G(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //E3
      clarinetNoteAddr = {MIDI_Notes::E(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && keys[13] && !keys[5] && !keys[6] && keys[0]){
      //F3
      clarinetNoteAddr = {53, Channel_3};
    }
    else if(!keys[10] && keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Gb3
      clarinetNoteAddr = {MIDI_Notes::Gb(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //B4
      clarinetNoteAddr = {MIDI_Notes::B(4), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && keys[13] && keys[5] && !keys[6] && keys[0]){
      //C5
      clarinetNoteAddr = {MIDI_Notes::C(5), Channel_3};
    }
    else if(keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Db5
      clarinetNoteAddr = {MIDI_Notes::Db(5), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //D5
      clarinetNoteAddr = {MIDI_Notes::D(5), Channel_3};
    }
    else if(!keys[10] && keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Eb5
      clarinetNoteAddr = {MIDI_Notes::Eb(5), Channel_3};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //A3
      clarinetNoteAddr = {MIDI_Notes::A(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //E5
      clarinetNoteAddr = {MIDI_Notes::E(5), Channel_3};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Bb3
      clarinetNoteAddr = {MIDI_Notes::Bb(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //F5
      clarinetNoteAddr = {77, Channel_3};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //B3
      clarinetNoteAddr = {MIDI_Notes::B(3), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Gb5
      clarinetNoteAddr = {MIDI_Notes::Gb(5), Channel_3};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //C4
      clarinetNoteAddr = {MIDI_Notes::C(4), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //G5
      clarinetNoteAddr = {MIDI_Notes::G(5), Channel_3};
    }
  }
  else if(keys[3] && keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //D4
      clarinetNoteAddr = {MIDI_Notes::D(4), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //A5
      clarinetNoteAddr = {MIDI_Notes::A(5), Channel_3};
    }
  }
  else if(keys[3] && !keys[2] && !keys[1]){
    if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //E4
      clarinetNoteAddr = {MIDI_Notes::E(4), Channel_3};
    }
    else if(keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Eb4
      clarinetNoteAddr = {MIDI_Notes::Eb(4), Channel_3};
    }
    else if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && !keys[0]){
      //Gb4
      clarinetNoteAddr = {MIDI_Notes::Gb(4), Channel_3};
    }
    else if(keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Bb5
      clarinetNoteAddr = {MIDI_Notes::Bb(5), Channel_3};
    }
    else if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //B5
      clarinetNoteAddr = {MIDI_Notes::B(5), Channel_3};
    }
  }
  else if(!keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && keys[6] && !keys[0]){
      //A4
      clarinetNoteAddr = {MIDI_Notes::A(4), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && keys[6] && !keys[0]){
      //Bb4
      clarinetNoteAddr = {MIDI_Notes::Bb(4), Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //F4
      clarinetNoteAddr = {65, Channel_3};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //C6
      clarinetNoteAddr = {MIDI_Notes::C(6), Channel_3};
    }
  }
  else if(!keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Db6
      clarinetNoteAddr = {MIDI_Notes::Db(6), Channel_3};
    }
  }
  else if(!keys[3] && keys[2] && keys[1] && keys[7] && !keys[8] && !keys[9]){
    //D6
    clarinetNoteAddr = {MIDI_Notes::D(6), Channel_3};
  }
  else if(!keys[3] && keys[2] && keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //E6
    clarinetNoteAddr = {MIDI_Notes::E(6), Channel_3};
  }
  else if(!keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //G4
    clarinetNoteAddr = {MIDI_Notes::G(4), Channel_3};
  }
  //call function to send notes on/off
  clarinetSendNotes();
}

// Clarinet: Function signals synthesizer to turn clarinet note w/ variable volume on when right joystick is pressed
void clarinetSendNotes(){
  bool turnOff = ((fsrStructR.prevVolume <= JOYSTICKUPPERLIMIT && fsrStructR.prevVolume >= JOYSTICKLOWERLIMIT)&&(fsrStructR.currVolume <= JOYSTICKUPPERLIMIT && fsrStructR.currVolume >= JOYSTICKLOWERLIMIT)) ? 1:0;
  bool turnOn  = (fsrStructR.currVolume >= JOYSTICKUPPERLIMIT || fsrStructR.currVolume <= JOYSTICKLOWERLIMIT) ? 1:0;

  if(turnOff && fsrStructR.turnOffFirstTime == 1){
    //turn note off 
    midi.sendNoteOff(clarinetNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;    
  }
  else if(turnOn && fsrStructR.firstTimeNoteOn == 1){
    int16_t mapped_joystick = 0;
    if(dataSigReceivedR.joystick >= JOYSTICKUPPERLIMIT){
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKUPPERLIMIT, 1023, 20, 127), 20, 127);
    }
    else{
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKLOWERLIMIT, 0, 20, 127), 20, 127);
    }
    Serial.println("Mapped Joystick: "+String(mapped_joystick));

    midi.sendControlChange(ccAddr, mapped_joystick); //control volume then send note on

    // Send pitch bend message
    int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
    pitchBendValue = constrain(pitchBendValue, 0, 16383); // Ensure the value stays within bounds
    midi.sendPitchBend(Channel_3, pitchBendValue);
                        
    midi.sendNoteOn(clarinetNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
}

// Flute: Function determines which note to turn on when in flute mode
void fluteMode(){
  bool keys[14];
    keys[0] = (dataSigReceivedR.fsr0 >= 5) ? 1 : 0; //right keypad
    keys[1] = (dataSigReceivedR.fsr2 >= 5) ? 1 : 0;
    keys[2] = (dataSigReceivedR.fsr4 >= 5) ? 1 : 0;
    keys[3] = (dataSigReceivedR.fsr6 >= 5) ? 1 : 0;
    keys[4] = (dataSigReceivedR.fsr1 >= 5) ? 1 : 0;
    keys[5] = (dataSigReceivedR.fsr3 >= 5) ? 1 : 0;
    keys[6] = (dataSigReceivedR.fsr5 >= 5) ? 1 : 0;
    keys[7] =  (dataSigReceivedL.fsr0 >= 5) ? 1 : 0; //left keypad
    keys[8] =  (dataSigReceivedL.fsr1 >= 5) ? 1 : 0;
    keys[9] =  (dataSigReceivedL.fsr4 >= 5) ? 1 : 0;
    keys[10] = (dataSigReceivedL.fsr6 >= 5) ? 1 : 0;
    keys[11] = (dataSigReceivedL.fsr2 >= 5) ? 1 : 0;
    keys[12] = (dataSigReceivedL.fsr3 >= 5) ? 1 : 0;
    keys[13] = (dataSigReceivedL.fsr5 >= 5) ? 1 : 0;

  Serial.println("Left Keys[]: "+String(keys[7])+","+String(keys[8])+","+String(keys[9])+","+String(keys[10])+","+String(keys[11])+","+String(keys[12])+","+String(keys[13]));
  Serial.println("Right Keys[]: "+String(keys[0])+","+String(keys[1])+","+String(keys[2])+","+String(keys[3])+","+String(keys[4])+","+String(keys[5])+","+String(keys[6]));
  
  // if statements to change fluteNoteAddr
  if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //G3
      fluteNoteAddr = {MIDI_Notes::G(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //E3
      fluteNoteAddr = {MIDI_Notes::E(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && keys[13] && !keys[5] && !keys[6] && keys[0]){
      //F3
      fluteNoteAddr = {53, Channel_4};
    }
    else if(!keys[10] && keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Gb3
      fluteNoteAddr = {MIDI_Notes::Gb(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //B4
      fluteNoteAddr = {MIDI_Notes::B(4), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && keys[13] && keys[5] && !keys[6] && keys[0]){
      //C5
      fluteNoteAddr = {MIDI_Notes::C(5), Channel_4};
    }
    else if(keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Db5
      fluteNoteAddr = {MIDI_Notes::Db(5), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //D5
      fluteNoteAddr = {MIDI_Notes::D(5), Channel_4};
    }
    else if(!keys[10] && keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Eb5
      fluteNoteAddr = {MIDI_Notes::Eb(5), Channel_4};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //A3
      fluteNoteAddr = {MIDI_Notes::A(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //E5
      fluteNoteAddr = {MIDI_Notes::E(5), Channel_4};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Bb3
      fluteNoteAddr = {MIDI_Notes::Bb(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //F5
      fluteNoteAddr = {77, Channel_4};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //B3
      fluteNoteAddr = {MIDI_Notes::B(3), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Gb5
      fluteNoteAddr = {MIDI_Notes::Gb(5), Channel_4};
    }
  }
  else if(keys[3] && keys[2] && keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //C4
      fluteNoteAddr = {MIDI_Notes::C(4), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //G5
      fluteNoteAddr = {MIDI_Notes::G(5), Channel_4};
    }
  }
  else if(keys[3] && keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //D4
      fluteNoteAddr = {MIDI_Notes::D(4), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //A5
      fluteNoteAddr = {MIDI_Notes::A(5), Channel_4};
    }
  }
  else if(keys[3] && !keys[2] && !keys[1]){
    if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //E4
      fluteNoteAddr = {MIDI_Notes::E(4), Channel_4};
    }
    else if(keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //Eb4
      fluteNoteAddr = {MIDI_Notes::Eb(4), Channel_4};
    }
    else if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && !keys[0]){
      //Gb4
      fluteNoteAddr = {MIDI_Notes::Gb(4), Channel_4};
    }
    else if(keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Bb5
      fluteNoteAddr = {MIDI_Notes::Bb(5), Channel_4};
    }
    else if(!keys[7] && !keys[8] && !keys[9] && !keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //B5
      fluteNoteAddr = {MIDI_Notes::B(5), Channel_4};
    }
  }
  else if(!keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && keys[6] && !keys[0]){
      //A4
      fluteNoteAddr = {MIDI_Notes::A(4), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && keys[6] && !keys[0]){
      //Bb4
      fluteNoteAddr = {MIDI_Notes::Bb(4), Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && !keys[5] && !keys[6] && keys[0]){
      //F4
      fluteNoteAddr = {65, Channel_4};
    }
    else if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //C6
      fluteNoteAddr = {MIDI_Notes::C(6), Channel_4};
    }
  }
  else if(!keys[3] && keys[2] && keys[1] && keys[7] && keys[8] && !keys[9]){
    if(!keys[10] && !keys[11] && !keys[12] && !keys[13] && keys[5] && !keys[6] && keys[0]){
      //Db6
      fluteNoteAddr = {MIDI_Notes::Db(6), Channel_4};
    }
  }
  else if(!keys[3] && keys[2] && keys[1] && keys[7] && !keys[8] && !keys[9]){
    //D6
    fluteNoteAddr = {MIDI_Notes::D(6), Channel_4};
  }
  else if(!keys[3] && keys[2] && keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //E6
    fluteNoteAddr = {MIDI_Notes::E(6), Channel_4};
  }
  else if(!keys[3] && !keys[2] && !keys[1] && !keys[7] && !keys[8] && !keys[9]){
    //G4
    fluteNoteAddr = {MIDI_Notes::G(4), Channel_4};
  }
  //call function to send notes on/off
  fluteSendNotes();
 
}

// Flute: Function signals synthesizer to turn flute note w/ variable volume on when right joystick is pressed
void fluteSendNotes(){
  bool turnOff = ((fsrStructR.prevVolume <= JOYSTICKUPPERLIMIT && fsrStructR.prevVolume >= JOYSTICKLOWERLIMIT)&&(fsrStructR.currVolume <= JOYSTICKUPPERLIMIT && fsrStructR.currVolume >= JOYSTICKLOWERLIMIT)) ? 1:0;
  bool turnOn  = (fsrStructR.currVolume >= JOYSTICKUPPERLIMIT || fsrStructR.currVolume <= JOYSTICKLOWERLIMIT) ? 1:0;

  if(turnOff && fsrStructR.turnOffFirstTime == 1){
    //turn note off 
    midi.sendNoteOff(fluteNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;    
  }
  else if(turnOn && fsrStructR.firstTimeNoteOn == 1){
    int16_t mapped_joystick = 0;
    if(dataSigReceivedR.joystick >= JOYSTICKUPPERLIMIT){
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKUPPERLIMIT, 1023, 20, 127), 20, 127);
    }
    else{
      mapped_joystick = constrain(map(dataSigReceivedR.joystick, JOYSTICKLOWERLIMIT, 0, 20, 127), 20, 127);
    }
    Serial.println("Mapped Joystick: "+String(mapped_joystick));

    midi.sendControlChange(ccAddr, mapped_joystick); //control volume then send note on

    // Send pitch bend message
    int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
    pitchBendValue = constrain(pitchBendValue, 0, 16383); // Ensure the value stays within bounds
    midi.sendPitchBend(Channel_4, pitchBendValue);
                        
    midi.sendNoteOn(fluteNoteAddr, velocity);

    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
}

// Bass Guitar: Function determines which note to send
void setBassNotes(){
  int16_t rE_R = dataSigReceivedR.rotEncoder;

  // if zone1 then set notes for right keypad noteAddr1R-noteAddr4R values  
  bool zone1 = (rE_R <= maxReRange) ? 1 : 0;
  bool zone2 = (rE_R >= maxReRange+1 && rE_R <= 2*maxReRange) ? 1 : 0;
  bool zone3 = (rE_R >= 2*maxReRange+1 && rE_R <= 3*maxReRange) ? 1 : 0;
  bool zone4 = (rE_R >= 3*maxReRange+1 && rE_R <= 4*maxReRange) ? 1 : 0;

  if(zone1){  //top of the rod
    noteAddr1R = {65, Channel_6};
    noteAddr3R = {MIDI_Notes::Bb(4), Channel_6};
    noteAddr5R = {MIDI_Notes::Eb(4), Channel_6};
    noteAddr7R = {MIDI_Notes::Ab(4), Channel_6};
  }
  else if(zone2){
    noteAddr1R = {MIDI_Notes::Gb(4), Channel_6};
    noteAddr3R = {MIDI_Notes::B(4), Channel_6};
    noteAddr5R = {MIDI_Notes::E(4), Channel_6};
    noteAddr7R = {MIDI_Notes::A(4), Channel_6};
  }
  else if(zone3){
    noteAddr1R = {MIDI_Notes::G(4), Channel_6};
    noteAddr3R = {MIDI_Notes::C(4), Channel_6};
    noteAddr5R = {77, Channel_6};
    noteAddr7R = {MIDI_Notes::Bb(5), Channel_6};
  }
  else if(zone4){
    noteAddr1R = {MIDI_Notes::Ab(5), Channel_6};
    noteAddr3R = {MIDI_Notes::Db(5), Channel_6};
    noteAddr5R = {MIDI_Notes::Gb(5), Channel_6};
    noteAddr7R = {MIDI_Notes::B(5), Channel_6};
  }
  else{
    noteAddr1R = {MIDI_Notes::A(5), Channel_6};
    noteAddr3R = {MIDI_Notes::D(5), Channel_6};
    noteAddr5R = {MIDI_Notes::G(5), Channel_6};
    noteAddr7R = {MIDI_Notes::C(5), Channel_6};
  }

  //Serial.println("Bass Guitar Notes Set");
}

// Bass Guitar
void bassFadeVolume(int ith_Val){
  // Pitch Bend
  int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
  pitchBendValue = constrain(pitchBendValue, 0, 16383); 
  midi.sendPitchBend(Channel_6, pitchBendValue);

  if(fsrStructR.initVolume > fsrStructR.currVolume){
    //decrease volume very fast
    for(int i=fsrStructR.initVolume; i>=0; i-=10)
    {
      midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                    // would be sending the prev. volume
      chooseNoteOn(ith_Val, 'R');
    }
  }
  else{
    // decrease volume slowly
    for(int i=fsrStructR.initVolume; i>=0; i-=5)
    {
      midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                      // would be sending the prev. volume
      chooseNoteOn(ith_Val, 'R');
    }
  }
  //update noteOnHis struct here
  noteOnSig.ith_Val[ith_Val] = 1;
  noteOnSig.whichKeypad[ith_Val] = 'R';
}

// Bass Guitar: Function determines which note is turned on or off
void bassGuitarMode(int i){
  if(fsrStructR.prevVolume <= 24 && fsrStructR.currVolume <= 24 && fsrStructR.turnOffFirstTime == 1)
  {
    //Serial.println("Bass Guitar Note Off");
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'R'){
      chooseNoteOff(i,'R');

      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';   
    }
    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;
  }
  else if(fsrStructR.currVolume >= 25 && fsrStructR.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    //Serial.println("Bass Guitar Note On");
    bassFadeVolume(i);
    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
  else{ 
    //Serial.println("Bass Guitar Skips");
  }
}

// Chinese Piano
void setCPianoNotes(int8_t tiltSensor, char whichKeypad){
  int16_t rE_L = dataSigReceivedL.rotEncoder;
  int16_t rE_R = dataSigReceivedR.rotEncoder;

  if(whichKeypad == 'L'){
    if(rE_L <= maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::E(2), Channel_5};
      noteAddr2L = {41, Channel_5};
      noteAddr5L = {MIDI_Notes::G(2), Channel_5};
      noteAddr7L = {MIDI_Notes::A(2), Channel_5};
      noteAddr3L = {MIDI_Notes::Eb(2), Channel_5};
      noteAddr4L = {MIDI_Notes::Gb(2), Channel_5};
      noteAddr6L = {MIDI_Notes::Ab(2), Channel_5};
      //Serial.println("Zone 1");
    }
    else if(rE_L >= maxReRange+1 && rE_L <= 2*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::B(2), Channel_5};
      noteAddr2L = {MIDI_Notes::C(3), Channel_5};
      noteAddr5L = {MIDI_Notes::D(3), Channel_5};
      noteAddr7L = {MIDI_Notes::E(3), Channel_5};
      noteAddr3L = {MIDI_Notes::Bb(2), Channel_5};
      noteAddr4L = {MIDI_Notes::Db(3), Channel_5};
      noteAddr6L = {MIDI_Notes::Eb(3), Channel_5};
      //Serial.println("Zone 2");
    }
    else if(rE_L >= 2*maxReRange+1 && rE_L <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      //MIDI_Notes::note	(	int8_t 	note,int8_t 	numOctave )	;
      noteAddr1L = {53, Channel_5};
      //noteAddr1L = {53, Channel_5};
      noteAddr2L = {MIDI_Notes::G(3), Channel_5};
      noteAddr5L = {MIDI_Notes::A(3), Channel_5};
      noteAddr7L = {MIDI_Notes::B(3), Channel_5};
      noteAddr3L = {MIDI_Notes::Gb(3), Channel_5};
      noteAddr4L = {MIDI_Notes::Ab(3), Channel_5};
      noteAddr6L = {MIDI_Notes::Bb(3), Channel_5};
      //Serial.println("Zone 3");
    }
    else if(rE_L >= 3*maxReRange+1 && rE_L <= 4*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1L = {MIDI_Notes::C(4), Channel_5};
      noteAddr2L = {MIDI_Notes::D(4), Channel_5};
      noteAddr5L = {MIDI_Notes::E(4), Channel_5};
      noteAddr7L = {65, Channel_5};
      noteAddr3L = {MIDI_Notes::Db(4), Channel_5};
      noteAddr4L = {MIDI_Notes::Eb(4), Channel_5};
      noteAddr6L = {MIDI_Notes::Gb(4), Channel_5};
      //Serial.println("Zone 4");
    }
    else if(rE_L >= 4*maxReRange+1 && rE_L <= 5*maxReRange){
      noteAddr1L = {MIDI_Notes::G(4), Channel_5};
      noteAddr2L = {MIDI_Notes::A(4), Channel_5};
      noteAddr5L = {MIDI_Notes::B(4), Channel_5};
      noteAddr7L = {MIDI_Notes::C(5), Channel_5};
      noteAddr3L = {MIDI_Notes::Ab(4), Channel_5};
      noteAddr4L = {MIDI_Notes::Bb(4), Channel_5};
      noteAddr6L = {MIDI_Notes::Db(5), Channel_5};
      //Serial.println("Zone 5");
    }
    else if(rE_L >= 5*maxReRange+1){
      noteAddr1L = {MIDI_Notes::D(5), Channel_5};
      noteAddr2L = {MIDI_Notes::E(5), Channel_5};
      noteAddr5L = {77, Channel_5};
      noteAddr7L = {MIDI_Notes::G(5), Channel_5};
      noteAddr3L = {MIDI_Notes::Eb(5), Channel_5};
      noteAddr4L = {MIDI_Notes::Gb(5), Channel_5};
      noteAddr6L = {MIDI_Notes::Ab(5), Channel_5};
      //Serial.println("Zone 6");
    }
  }
  else{
    if(rE_R <= maxReRange){
      noteAddr1R = {MIDI_Notes::D(5), Channel_5};
      noteAddr3R = {MIDI_Notes::E(5), Channel_5};
      noteAddr5R = {77, Channel_5};
      noteAddr7R = {MIDI_Notes::G(5), Channel_5};
      noteAddr2R = {MIDI_Notes::Eb(5), Channel_5};
      noteAddr4R = {MIDI_Notes::Gb(5), Channel_5};
      noteAddr6R = {MIDI_Notes::Ab(5), Channel_5};
      //Serial.println("R: Zone 6");
    }
    else if(rE_R >= maxReRange+1 && rE_R <= 2*maxReRange){
      noteAddr1R = {MIDI_Notes::G(4), Channel_5};
      noteAddr3R = {MIDI_Notes::A(4), Channel_5};
      noteAddr5R = {MIDI_Notes::B(4), Channel_5};
      noteAddr7R = {MIDI_Notes::C(5), Channel_5};
      noteAddr2R = {MIDI_Notes::Ab(4), Channel_5};
      noteAddr4R = {MIDI_Notes::Bb(4), Channel_5};
      noteAddr6R = {MIDI_Notes::Db(5), Channel_5};
      //Serial.println("R: Zone 5");
    }
    else if(rE_R >= 2*maxReRange+1 && rE_R <= 3*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::C(4), Channel_5};
      noteAddr3R = {MIDI_Notes::D(4), Channel_5};
      noteAddr5R = {MIDI_Notes::E(4), Channel_5};
      noteAddr7R = {65, Channel_5};
      noteAddr2R = {MIDI_Notes::Db(4), Channel_5};
      noteAddr4R = {MIDI_Notes::Eb(4), Channel_5};
      noteAddr6R = {MIDI_Notes::Gb(4), Channel_5};
      //Serial.println("R: Zone 4");
    }
    else if(rE_R >= 3*maxReRange+1 && rE_R <= 4*maxReRange){
      noteAddr1R = {53, Channel_5};
      noteAddr3R = {MIDI_Notes::G(3), Channel_5};
      noteAddr5R = {MIDI_Notes::A(3), Channel_5};
      noteAddr7R = {MIDI_Notes::B(3), Channel_5};
      noteAddr2R = {MIDI_Notes::Gb(3), Channel_5};
      noteAddr4R = {MIDI_Notes::Ab(3), Channel_5};
      noteAddr6R = {MIDI_Notes::Bb(3), Channel_5};
      //Serial.println("R: Zone 3");
    }
    else if(rE_R >= 4*maxReRange+1 && rE_R <= 5*maxReRange){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::B(2), Channel_5};
      noteAddr3R = {MIDI_Notes::C(3), Channel_5};
      noteAddr5R = {MIDI_Notes::D(3), Channel_5};
      noteAddr7R = {MIDI_Notes::E(3), Channel_5};
      noteAddr2R = {MIDI_Notes::Bb(2), Channel_5};
      noteAddr4R = {MIDI_Notes::Db(3), Channel_5};
      noteAddr6R = {MIDI_Notes::Eb(3), Channel_5};
      //Serial.println("R: Zone 2");
    }
    else if(rE_R >= 5*maxReRange+1){
      //set note for fsr6 to C and fsr0 to F
      noteAddr1R = {MIDI_Notes::E(2), Channel_5};
      noteAddr3R = {41, Channel_5};
      noteAddr5R = {MIDI_Notes::G(2), Channel_5};
      noteAddr7R = {MIDI_Notes::A(2), Channel_5};
      noteAddr2R = {MIDI_Notes::Eb(2), Channel_5};
      noteAddr4R= {MIDI_Notes::Gb(2), Channel_5};
      noteAddr6R = {MIDI_Notes::Ab(2), Channel_5};
      //Serial.println("R: Zone 1");
    }
  }
}

// Piano: Function to produce key fade in piano mode
void cPianoFadeVolume(int ith_Val, char whichKeypad){
  // Pitch Bend
  int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
  pitchBendValue = constrain(pitchBendValue, 0, 16383); 
  midi.sendPitchBend(Channel_5, pitchBendValue);

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
        chooseNoteOn(ith_Val, 'L');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructL.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructL.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, 'L');
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
        chooseNoteOn(ith_Val, 'R');
      }
    }
    else{
      // decrease volume slowly
      for(int i=fsrStructR.initVolume; i>=0; i-=5)
      {
        midi.sendControlChange(ccAddr, fsrStructR.initVolume); //changes CC to volume control option
                                                        // would be sending the prev. volume
        chooseNoteOn(ith_Val, 'R');
      }
    }
  }
  //update noteOnHis struct here
  noteOnSig.ith_Val[ith_Val] = 1;
  noteOnSig.whichKeypad[ith_Val] = whichKeypad;
}

// Chinese Piano
void chinesePianoMode(int i){
  if(fsrStructL.prevVolume <= 24 && fsrStructL.currVolume <= 24 && fsrStructL.turnOffFirstTime == 1)
  {
    // make sure to only turn the previous note off
    if(noteOnSig.ith_Val[i] == 1 && noteOnSig.whichKeypad[i] == 'L'){
      chooseNoteOff(i,'L');

      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';   
    }
    fsrStructL.turnOffFirstTime = 0;
    fsrStructL.firstTimeNoteOn = 1;
  }
  else if(fsrStructL.currVolume >= 25 && fsrStructL.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    cPianoFadeVolume(i, 'L');
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
      chooseNoteOff(i, 'R');
      noteOnSig.ith_Val[i] = 0;
      noteOnSig.whichKeypad[i] = 'N';  
    }
    fsrStructR.turnOffFirstTime = 0;
    fsrStructR.firstTimeNoteOn = 1;
  }
  else if(fsrStructR.currVolume >= 25 && fsrStructR.firstTimeNoteOn == 1){   //NOTE SHOULD ONLY GO OFF ONCE WHICH IS AT INITIAL TIME OF PRESS, after that it will never go here
    cPianoFadeVolume(i, 'R');
    fsrStructR.turnOffFirstTime = 1;
    fsrStructR.firstTimeNoteOn = 0;
  }
  else{
    //Serial.println("R: Do Nothing");
  }
}

// Final Demo Function///////////////////////////////////////////////////////////////////////////////////////////
int switchInstruments(int8_t tiltSensor, int16_t rE_L){
  char tilt = 'A';
  int8_t rotPlate_L = (dataSigReceivedL.rotPlate1<<2) | (dataSigReceivedL.rotPlate2<<1) | dataSigReceivedL.rotPlate3;  //left rotation Plate

  //update history of rotPlate
  if(rotPlate_L != 0b111){
    rotPlateHist[0] = rotPlateHist[1];
    rotPlateHist[1] = rotPlate_L;
  }

  //Serial.println("s tiltSensor: "+String(tiltSensor));
  Serial.println("Left Rotational Plate: "+String(rotPlate_L));

  //determine orientation of tilt sensor
  if(tiltSensor == 0b011 || tiltSensor == 0b110 || tiltSensor == 0b001){
    tilt = 'V';
  }
  else if(tiltSensor == 0b101){
    tilt = 'A';
  }
  else{
    tilt = 'H';
  }
  //Serial.println("Tilt Sensor: "+String(tiltSensor));
  Serial.println("Rotational Plate: "+String(rotPlateHist[1]));

  if(tilt == 'H'){
    if(rotPlateHist[1] == 0b101){
      //channel 1 to piano
      return 1;
    }
    else if(rotPlateHist[1] == 0b011){
      //channel 4 to flute
      return 4;
    }
    else if(rotPlateHist[1] == 0b110){
      //channel 5 to Chinese Piano
      return 5;
    }
  }
  else if(tilt == 'A'){
    if(dataSigReceivedR.rotEncoder < borderValue_L){
      //channel 2 to sax
      return 2;
    }
    else if(dataSigReceivedR.rotEncoder >= borderValue_L){
      //channel 3 to clarinet
      return 3;
    }
  }
  else if(tilt == 'V'){
    //channel 6 to bass guitar
    return 6;
  }
  return 1;
}

void setup(void){
  Serial.begin(9600);
  pinMode(sensorHPin, INPUT);
  pinMode(sensorVPin, INPUT);
  pinMode(sensorAPin, INPUT);

  radio.begin(); // Start the NRF24L01
  radio.openReadingPipe(1, 0xABCDEF12); //left keypad Tx
  radio.openReadingPipe(2, 0xABCDEF23); //right keypad Tx
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
  midi.begin(); //Initializes MIDI interface
  Serial.println("-------------------------Program Start------------------------");
}

void loop(void){
  //read in values for tiltSensor
  int sensor_HValue = digitalRead(sensorHPin);
  int sensor_VValue = digitalRead(sensorVPin);
  int sensor_AValue = digitalRead(sensorAPin);
  int8_t tiltSensor = (sensor_HValue << 2) | (sensor_VValue << 1) | sensor_AValue; 
  tiltSensorNoise(tiltSensor);
  Serial.println("Tilt Sensor: "+String(tiltSensor));

  if(radio.available()){
    radio.read(&tempSig, sizeof(tempSig));
    //if left or right is 0, then copy all values from tempSig to dataSigReceivedL, else dataSigReceivedR
    if(tempSig.lOrR == 0){
      memcpy(&dataSigReceivedL, &tempSig, sizeof(tempSig));
    }
    else{
      memcpy(&dataSigReceivedR, &tempSig, sizeof(tempSig));
    }
  
    for(int8_t i=startingMuxSel; i<=(startingMuxSel + 0b0110); i++){
      pickStruct(i); //copy dataSigReceived to temp struct

      int16_t rE_L = dataSigReceivedL.rotEncoder;

      //only needs to be called once since both keypads should be on the same channel
      int which_channel = switchInstruments(tiltSensor, rE_L);

      if(which_channel == 1){
        Serial.println("Piano Mode On");
        fsrStructL.prevVolume = fsrStructL.currVolume;
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructL.currVolume = mapFSRtoVolume(i, 'L');
        fsrStructR.currVolume = mapFSRtoVolume(i, 'R');

        // initalize volume: fsrStuct(L/R).initVolume;
        initializeVol(which_channel);

        // set notes to keys here and return a zone to prevent B sharp from turning on
        setPianoNotes(cmpTilt, 'L');
        setPianoNotes(cmpTilt, 'R');

        // Pitch Bend
        int pitchBendValue = map(dataSigReceivedL.joystick, 0, 1023, 0, 16383);
        pitchBendValue = constrain(pitchBendValue, 0, 16383); 
        midi.sendPitchBend(Channel_1, pitchBendValue);

        // if note is released or just not pressed, then proceed, else turn note on
        pianoMode(i);
      }
      else if(which_channel == 2){
        Serial.println("Saxophone Mode On");

        //maintain volume history
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructR.currVolume = dataSigReceivedR.joystick;
        initializeVol(which_channel);

        //call function for sax...
        saxMode();
      }
      else if(which_channel == 3){
        Serial.println("Clarinet Mode On");

        //maintain volume history
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructR.currVolume = dataSigReceivedR.joystick;
        initializeVol(which_channel);

        //call function for clarinet...
        clarinetMode();
      }
      else if(which_channel == 4){
        Serial.println("Flute Mode On");

        //maintain volume history
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructR.currVolume = dataSigReceivedR.joystick;
        initializeVol(which_channel);

        //call function for flute...
        fluteMode();
      }
      else if(which_channel == 5){
        Serial.println("Chinese Piano Mode On");

        //call function for chinese piano...
        fsrStructL.prevVolume = fsrStructL.currVolume;
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructL.currVolume = mapFSRtoVolume(i, 'L');
        fsrStructR.currVolume = mapFSRtoVolume(i, 'R');

        // initalize volume: fsrStuct(L/R).initVolume;
        initializeVol(which_channel);

        // set notes to keys here and return a zone to prevent B sharp from turning on
        setCPianoNotes(cmpTilt, 'L');
        setCPianoNotes(cmpTilt, 'R');
        // if note is released or just not pressed, then proceed, else turn note on
        chinesePianoMode(i);
      }
      else if(which_channel == 6){
        Serial.println("Bass Guitar Mode On");

        //maintain volume history
        fsrStructR.prevVolume = fsrStructR.currVolume;
        fsrStructR.currVolume = mapFSRtoVolume(i, 'R');

        initializeVol(which_channel);

        //call function for bass guitar...
        setBassNotes();
        bassGuitarMode(i);
      }

      //call function to copy all values from fsrReading(L/R) to struct
      copytoStruct(i);
    }

    Serial.println("");
    Serial.println("dataSigReceivedL: "+String(dataSigReceivedL.fsr0)+", "+String(dataSigReceivedL.fsr1)+", "+String(dataSigReceivedL.fsr2)+", "+String(dataSigReceivedL.fsr3)+", "+String(dataSigReceivedL.fsr4)+", "+String(dataSigReceivedL.fsr5)+", "+String(dataSigReceivedL.fsr6));
    Serial.println("dataSigReceivedR: "+String(dataSigReceivedR.fsr0)+", "+String(dataSigReceivedR.fsr1)+", "+String(dataSigReceivedR.fsr2)+", "+String(dataSigReceivedR.fsr3)+", "+String(dataSigReceivedR.fsr4)+", "+String(dataSigReceivedR.fsr5)+", "+String(dataSigReceivedR.fsr6));
    Serial.println("Joystick(Left/Right): "+String(dataSigReceivedL.joystick)+","+String(dataSigReceivedR.joystick));
    Serial.println("Rotary Encoder(Left/Right): "+String(dataSigReceivedL.rotEncoder)+","+String(dataSigReceivedR.rotEncoder));
  }
  else
  {
    Serial.println("No msg received "); 
  }
  midi.update(); // Handle or discard MIDI input
  delay(23);
}