/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 04/11/2024
  Purpose: The purpose of this program is to read in rotary encoder values
            and FSR values from 1 keypad. Rotary Encoder values will be sent
            the base station to be used to determine which notes to play. 
  Note: Any delay put in the main loop will impact the efficiency of the rotary encoder.
        Rotary Encoder DT: Nano D3, Teensy 6 (yellow wire)
                      CLK: Nano D2, Teensy 7 (black wire)

        Nano to Tx: SCK - D13
                    MISO - D12
                    MOSI - D11
                    CE - D9
                    CSN - D10

*/

#include <SPI.h>
#include <RF24.h> 
#include <light_CD74HC4067.h> //library for mux
#include <Encoder.h>  //lib for Teensy

RF24 radio(9,10); // pin assignments for nRF CE,CSN

const int MUX_ENABLE = 8;
const int S0 = 4;
const int S1 = 3;
const int S2 = 2;
const int S3 = 1;
const int8_t ROTENC1 = 6;  //either CLK or DT from Rotary Encoder
const int8_t ROTENC2 = 7;
const int SIG = A6; //signal from mux board coming in
const int ROTPLATE1 = 17;
const int ROTPLATE2 = 18;
const int ROTPLATE3 = 19;
const int JOYSTICK = A8;

CD74HC4067 mux(S0,S1,S2,S3); //creates a new object
//BasicEncoder encoder(ROTENC1, ROTENC2);
Encoder myEnc(7,6); 

// struct to send all 7 FSR data to the base station
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
struct DataSig dataSigSend = {0,0,0,0,0,0,0,0,0,0,0,0};

//assign fsrDataSig to appropriate int within struct dataSigSend
void setStructData(int fsrDataSig, int i){
  if(i == 0b0000){
    dataSigSend.fsr0 = fsrDataSig;
    //Serial.print("Updating struct.fsr0: ");
    //Serial.println(dataSigSend.fsr0);
  }
  else if(i == 0b0001){
    dataSigSend.fsr1 = fsrDataSig;
    //Serial.print("Updating struct.fsr1: ");
    //Serial.println(dataSigSend.fsr1);
    //Serial.println("Entering 1");
  }
  else if(i == 0b0010){
    dataSigSend.fsr2 = fsrDataSig;
    //Serial.println("Entering 2");
  }
  else if(i == 0b0011){
    dataSigSend.fsr3 = fsrDataSig;
    //Serial.println("Entering 3");
  }
  else if(i == 0b0100){
    dataSigSend.fsr4 = fsrDataSig;
    //Serial.println("Entering 4");
  }
  else if(i == 0b0101){
    dataSigSend.fsr5 = fsrDataSig;
    //Serial.println("Entering 5");
  }
  else if(i == 0b0110){
    dataSigSend.fsr6 = fsrDataSig;
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
  pinMode(ROTPLATE1, INPUT_PULLUP);
  pinMode(ROTPLATE2, INPUT_PULLUP);
  pinMode(ROTPLATE3, INPUT_PULLUP);
  
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

int8_t iterate = 0;
int8_t z=0;
//int8_t oldPosition = -999;
//dataSigSend.rotEncoder = -999;

void loop() {
  int8_t newPosition = myEnc.read();
  if(newPosition != dataSigSend.rotEncoder){
    dataSigSend.rotEncoder = newPosition;
    //Serial.println("RotaryEncoder Val: "+String(dataSigSend.rotEncoder));
  }

  //reading rotational plate sensors
  dataSigSend.rotPlate1 = digitalRead(ROTPLATE1);
  dataSigSend.rotPlate2 = digitalRead(ROTPLATE2);
  dataSigSend.rotPlate3 = digitalRead(ROTPLATE3);

  //Read joystick values
  dataSigSend.joystick = analogRead(JOYSTICK);
  
  digitalWrite(MUX_ENABLE, HIGH);

  // for each iteration of the main loop, i increases from 0-6 back to 0
  mux.channel(iterate); //opens channel i on mux
  int val = analogRead(SIG); 
  //Serial.println("Mux Channel "+String(i)+": "+String(val));  // Print value

  //assign val to appropriate fsr# within struct dataSigSend
  setStructData(val, iterate); 
  
  digitalWrite(MUX_ENABLE, LOW);

  //increase value of i 
  iterate += 1;
  if(iterate >= 7){
    iterate = 0;
    radio.write(&dataSigSend, sizeof(dataSigSend));
    Serial.println("Mux Values: " + String(dataSigSend.fsr0) + ", " + String(dataSigSend.fsr1) + ", " + String(dataSigSend.fsr2) + ", " + String(dataSigSend.fsr3) + ", " + String(dataSigSend.fsr4) + ", " + String(dataSigSend.fsr5) + ", " + String(dataSigSend.fsr6));
    Serial.println("Rotational Sensors: "+String(dataSigSend.rotPlate1)+", " + String(dataSigSend.rotPlate2) + ", " + String(dataSigSend.rotPlate3));
    Serial.println("RotaryEncoder Val: "+String(dataSigSend.rotEncoder));
    Serial.println("Joystick: "+String(dataSigSend.joystick));
  }

  
  //read in value from rotary encoder and place it in 
  //Serial.println("Mux Values: " + String(dataSigSend.fsr0) + ", " + String(dataSigSend.fsr1) + ", " + String(dataSigSend.fsr2) + ", " + String(dataSigSend.fsr3) + ", " + String(dataSigSend.fsr4) + ", " + String(dataSigSend.fsr5) + ", " + String(dataSigSend.fsr6));
  //Serial.println("Rotary Encoder Value: "+String(dataSigSend.rotEncoder));
  //radio.write(&rotEncoder, sizeof(rotEncoder));

  //delay(500);  //send msg every sec
}

/* for the NANO
const int MUX_ENABLE = 8;
const int S0 = 7;
const int S1 = 6;
const int S2 = 5;
const int S3 = 4;
const int8_t ROTENC1 = 3;  //either CLK or DT from Rotary Encoder
const int8_t ROTENC2 = 2;
const int SIG = A5; //signal from mux board coming in
*/