#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG true 

// pins
#define ONE_WIRE_BUS 10
#define BUTTON_PIN 9
#define HEATER_PIN 8
#define BATTERY_PIN A7  
#define LED_LOWBAT 2
#define LED_0 3
#define LED_1 4
#define LED_2 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float tempLow[4]  = {0, 66.0, 78.0, 91.0};
float tempHigh[4] = {0, 68.0, 80.0, 93.0};
int dutyCycles[4] = {0, 20, 50, 80}; 

int mode = 0; 
bool lastButtonState = HIGH;
float currentTempF = 0;
float batteryVoltage = 0;

const int pwmFrequency = 5;  
unsigned long lastToggleTime = 0;
bool pwmState = false;

unsigned long lastTempRequest = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastBatteryCheck = 0;
const int delayInMillis = 94; 

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(LED_LOWBAT, OUTPUT);
  pinMode(LED_0, OUTPUT); 
  pinMode(LED_1, OUTPUT); 
  pinMode(LED_2, OUTPUT);

  sensors.begin();
  sensors.setResolution(9);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  
  if (DEBUG) Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();

  // button logic
  bool reading = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && reading == LOW) {
    mode = (mode + 1) % 4;
    delay(80); 
  }
  lastButtonState = reading;

  // battery monitoring
  if (now - lastBatteryCheck >= 2000) {
    int raw = analogRead(BATTERY_PIN);
    
    batteryVoltage = raw * (3.3 / 1024.0) * 4.0;
    
    digitalWrite(LED_LOWBAT, (batteryVoltage < 10.8) ? HIGH : LOW);
    lastBatteryCheck = now;
  }

  // temp update
  if (now - lastTempRequest >= delayInMillis) {
    currentTempF = sensors.getTempFByIndex(0);
    sensors.requestTemperatures(); 
    lastTempRequest = now;
  }

  if (mode == 0) {
    allOff();
  } else {
    // leds
    digitalWrite(LED_0, (mode >= 1));
    digitalWrite(LED_1, (mode >= 2));
    digitalWrite(LED_2, (mode >= 3));

    // heater logic
    unsigned long period = 1000 / pwmFrequency;
    unsigned long onTime = (period * dutyCycles[mode]) / 100;
    unsigned long offTime = period - onTime;
    float midTemp = (tempLow[mode] + tempHigh[mode]) / 2.0;

    if (currentTempF < tempLow[mode]) {
      pwmState = true;
      digitalWrite(HEATER_PIN, HIGH);
    } else if (currentTempF >= midTemp) {
      pwmState = false;
      digitalWrite(HEATER_PIN, LOW);
    } else {
      if (pwmState && (now - lastToggleTime >= onTime)) {
        pwmState = false;
        digitalWrite(HEATER_PIN, LOW);
        lastToggleTime = now;
      } else if (!pwmState && (now - lastToggleTime >= offTime)) {
        pwmState = true;
        digitalWrite(HEATER_PIN, HIGH);
        lastToggleTime = now;
      }
    }
  }

  // debugging
  if (DEBUG && (now - lastSerialPrint >= 500)) {
    Serial.print("M: "); Serial.print(mode);
    Serial.print(" | T: "); Serial.print(currentTempF);
    Serial.print("F | Batt: "); Serial.print(batteryVoltage);
    Serial.print("V | Heat: "); Serial.println(digitalRead(HEATER_PIN) ? "ON" : "OFF");
    lastSerialPrint = now;
  }
}

void allOff() {
  digitalWrite(LED_0, LOW);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(HEATER_PIN, LOW);
}
