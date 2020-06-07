#include <math.h>

const int TEMP_PIN = A1;

const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;

float R = 0.0;
float T = 0.0; //Kelvin

void setup() {
  Serial.begin(9600);
  while(!Serial);
  pinMode(TEMP_PIN,INPUT);
  Serial.println("Starting Lab 1.5!");

}

void loop() {

  int sensorValue = analogRead(TEMP_PIN); // = Vsig

  R = ((Vcc/sensorValue) - 1.0)*R0;

  T = (1/((log(R/R0)/B)+(1/298.15))) - 273.15; // Temperatura in gradi celsius

  Serial.print("Temperature: ");
  Serial.println(T);

  delay(5000);

}
