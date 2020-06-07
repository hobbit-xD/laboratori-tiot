#include <LiquidCrystal_PCF8574.h>
#include <math.h>
#include <Wire.h>

LiquidCrystal_PCF8574 lcd(0x27);

const int TEMP_PIN = A1;

const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;

float R = 0.0;
float T = 0.0; //Kelvin

void setup() {
  int error;
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting Lab 1.5!");
  Serial.println("Check for LCD");

  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("LCD found");

    lcd.begin(16, 2); // initialize the lcd
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
    lcd.print("Temperature:");

  } else {
    Serial.println("LCD not found.");
  }

}

void loop() {

  int sensorValue = analogRead(TEMP_PIN); // = Vsig

  R = ((Vcc / sensorValue) - 1.0) * R0;

  T = (1 / ((log(R / R0) / B) + (1 / 298.15))) - 273.15; // Temperatura in gradi celsius

  lcd.setCursor(12, 0);
  lcd.print(T);

  delay(5000);
}
