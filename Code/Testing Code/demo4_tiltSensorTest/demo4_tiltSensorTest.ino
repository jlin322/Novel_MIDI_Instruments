/*
  Programmer: Jolin Lin
  ECD 423 - Novel MIDI Instrument (Senior PRJ)
  Date: 02/29/2024
  Purpose: The purpose of this program is to test out different 
            angles for the tilt sensors such that the vertical, horizontal,
            and angled orientation has different and consistent combinations.
*/

const int sensorVPin = 7; // Vertical
const int sensorHPin = 9; // Horizontal
const int sensorAPin = 8; // Angled

void setup() {
  Serial.begin(9600);
  pinMode(sensorHPin, INPUT);
  pinMode(sensorVPin, INPUT);
  pinMode(sensorAPin, INPUT);
}

void loop() {
  // Read the state of each sensor
  int sensor_HValue = digitalRead(sensorHPin);
  int sensor_VValue = digitalRead(sensorVPin);
  int sensor_AValue = digitalRead(sensorAPin);

  // Print the values of each sensor
  Serial.print("Horizontal, Vertical, Angled: ");
  Serial.print(sensor_HValue);
  Serial.print(sensor_VValue);
  Serial.print(sensor_AValue);
  Serial.println(" ");

  delay(100); // Adjust delay as needed
}
