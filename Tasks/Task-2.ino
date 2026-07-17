#include <Servo.h>

const int LED_PIN = 8;
const int POT_PIN = A0;
const int SER_PIN = 3;

const unsigned long timerPot = 20;
unsigned long prevPotTime = 0;
const unsigned long timerSer = 50;
unsigned long prevSerTime = 0;
const unsigned long timerBlink = 250;
unsigned long prevBlinkTime = 0;
const unsigned long timerSerial = 1000;
unsigned long prevSerialTime = 0;

Servo servo;
int currentPotValue = 0;
int currentServoAngle = 0;
bool isLedOn = false;


void taskReadPot() {
  currentPotValue = analogRead(POT_PIN);
}

void taskUpdateSer() {
  currentServoAngle = map(currentPotValue, 0, 1023, 0, 180);
  servo.write(currentServoAngle);
}

void taskBlink() {
  isLedOn = !isLedOn;
  digitalWrite(LED_PIN, isLedOn);
}

void taskPrint() {
  Serial.print("Servo angle: ");
  Serial.println(currentServoAngle);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  servo.attach(SER_PIN);
}

void loop() {
  unsigned long now = millis();

  if(now - prevPotTime >= timerPot) {
    taskReadPot();
    prevPotTime += timerPot;
  }

  if(now - prevSerTime >= timerSer) {
    taskUpdateSer();
    prevSerTime += timerSer;
  }

  if(now - prevBlinkTime >= timerBlink) {
    taskBlink();
    prevBlinkTime += timerBlink;
  }

  if(now - prevSerialTime >= timerSerial) {
    taskPrint();
    prevSerialTime += timerSerial;
  }

}
