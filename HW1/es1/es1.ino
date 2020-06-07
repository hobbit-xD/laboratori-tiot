#include <TimerOne.h>

const int led1 = 11;
const int led2 = 12;

int statoLed1 = 0;
int statoLed2 = 0;

void setup() {

  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);

  Timer1.initialize(5000000); // 5 secondi
  Timer1.attachInterrupt(blinkLed);

}

void blinkLed()
{
  
  if(statoLed1 == 0)
    statoLed1 = 1;
  else
    statoLed1 = 0;

  digitalWrite(led1,statoLed1);
}

void loop() {

  statoLed2 = !statoLed2;
  digitalWrite(led2,statoLed2);
  
  delay(3000); // 3 secondi
}
