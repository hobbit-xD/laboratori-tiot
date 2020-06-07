#include <TimerOne.h>

const int RLED_PIN = 11;
const int GLED_PIN = 12;

const float R_HALF_PERIOD = 1.5;
const float G_HALF_PERIOD = 3.5;

volatile unsigned int greenLedState = LOW;
unsigned int redLedState = LOW;

void setup() {
  // put your setup code here, to run once:
  pinMode(RLED_PIN, OUTPUT);
  pinMode(GLED_PIN, OUTPUT);
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Connessione seriale avviata!");
  Timer1.initialize(G_HALF_PERIOD * 1e06);
  Timer1.attachInterrupt(blinkGreen);
}

void loop() {
  // put your main code here, to run repeatedly:
  serialPrintStatus();
  redLedState = !redLedState;
  digitalWrite(RLED_PIN, redLedState);
  delay(R_HALF_PERIOD * 1e03);
}

void serialPrintStatus(){
  if(Serial.available() > 0){
    unsigned int input = Serial.read();

    if(input == 'R'){
      Serial.print("Stato led RED: ");
      Serial.println(!redLedState);
    }else if(input == 'G'){
      Serial.print("Stato led GREEN: ");
      Serial.println(greenLedState);
    }else{
      Serial.println("Comando invalido");
    }

    // eventuali caratteri oltre al primo vengono ignorati
    while(Serial.available() > 0)
        Serial.read();
  }
}

void blinkGreen(){
  greenLedState = !greenLedState;
  digitalWrite(GLED_PIN, greenLedState); 
}
