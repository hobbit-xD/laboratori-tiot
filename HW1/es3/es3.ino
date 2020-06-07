volatile unsigned int count = 0;
// valore booleano per descrivere lo stato del sensore, fintanto che è true il sensore sta
// rilevando presenze secondo le specifiche, controllando il valore si può accendere/spegnere
// il led di stato
volatile bool active = false;
const int LED_PIN = 12;
const int PIR_PIN = 7;

void setup() {
  // put your setup code here, to run once:
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), checkPresence, CHANGE);
  while(!Serial);
  Serial.println("Connessione seriale avviata!");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(30000);
  Serial.print("Total count: ");
  Serial.println(count);
}


// Il parametro di delay viene impostato al minimo per poter effettuare rilevazioni e test utili
// in breve tempo
void checkPresence(){
  if(!active){
    count++;
    active = !active;
    //Serial.println("Individuata persona");
    digitalWrite(LED_PIN, HIGH);
  }else{
    active = !active;
    //Serial.println("Termine delay, 3 sec cooldown");
    digitalWrite(LED_PIN, LOW);
  }
  
  
}
