/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 03/13/2024
  Purpose: The purpose of this program is to read multiple FSR values from a
            mux breakout board and send this data to the base station then to
              the synthesizer.
  Notes: Keep track of individual FSR mapping values
    
*/

#include <SPI.h>
#include <RF24.h>
#include <light_CD74HC4067.h>

RF24 radio(9,10); // pin assignments for nRF CE,CSN
const int MUX_ENABLE = 8;
const int S0 = 7;
const int S1 = 6;
const int S2 = 5;
const int S3 = 4;
const int SIG = A5; //signal from mux board coming in

CD74HC4067 mux(S0,S1,S2,S3); //creates a new object

// struct to send all 7 FSR data to the base station
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
struct DataSig muxData = {0,0,0,0,0,0,0};

//assign fsrDataSig to appropriate int within struct muxData
void setStructData(int fsrDataSig, int i){
  if(i == 0b0000){
    muxData.fsr0 = fsrDataSig;
    //Serial.print("Updating struct.fsr0: ");
    //Serial.println(muxData.fsr0);
  }
  else if(i == 0b0001){
    muxData.fsr1 = fsrDataSig;
    //Serial.print("Updating struct.fsr1: ");
    //Serial.println(muxData.fsr1);
    //Serial.println("Entering 1");
  }
  else if(i == 0b0010){
    muxData.fsr2 = fsrDataSig;
    //Serial.println("Entering 2");
  }
  else if(i == 0b0011){
    muxData.fsr3 = fsrDataSig;
    //Serial.println("Entering 3");
  }
  else if(i == 0b0100){
    muxData.fsr4 = fsrDataSig;
    //Serial.println("Entering 4");
  }
  else if(i == 0b0101){
    muxData.fsr5 = fsrDataSig;
    //Serial.println("Entering 5");
  }
  else if(i == 0b0110){
    muxData.fsr6 = fsrDataSig;
    //Serial.println("Entering 6");
  }
  else{
    //do nothing
    Serial.println("Not updating anything");
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(SIG, INPUT);

  Serial.println("Initializing radio...");
  radio.begin(); // Starts the NRF24L01
  Serial.println("Radio initialized.");

  Serial.println("Setting up writing pipe...");
  radio.openWritingPipe(0xABCDEF12); 
  Serial.println("Writing pipe set up.");

  Serial.println("Setting up PA levels...");
  radio.setPALevel(RF24_PA_MIN);  // RF24_PA_MAX is default.
  Serial.println("PA levels set up.");
  Serial.println("-------------------------Program Start------------------------");
}

void loop() {
  digitalWrite(MUX_ENABLE, HIGH);
  // have code run in an incrementing loop through every S3,S2,S1,S0
  for(byte i=0; i<=6;i++){
    mux.channel(i); //opens channel i on mux
    int val = analogRead(SIG); 
    //Serial.println("Mux Channel "+String(i)+": "+String(val));  // Print value

    //assign val to appropriate fsr# within struct muxData
    setStructData(val, i);    
  }
  digitalWrite(MUX_ENABLE, LOW);

  //--------------------------------------------Test Data Here----------------------------------------//
  //muxData = {10,590,0,0,150,0,100};
  /*Serial.println("Sending struct muxData: ");
  Serial.print("muxData.fsr0: ");
  Serial.println(muxData.fsr0);
  Serial.print("muxData.fsr1: ");
  Serial.println(muxData.fsr1);
  Serial.print("muxData.fsr2: ");
  Serial.println(muxData.fsr2);
  Serial.print("muxData.fsr3: ");
  Serial.println(muxData.fsr3);
  Serial.print("muxData.fsr4: ");
  Serial.println(muxData.fsr4);
  Serial.print("muxData.fsr5: ");
  Serial.println(muxData.fsr5);
  Serial.print("muxData.fsr6: ");
  Serial.println(muxData.fsr6);
  Serial.println("");*/
  Serial.println("Mux Values: " + String(muxData.fsr0) + ", " + String(muxData.fsr1) + ", " + String(muxData.fsr2) + ", " + String(muxData.fsr3) + ", " + String(muxData.fsr4) + ", " + String(muxData.fsr5) + ", " + String(muxData.fsr6));


  radio.write(&muxData, sizeof(muxData));
  //transmitData = 0;

  delay(500);  //send msg every sec
  
}

/*muxData.fsr0 = 100;
  muxData.fsr1 = 300;
  Serial.println("Sending struct muxData: ");
  Serial.print("muxData.fsr0: ");
  Serial.println(muxData.fsr0);
  Serial.println("");

  //int has 32 bits;assign left most 16 digits to fsr0 and the rest to fsr1
  //uint32_t transmitData = (muxData.fsr1 << 16) | (muxData.fsr0);
  uint32_t transmitData = (muxData.fsr1*1000)+muxData.fsr0;
  Serial.println("TransmitData: "+String(transmitData));
  Serial.println("");

  radio.write(&transmitData, sizeof(transmitData));
  transmitData = 0;

  //delay(100);  //send msg every sec
*/
