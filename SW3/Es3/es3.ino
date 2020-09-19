#include <ArduinoJson.h>
#include <MQTTclient.h>
#include <Process.h>

#define MQTT_HOST F("test.mosquitto.org")

#define LED_PIN 10
#define TEMP_PIN A1

const String url = "http://0.0.0.0:8080/devices";
String my_base_topic = "/tiot/22";
float temp;

/*====== Costanti sensore temperatura ======*/
const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;
float R = 0.0;
float T = 0.0; //Kelvin
/*==========================================*/

const int capacity = JSON_OBJECT_SIZE(6) + JSON_ARRAY_SIZE(1) + 40;
DynamicJsonDocument doc_rec(capacity);
DynamicJsonDocument doc_snd(capacity);

//Funzione per leggere i valori di temperatura dal sensore
float readTemp() {

  int sensorValue = analogRead(TEMP_PIN); // = Vsig
  R = ((Vcc / sensorValue) - 1.0) * R0;
  T = (1 / ((log(R / R0) / B) + (1 / 298.15))) - 273.15; // Temperatura in gradi celsius

  return T;
}
void setLED(const String& topic, const String& subtopic, const String& message) {
  DeserializationError err = deserializeJson(doc_rec, message);
  if (err) {
    Serial.print(F("deserializeJson() failed with code: "));
    Serial.println(err.c_str());
  }

  if (doc_rec[F("e")][0][F("n")] == F("led")) {

    int value = int(doc_rec[F("e")][0][F("v")]);
    switch (value) {
      case 0:
        digitalWrite(LED_PIN, LOW);
        break;
      case 1:
        digitalWrite(LED_PIN, HIGH);
        break;
      default:
        Serial.println(F("Valore per attivare/disabilitare il led non corretto"));
        break;
    }
  }
}

String senMlEncode (String title, float value, String unit) {

  doc_snd.clear();
  doc_snd[F("bn")] = "Yun";

  doc_snd[F("e")][0][F("n")] = title;
  doc_snd[F("e")][0][F("t")] = millis();
  doc_snd[F("e")][0][F("v")] = value;

  if (unit != "") {
    doc_snd[F("e")][0][F("u")] = unit;
  } else {
    doc_snd[F("e")][0][F("u")] = (char*)NULL;
  }

  String output;
  serializeJson(doc_snd, output);

  return output;
}
int postRequest(String data) {
  Process p;
  p.begin("curl");
  p.addParameter("-H");
  p.addParameter("Content-Type: application/json");
  p.addParameter("-X");
  p.addParameter("POST");
  p.addParameter("-d");
  p.addParameter(data);
  p.addParameter(url);
  p.run();
  return p.exitValue(); // Status curl command
}

String senMlEncodeRegisterCatalog() {

  doc_rec.clear();
  doc_rec["end-points"]["mqtt-topics"]["temp"] = my_base_topic + "/temperature";
  doc_rec["end-points"]["mqtt-topics"]["led"] = my_base_topic + "/led";

  JsonArray res = doc_rec.createNestedArray("resources");
  res.add("temperature");
  res.add("led");

  String output;
  serializeJson(doc_rec, output);
  return output;
}


void setup() {

  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Bridge.begin();
  digitalWrite(LED_BUILTIN, LOW);

  mqtt.begin(MQTT_HOST, 1883);
  mqtt.subscribe(my_base_topic + F("/led"), setLED);

}

void loop() {

  mqtt.monitor();

  //Leggo la temperatura
  temp = readTemp();

  //Continuo ad inviare i valori di temperatura misurati
  String message = senMlEncode(F("temperaure"), temp, F("Cel"));
  mqtt.publish(my_base_topic + F("/temperature"), message);

  int retval = postRequest(senMlEncodeRegisterCatalog());
  if(retval != 0){
    Serial.print("Something went wrong, curl return value was ");
    Serial.println(retval);
  }
  
  delay(5000);

}
