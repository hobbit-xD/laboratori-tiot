unsigned int current_speed = 0;
const unsigned int FAN_PIN = 6;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Ready!");
  Serial.println("+ to increase speed");
  Serial.println("- to decrease speed");
  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, current_speed);
}

void loop() {
  changeSpeed();
}

void changeSpeed(){
  if(Serial.available() > 0){
    unsigned int input = Serial.read();

    if(input == '+'){
      if(current_speed == 255){
         Serial.println("Max speed reached");
      }else{
        current_speed += 51;
        analogWrite(FAN_PIN, current_speed); 
        Serial.print("Setting duty cycle to: ");
        Serial.print((float)current_speed/255*100);
        Serial.println("%");
      }
    }else if(input == '-'){
      if(current_speed == 0){
         Serial.println("Min speed reached");
      }else{
        current_speed -= 51;
        analogWrite(FAN_PIN, current_speed); 
        Serial.print("Setting duty cycle to: ");
        Serial.print((float)current_speed/255*100);
        Serial.println("%");
      }
    }else{
      Serial.println("Comando invalido");
    }

    // eventuali caratteri oltre al primo vengono ignorati
    while(Serial.available() > 0)
        Serial.read();
  }
}
