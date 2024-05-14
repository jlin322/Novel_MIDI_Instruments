/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 04/24/2024
  Purpose: This program is a continuation of demo11_rotaryEncoder by receiving values
            from the rotary encoder, tilt sensor and rotational sensor to switch
            between instruments(channels). Tilt sensors are directly connected to the 
            based station using a wire.
*/

#include <SPI.h>
#include <RF24.h> 
#include <light_CD74HC4067.h> //library for mux
#include <Encoder.h>  //library for Teensy 4.0

RF24 radio(9,10); //nRF: CE,CSN

// Analog and digital pins below
const int MUX_ENABLE = 8;
const int S0 = 4;
const int S1 = 3; 
const int S2 = 2; 
const int S3 = 1;
const int8_t ROTENC1 = 6; 
const int8_t ROTENC2 = 7;
const int ROTPLATE1 = 17;
const int ROTPLATE2 = 18;
const int ROTPLATE3 = 19;
const int SIG = A7; 
const int JOYSTICK = A8; 

CD74HC4067 mux(S0,S1,S2,S3); //creates a new object
Encoder myEnc(7,6); 

// struct to send all 7 FSR data to the base station: total of 21 bytes
struct DataSig{
  bool lOrR;
  uint16_t fsr0;      
  uint16_t fsr1;
  uint16_t fsr2;
  uint16_t fsr3;
  uint16_t fsr4;
  uint16_t fsr5;
  uint16_t fsr6;
  int16_t rotEncoder; 
  bool rotPlate1;  
  bool rotPlate2;
  bool rotPlate3;
  int16_t joystick;  
};
struct DataSig dataSigSend = {0,0,0,0,0,0,0,0,0,0,0};

//Function to assign fsrDataSig to appropriate int within struct dataSigSend
void setStructData(int fsrDataSig, int i){
  if(i == 0b0000){
    dataSigSend.fsr0 = fsrDataSig;
  }
  else if(i == 0b0001){
    dataSigSend.fsr1 = fsrDataSig;
  }
  else if(i == 0b0010){
    dataSigSend.fsr2 = fsrDataSig;
  }
  else if(i == 0b0011){
    dataSigSend.fsr3 = fsrDataSig;
  }
  else if(i == 0b0100){
    dataSigSend.fsr4 = fsrDataSig;
  }
  else if(i == 0b0101){
    dataSigSend.fsr5 = fsrDataSig;
  }
  else if(i == 0b0110){
    dataSigSend.fsr6 = fsrDataSig;
  }
  else{
    Serial.println("Not updating anything");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ROTPLATE1, INPUT_PULLUP);
  pinMode(ROTPLATE2, INPUT_PULLUP);
  pinMode(ROTPLATE3, INPUT_PULLUP);
  
  radio.begin(); // Starts the NRF24L01
  radio.openWritingPipe(0xABCDEF23); 
  radio.setPALevel(RF24_PA_MIN);
  Serial.println("-------------------------Program Start------------------------");
}

int16_t iterate = 0;
int16_t z=0;

void loop() {
  int16_t newPosition = myEnc.read();
  if(newPosition != dataSigSend.rotEncoder){
    dataSigSend.rotEncoder = newPosition/4;
  }

  //read joystick value
  dataSigSend.joystick = analogRead(JOYSTICK);

  //reading rotational plate sensors
  dataSigSend.rotPlate1 = digitalRead(ROTPLATE1);
  dataSigSend.rotPlate2 = digitalRead(ROTPLATE2);
  dataSigSend.rotPlate3 = digitalRead(ROTPLATE3);
  
  digitalWrite(MUX_ENABLE, HIGH);
  // for each iteration of the main loop, i increases from 0-6 back to 0
  mux.channel(iterate); //opens channel i on mux
  int val = analogRead(SIG); 

  //assign val to appropriate fsr# within struct dataSigSend
  setStructData(val, iterate); 
  digitalWrite(MUX_ENABLE, LOW);

  // set LeftOrRight to 1
  dataSigSend.lOrR = 1;

  //increase value of i 
  iterate += 1;
  if(iterate >= 7){
    iterate = 0;
    radio.write(&dataSigSend, sizeof(dataSigSend));
    Serial.println("Mux Values: " + String(dataSigSend.fsr0) + ", " + String(dataSigSend.fsr1) + ", " + String(dataSigSend.fsr2) + ", " + String(dataSigSend.fsr3) + ", " + String(dataSigSend.fsr4) + ", " + String(dataSigSend.fsr5) + ", " + String(dataSigSend.fsr6));
    Serial.println("Rotational Sensors: "+String(dataSigSend.rotPlate1)+", " + String(dataSigSend.rotPlate2) + ", " + String(dataSigSend.rotPlate3));
    Serial.println("RotaryEncoder Val: "+String(dataSigSend.rotEncoder));
    Serial.println("Joystick Val: "+String(dataSigSend.joystick));
    Serial.println("");
  }
}