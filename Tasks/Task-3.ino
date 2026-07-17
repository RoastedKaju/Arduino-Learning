/**
* Read temperature, use potentiometer to set max temperature limit.
* Set LED to correct lighting mode based on different states.
* Display current temperature and limit on LCD.
* Use button to change the temperature shown from Celsius to Fahrenheit,
* Apply software button debounce.
*/

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

const int POT_PIN = A5;
const int DHT_PIN = 2;
const int RED_PIN = 13;
const int GREEN_PIN = 12;
const int BLUE_PIN = 8;
const int I2C_ADDR = 0x27;
const int BUZZER_PIN = 4;
const unsigned long BUZZER_DURATION = 100;
const int BTN_PIN = 7;

bool firstRun = true;
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);
DHT dht(DHT_PIN, DHT22);
int potValue = 0;
int limitTemp = 0;
float tempC = 0.0f;
float tempF = 0.0f;
float humid = 0.0f;
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
bool showTempInC = true;

enum class State {
  safe,
  warn,
  alarm
};
State currentState = State::safe;
State prevState = currentState;

void readPotMeter() {
  potValue = analogRead(POT_PIN);
  limitTemp = map(potValue, 0, 1023, -40, 80);
}

void readTemp() {
  tempC = dht.readTemperature();
  tempF = dht.readTemperature(true);
  humid = dht.readHumidity();

  Serial.print("Limit is: ");
  Serial.print(limitTemp);
  Serial.print(" Current Temp is: ");
  Serial.println(tempC);

  // Check if readings failed
  if (isnan(humid) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT22 sensor!");
    return;
  }
}

void refreshLCD() {
  float temp;
  float limit;
  if(showTempInC) {
    temp = tempC;
    limit = limitTemp;
  } else {
    temp = tempF;
    limit = (limitTemp * 1.8) + 32;
  }
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(showTempInC ? " C" : " F");
  lcd.setCursor(0, 1);
  lcd.print("Limit: ");
  lcd.print(limit);
  lcd.print(showTempInC ? " C" : " F");
}

void blinkLED() {
  if(tempC >= limitTemp) {
    // alarm
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
    currentState = State::alarm;
  }
  else if (tempC >= (limitTemp - 2) && tempC < limitTemp) {
    // warning
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    currentState = State::warn;
  }
  else if(tempC < limitTemp - 2) {
    // safe
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
    currentState = State::safe;
  }
}

void buzzer() {
  if (currentState != prevState) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerStartTime = millis();
    buzzerActive = true;

    prevState = currentState;
  }

  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

void button() {
  const int value = digitalRead(BTN_PIN);
  if(showTempInC != value) {
    showTempInC = value;
  }
}

struct Task {
  unsigned long prev;
  unsigned long period;
  void (*func)();
};

Task tasks[] = {
  {0, 50, readPotMeter},
  {0, 2000, readTemp},
  {0, 500, refreshLCD},
  {0, 500, blinkLED},
  {0, 50, buzzer},
  {0, 10, button}
};

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  // pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
}

void loop() {
  unsigned long now = millis();

  if(firstRun) {
    for(auto& task : tasks) {
      task.func();
    }
    firstRun = false;
  }

  for(auto& task : tasks) {
    if(now - task.prev >= task.period) {
      task.prev += task.period;
      task.func();
    }
  }
}
