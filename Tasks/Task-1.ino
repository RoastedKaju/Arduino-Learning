#include <Servo.h>

const int POT_PIN = A0;
const int SERVO_PIN = 3;
const int RED_PIN = 7;
const int BLUE_PIN = 8;

Servo servo;

int oldAngle = -100;

enum class Region {
  none,
  low,
  mid,
  high
};
// start with invalid state so we can force an initial update of our LED states
Region currentRegion = Region::none;

Region getRegion(const int angle) {
  if(angle <= 59) {
    return Region::low;
  }
  if(angle <= 119) {
    return Region::mid;
  }
  if(angle <= 180) {
    return Region::high;
  }

  return Region::low;
}

bool nearlyEqual(const int A, const int B, const int D) {
  int value = abs(A - B);
  if(value < D) {
    return true;
  }

  return false;
}


void setup() {
  Serial.begin(9600);

  servo.attach(SERVO_PIN);

  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

void loop() {
  const int potValue = analogRead(POT_PIN);
  const int angle = map(potValue, 0, 1023, 0, 180);

  // returning from super loop like this is avoided in real life as it halts everything
  if(nearlyEqual(angle, oldAngle, 2)) {
    return;
  }

  Region region = getRegion(angle);
  switch(region) {
    case Region::low:
    if(region != currentRegion) {
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(BLUE_PIN, LOW);
      Serial.println("Entered LOW region");
      currentRegion = region;
    }
    break;
    case Region::mid:
    if(region != currentRegion) {
      digitalWrite(RED_PIN, LOW);
      digitalWrite(BLUE_PIN, HIGH);
      Serial.println("Entered MID region");
      currentRegion = region;
    }
    break;
    case Region::high:
    if(region != currentRegion) {
      digitalWrite(RED_PIN, HIGH);
      digitalWrite(BLUE_PIN, HIGH);
      Serial.println("Entered HIGH region");
      currentRegion = region;
    }
    break;
  }

  // rotate servo
  servo.write(angle);

  // update old angle
  oldAngle = angle;
}

