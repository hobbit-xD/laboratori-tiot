#include <math.h>
#include <Process.h>
#include <Bridge.h>
#include <ArduinoJson.h>

#define TEMP_PIN A1
/*====== Costanti sensore temperatura ======*/

const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;
float R = 0.0;
float T = 0.0; //Kelvin
/*==========================================*/
float readTemp() {

  int sensorValue = analogRead(TEMP_PIN); // = Vsig
  R = ((Vcc / sensorValue) - 1.0) * R0;
  T = (1 / ((log(R / R0) / B) + (1 / 298.15))) - 273.15; // Temperatura in gradi celsius

  return T;
}

const String target = "127.0.0.1:8080/devices";
const int jsonCap = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1) + 40;
DynamicJsonDocument jsonOut(jsonCap);


void setup() {
  //led utilizzato per verificare l'avvio della connessione bridge
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEMP_PIN, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Bridge.begin();
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
}

void loop() {
  String jsonValue = senMlRegister();
  unsigned int retval = postRequest(jsonValue);
  if(retval != 0){
    Serial.print("Something went wrong, curl return value was ");
    Serial.println(retval);
  }
  //Serial.print("curl return value was ");
  //Serial.println(retval);
  delay(5000);
}

int postRequest(String data){
  Process p;

  p.begin("curl");
  p.addParameter("-H");
  p.addParameter("Content-Type: application/json");
  p.addParameter("-X");
  p.addParameter("POST");
  p.addParameter("-d");
  p.addParameter(data);
  p.addParameter(target);
  p.run();

  return p.exitValue();
}


String senMlJson(float value){
  jsonOut.clear();
  jsonOut["bn"] = "Yun";
  jsonOut["e"][0]["n"] = F("temperature");
  jsonOut["e"][0]["t"] = millis();
  jsonOut["e"][0]["v"] = value;
  jsonOut["e"][0]["u"] = F("Cel");
  
  String out;
  serializeJson(jsonOut, out);
  return out;
}
String senMlRegister(){
  jsonOut.clear();
  
  jsonOut["end-points"]["REST"] = "http";
  jsonOut["resources"] = "temperature";
 
  
  String out;
  serializeJson(jsonOut, out);
  return out;
}
