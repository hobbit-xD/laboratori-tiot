#include <math.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>
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

BridgeServer srv;

const int jsonCap = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1) + 40;
DynamicJsonDocument jsonOut(jsonCap);


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEMP_PIN, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Bridge.begin();
  digitalWrite(LED_BUILTIN, LOW);
  srv.listenOnLocalhost();
  srv.begin();
  //Serial.begin(9600);
  //while(!Serial);
  //Serial.println("Listening on srv");
}

void loop() {
  BridgeClient clt = srv.accept();

  if(clt){
    processRequest(clt);
    clt.stop();
  }
  delay(50);
}

void processRequest(BridgeClient clt){
  String cmd = clt.readStringUntil('/');
  cmd.trim();

  if(cmd == F("led")){
    handleLed(clt);
  }else if(cmd == F("temperature")){
    handleTemp(clt);
  }else{
    handleError(clt, 404);
  }
}

void handleError(BridgeClient clt, unsigned int err_code){
  if(err_code == 400){
    printResponse(clt, err_code, F("400 Bad Request")); 
  }else if(err_code == 404){
    printResponse(clt, err_code, F("404 Not Found")); 
  }
  
}

void handleLed(BridgeClient clt){
  unsigned int val = clt.parseInt();
  if(val == 0 || val == 1){
    digitalWrite(LED_BUILTIN, val);
    printResponse(clt, 200, senMlJson( F("led"), val, F("")));
  }else{
    handleError(clt, 400);
  }
}
void handleTemp(BridgeClient clt){
  if(clt.peek()!= -1){
    handleError(clt, 404);
  }else{
    printResponse(clt, 200, senMlJson( F("temperature"), readTemp(), F("Cel")));
  }
}

void printResponse(BridgeClient clt, unsigned int code, String body){
  clt.println("Status: " + String(code));
  if(code == 200){
    clt.println(F("Content-type: application/json; charset=utf-8"));
  }else{
    clt.println(F("Content-type: text/plain; charset=utf-8"));
  }
  clt.println();
  clt.println(body);
}

String senMlJson(String obj, float value, String unit){
  jsonOut.clear();
  jsonOut["bn"] = "Yun";
  jsonOut["e"][0]["n"] = obj;
  jsonOut["e"][0]["t"] = millis();
  jsonOut["e"][0]["v"] = value;
  if(unit != ""){
    jsonOut["e"][0]["u"] = unit;
  }else{
    jsonOut["e"][0]["u"] = (char*)NULL;
  }

  String out;
  serializeJson(jsonOut, out);
  return out;
}
