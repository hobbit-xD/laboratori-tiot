#include <math.h>
#include <TimerOne.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>

#define TEMP_PIN A1
#define LED_PIN 5 //Pin PWN della scheda
#define FAN_PIN 6
#define NOISE_PIN 7
#define PIR_PIN 8

#define DEBUG 1

LiquidCrystal_PCF8574 lcd(0x27);

typedef struct
{
  int tmin;
  int tmax;
  int tminP;
  int tmaxP;
} Temp;

Temp acTemp;
Temp htTemp;

float temp;
int fanSpeed;
int fanLCD;
int ledIntensity;
int ledLCD;
bool running = false;
bool isPresence = false;

int acTmin;
int acTmax;
int htTmin;
int htTmax;

int newData[8];

/*====== Costanti sensore temperatura ======*/

const long int R0 = 100000; //Ohm
const int B = 4275; //K
const float Vcc = 1023.0;
float R = 0.0;
float T = 0.0; //Kelvin
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

/** LCD **/
//Counter to change positions of pages
int page_counter = 1 ;     //To move beetwen pages
unsigned long previousMillis = 0;
unsigned long interval = 5000; //Desired wait time 5 seconds

/*============= Esercizio 9 bonus ==========*/
unsigned int currentClap = 0;
unsigned int previousClap = 0;
bool ledState = false;
bool justSwitched = false;
/*==========================================*/

// Inizializzo i sensori con le temperature massime e minime
void setupTemp(Temp* temp, int tmin, int tmax, int tminP, int tmaxP) {
  temp->tmin = tmin;
  temp->tmax = tmax;

  temp->tminP = tminP;
  temp->tmaxP = tmaxP;
}

//Funzione per leggere i valori di temperatura dal sensore
float readTemp() {

  int sensorValue = analogRead(TEMP_PIN); // = Vsig
  R = ((Vcc / sensorValue) - 1.0) * R0;
  T = (1 / ((log(R / R0) / B) + (1 / 298.15))) - 273.15; // Temperatura in gradi celsius

  return T;
}

// Funzione esercizio 9 (bonus)  -> controlla che il led è connesso in pwm quindi non posso usare digitalWrite
void bonusNOISE() {
  if (digitalRead(NOISE_PIN) == LOW) {
    if (currentClap == previousClap) {
      previousClap = millis();
      return;
    }
    if (justSwitched) {
      delay(600);
      justSwitched = false;
    } else {
      currentClap = millis();
      if (currentClap > previousClap + 200 && currentClap < previousClap + 600) {
        ledState = !ledState;
        analogWrite(LED_PIN, ledState*255);
        justSwitched = true;
      }
    }
  }
  previousClap = currentClap - 1;
}

// Funzione che mi ritorna true se almeno uno dei due sensori rileva movimento -> persone nella stanza
bool checkPresence() {
  return checkPIR() || noise_presence;
}

//Funzione per verificare se c'è qualcuno nella stanza attraverso il sensore PIR
bool checkPIR() {
  if (digitalRead(PIR_PIN)) {
    pir_presence = true;
    pir_time = millis() / 1000;
    //Serial.println("Individuata persona");
  } else {
    if (millis() / 1000 - pir_time > pir_timeout) {
      pir_presence = false;
      //Serial.println("Nessuno presente");
    }
  }
  return pir_presence;
}

//Funzione per verificare se c'è qualcuno nella stanza attraverso il sensore di rumore
void checkNOISE() {
  unsigned int t = millis() / 1000;

  if (t - noise_time < noise_interval) {
    noise_count++;
    //Serial.print("Evento: ");
    //Serial.println(noise_count);
  } else {
    noise_count = 0;
    noise_time = t;
  }

  if (noise_count > noise_events) {
    noise_count = 0;
    noise_time = t;
    noise_presence = true;
    //Serial.println("Rilevata presenza");
    timer_counter = 0;
    Timer1.restart();
  }
}

//timer1 può essere inizializzato a max 8 secondi, così si risolve
void noiseTimeout() {
  timer_counter++;

  if (timer_counter > noise_timeout / 8) {
    //Serial.println("Reset noise presence");
    noise_presence = false;
    timer_counter = 0;
    Timer1.restart();
    Timer1.stop();
  }
}

void lcdDisplay() {

  switch (page_counter) {

    case 1: {    //Design of home page 1
        lcd.home();
        lcd.print("T:");
        lcd.print(temp);
        lcd.setCursor(8, 0);
        lcd.print("Pres:");
        if (isPresence == true) {
          lcd.print("YES");
        }
        else {
          lcd.print("NO");
        }

        lcd.setCursor(0, 1);
        lcd.print("AC:");
        lcd.print(fanLCD);
        lcd.print("%");
        lcd.setCursor(8, 1);
        lcd.print("HT:");
        lcd.print(ledLCD);
        lcd.print("%");

      }
      break;

    case 2: { //Design of page 2
        lcd.home();
        lcd.print("AC ");
        lcd.print("m:");
        lcd.print(acTmin);
        lcd.print(" M:");
        lcd.print(acTmax);
        lcd.setCursor(0, 1);
        lcd.print("HT ");
        lcd.print("m:");
        lcd.print(htTmin);
        lcd.print(" M:");
        lcd.print(htTmax);
      }
      break;
  }


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    lcd.clear();
    if (page_counter < 2) {
      page_counter = page_counter + 1;
    }
    else {
      page_counter = 1;
    }
  }
}

void setup() {
  int error;
  Serial.begin(9600);
  while (!Serial);

  //Inizializzo il motore e pongo la velocità a 0
  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, 0);

  //Inizializzo il sensore di temperatura
  pinMode(TEMP_PIN, INPUT);

  //Inizializzo sensore PIR
  pinMode(PIR_PIN, INPUT);

  //Inizializzo sensore di rumore
  pinMode(NOISE_PIN, INPUT);

  //Inizializzo il LED
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  //Interrupt per gestire sensore di rumore
  attachInterrupt(digitalPinToInterrupt(NOISE_PIN), checkNOISE, FALLING);
  Timer1.initialize(8e6);
  Timer1.attachInterrupt(noiseTimeout);

  //Controllo che connessione con LCD sia andata a buon fine
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("LCD found");

    lcd.begin(16, 2); // initialize the lcd
    lcd.setBacklight(255);
    lcd.home();
    lcd.clear();
  } else {
    Serial.println("LCD not found.");
  }

  Serial.println("Starting Lab HW 2");
  Serial.println("Che valori di temperatura vuoi utilizzare? (Scrivere il numero corrispondente nel monitor serial, è possibile cambiare opzione in qualsiasi momento dell' esecuzione)");
  Serial.println("\tA) Valori impostati in automatico dal programma per i due casi (stanza vuota e stanza con persone)");
  Serial.println("\tB) Impostare manualmente i valori");

}

void loop() {

  /* Punto 8 */
  if (Serial.available() > 0)
  {
    switch (Serial.read())
    {
      case 'A':
        Serial.println("Valori di default attivi");
        //Le temperature sono state scelte per risparmiare energia, se non c'è nessuno in casa abbiamo un range più alto per evitare che il condizionatore
        //stia sempre acceso anche quando non ci siamo.
        setupTemp(&acTemp, 27, 35, 20, 26);
         //Le temperature sono state scelte per risparmiare energia, se non c'è nessuno in casa abbiamo un range più basso per evitare che il riscaldamento
        //stia sempre acceso anche quando non ci siamo.       
        setupTemp(&htTemp, 10, 15, 16, 21);
        running = true;
        break;

      case 'B':
        Serial.println("Inserimento valori manuali attivo");
        Serial.println("Inserire tutti i valori in un unica riga sia per l'AC che per l'HT separati da una spazio");
        Serial.println("Hai 40 secondi per inserire i valori");
        Serial.println("acTminS acTmaxS acTminP acTmaxP htTminS htTmaxS htTminP htTmaxP");

        running = false;
        delay(40000);

        int i = 0;
        while (Serial.available() > 0) {

          // read the incoming byte:
          if (i < 8) {
            newData[i] = Serial.parseInt();
            i++;
          } else {
            i = 0;
          }
        }
        setupTemp(&acTemp, newData[0], newData[1], newData[2], newData[3]);
        setupTemp(&htTemp, newData[4], newData[5], newData[6], newData[7]);
        running = true;
        Serial.println("B");
        break;
    }
  }

  if (running) {

    //Leggo la temperatura
    temp = readTemp();

#if DEBUG
    Serial.print("Temp: ");
    Serial.println(temp);
#endif
    isPresence = checkPresence();

    /*====================================== Punto 6 ==========================================*/
    if (isPresence == true) {

      Serial.println("Ho rilevato presenza di persone nella stanza, quindi uso il secondo set di temperature");
      acTmin = acTemp.tminP;
      acTmax = acTemp.tmaxP;

      htTmin = htTemp.tminP;
      htTmax = htTemp.tmaxP;
    }
    else if (isPresence == false) {

      Serial.println("NON ho rilevato presenza di persone nella stanza, quindi uso il primo set di temperature");
      acTmin = acTemp.tmin;
      acTmax = acTemp.tmax;

      htTmin = htTemp.tmin;
      htTmax = htTemp.tmax;
    }
    /*==========================================================================================*/

#if DEBUG
    Serial.print("AC: ");
    Serial.print("\tTmin: ");
    Serial.print(acTmin);
    Serial.print(" - ");
    Serial.print("Tmax: ");
    Serial.println(acTmax);

    Serial.print("HT: ");
    Serial.print("\tTmin: ");
    Serial.print(htTmin);
    Serial.print(" - ");
    Serial.print("Tmax: ");
    Serial.println(htTmax);
#endif

    /*====================================== Punto 1 ==========================================*/
    if (temp < acTmin) {
      //Siamo al di sotto della temperatura minima quindi la ventola non fa nulla
      fanSpeed = 0;
      fanLCD = 0;
      analogWrite(FAN_PIN, fanSpeed);
    }
    else if (temp >= acTmin && temp <= acTmax) {

      //Valore di temperatura all'interno del range per il condizionatore
      //Calcolo la velocità della ventola
      fanSpeed = map(temp, acTmin, acTmax, 0, 255);
      fanLCD = map(temp, acTmin, acTmax, 0, 100);
      analogWrite(FAN_PIN, fanSpeed);

#if DEBUG
      Serial.print("Speed: ");
      Serial.println(fanSpeed);
#endif
    }
    else if ( temp > acTmax) {
      //Siamo al di sopra della temperatura massima quindi la ventola la lascio al massimo
      fanSpeed = 255;
      fanLCD = 100;
      analogWrite(FAN_PIN, fanSpeed);
    }
    /*==========================================================================================*/

    /*===================================== Punto 2 ============================================*/
    if (temp < htTmin) {
      //Siamo al di sotto della temperatura minima quindi led è a potenza massima
      ledIntensity = 255;
      ledLCD = 255;
      analogWrite(LED_PIN, ledIntensity);
    }
    else if (temp >= htTmin && temp <= htTmax) {

      //Valore di temperatura all'interno del range per il riscaldatore
      //Calcolo la intensità del led

      ledIntensity = map(temp, htTmax, htTmin, 0, 255);
      ledLCD = map(temp, htTmax, htTmin, 0, 100);
      analogWrite(LED_PIN, ledIntensity);

#if DEBUG
      Serial.print("Led: ");
      Serial.println(ledIntensity);
#endif
    }
    else if ( temp > htTmax) {
      //Siamo al di sopra della temperatura massima quindi led è a potenza minima
      ledIntensity = 0;
      ledLCD = 0;
      analogWrite(LED_PIN, ledIntensity);
    }
    /*=========================================================================================*/

    lcdDisplay();
    delay(1000);
  }

}
