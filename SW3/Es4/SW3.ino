#include <math.h>
#include <TimerOne.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include <MQTTclient.h>
#include <Process.h>
#include <ArduinoJson.h>

#define MQTT_HOST F("test.mosquitto.org")
#define CATALOG_ADDRESS F("192.168.1.12:8080/devices")
String my_base_topic = "/tiot/22/SW3";

#define TEMP_PIN A1
#define LED_PIN 5 //Pin PWN della scheda
#define FAN_PIN 6
#define NOISE_PIN 7
#define PIR_PIN 8

#define DEBUG 1

LiquidCrystal_PCF8574 lcd(0x27);

float temp;
unsigned int fanSpeed = 0;
unsigned int ledIntensity = 0;
bool presence = false;

/*====== Costanti sensore temperatura ======*/

const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;
/*==========================================*/

/* ============== Sensore PIR ==============*/
bool pir_presence = false;
unsigned int pir_timeout = 300; //300s --> 5 minuti
unsigned int pir_time = 0;
/*==========================================*/

/* ============ Sensore di rumore ==========*/
volatile bool noise_presence = false;
volatile unsigned int noise_timeout = 1200; //1200s --> 20 minuti
volatile unsigned int noise_time = 0;
volatile unsigned int noise_interval = 600; //600s --> 10 minuti
volatile unsigned int noise_count = 0;
volatile unsigned int noise_events = 100;

volatile unsigned int timer_counter = 0;
/*==========================================*/

//Funzione per leggere i valori di temperatura dal sensore
float readTemp() {
  int sensorValue = analogRead(TEMP_PIN); // = Vsig
  float R = ((Vcc / sensorValue) - 1.0) * R0;
  temp = (1 / ((log(R / R0) / B) + (1 / 298.15))) - 273.15; // Temperatura in gradi celsius

  String msg = senMlEncode(F("temperature"), temp, F("Cel"));
  mqtt.publish(my_base_topic + F("/data"), msg);
}

// Funzione che mi ritorna true se almeno uno dei due sensori rileva movimento -> persone nella stanza
bool checkPresence() {
  presence = (checkPIR() || noise_presence);

  String msg = senMlEncode(F("presence"), presence? 1 : 0, F(""));
  mqtt.publish(my_base_topic + F("/data"), msg);
}

//Funzione per verificare se c'è qualcuno nella stanza attraverso il sensore PIR
bool checkPIR() {
  if (digitalRead(PIR_PIN)) {
    pir_presence = true;
    pir_time = millis() / 1000;
  } else {
    if (millis() / 1000 - pir_time > pir_timeout) {
      pir_presence = false;
    }
  }
  return pir_presence;
}

//Funzione per verificare se c'è qualcuno nella stanza attraverso il sensore di rumore
void checkNOISE() {
  unsigned int t = millis() / 1000;

  if (t - noise_time < noise_interval) {
    noise_count++;
  } else {
    noise_count = 0;
    noise_time = t;
  }

  if (noise_count > noise_events) {
    noise_count = 0;
    noise_time = t;
    noise_presence = true;
    timer_counter = 0;
    Timer1.restart();
  }
}

//timer1 può essere inizializzato a max 8 secondi, così si risolve
void noiseTimeout() {
  timer_counter++;

  if (timer_counter > noise_timeout / 8) {
    noise_presence = false;
    timer_counter = 0;
    Timer1.restart();
    Timer1.stop();
  }
}

const int capacity = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1) + 40;
DynamicJsonDocument jsonData(capacity);

String senMlEncode (String title, float value, String unit) {
  jsonData.clear();
  jsonData[F("bn")] = "Yun";

  jsonData[F("e")][0][F("n")] = title;
  jsonData[F("e")][0][F("t")] = millis();
  jsonData[F("e")][0][F("v")] = value;

  if (unit != "") {
    jsonData[F("e")][0][F("u")] = unit;
  } else {
    jsonData[F("e")][0][F("u")] = (char*)NULL;
  }

  String output;
  serializeJson(jsonData, output);

  return output;
}

void setup() {
  int error;
  Serial.begin(9600);
  while (!Serial);

  //Inizializzo il motore e pongo la velocità a 0
  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, fanSpeed);

  //Inizializzo il LED e pongo l'intensità a 0 
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, ledIntensity);

  //Inizializzo il sensore di temperatura
  pinMode(TEMP_PIN, INPUT);

  //Inizializzo sensore PIR
  pinMode(PIR_PIN, INPUT);

  //Inizializzo sensore di rumore
  pinMode(NOISE_PIN, INPUT);


  //Interrupt per gestire sensore di rumore
  attachInterrupt(digitalPinToInterrupt(NOISE_PIN), checkNOISE, FALLING);
  Timer1.initialize(8e6);
  Timer1.attachInterrupt(noiseTimeout);

  //Controllo che connessione con LCD sia andata a buon fine
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(F("LCD found"));
    lcd.begin(16, 2); // initialize the lcd
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
  } else {
    Serial.println(F("LCD not found."));
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Bridge.begin();
  digitalWrite(LED_BUILTIN, LOW);

  mqtt.begin(MQTT_HOST, 1883);
  mqtt.subscribe(my_base_topic + F("/control"), setActuator);
  mqtt.subscribe(my_base_topic + F("/messages"), handleMsg);
}

void loop() {
    mqtt.monitor();

    registerDevice();
    readTemp();
    checkPresence();
    delay(1000);
}

void setActuator(const String& topic, const String& subtopic, const String& message){
  DynamicJsonDocument jsonMsg(capacity);
  deserializeJson(jsonMsg, message);
  Serial.println(message);
  
  if (jsonMsg["e"][0]["n"] == F("fan")) {
    fanSpeed = int(jsonMsg["e"][0]["v"]);
    analogWrite(FAN_PIN, fanSpeed);
  }else if (jsonMsg["e"][0]["n"] == F("heater")){
    ledIntensity = int(jsonMsg["e"][0]["v"]);
    analogWrite(LED_PIN, ledIntensity);
  }
}


void handleMsg(const String& topic, const String& subtopic, const String& message){
  lcd.clear();
  lcd.print(F("Messaggio:"));
  unsigned int row = 1;
  lcd.setCursor(0, row);
  unsigned int n = message.length();
  for(unsigned int i = 0; i<n; i++){
    lcd.print(message[i]);
    if( (i+1) % 16 ==0){
      row = (row+1)%2;
      if(row == 0){
        delay(1500);
        lcd.clear();
      }else
        lcd.setCursor(0, row);
    }
  }
}

void registerDevice(){
  Process p;
  String data = F("{\"bn\":\"Yun\", \"resources\": [\"temperature\", \"presence\"], \"end-points\": {\"rest\": [], \"mqtt-topics\": [\"/tiot/22/SW3/data\", \"/tiot/22/SW3/control\", \"/tiot/22/SW3/messages\"]}}");
  //Serial.println(data);
  p.begin(F("curl"));
  p.addParameter(F("-H"));
  p.addParameter(F("Content-Type: application/json"));
  p.addParameter(F("-X"));
  p.addParameter(F("POST"));
  p.addParameter(F("-d"));
  p.addParameter(data);
  p.addParameter(CATALOG_ADDRESS);
  p.run();

  return p.exitValue();
}
