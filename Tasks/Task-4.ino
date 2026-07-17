/***
 * Smart Garage Door Controller
 * You're writing firmware for a garage door, the door has one button.
 * Pressing the button starts opening or closing the door.
 * If the safety sensor detects something underneath while the door is closing, it must immediately reverse.
 * Everything is event-driven.
 */

#include <Servo.h>
#include <LiquidCrystal_I2C.h>

const int G_BTN_PIN = 2;
const int R_BTN_PIN = 7;
const int S_PIN = 3;
const int R_LED_PIN = 12;
const int G_LED_PIN = 13;
const int Y_LED_PIN = 4;
const int I2C_ADDR = 0x27;
const int BUZZER_PIN = 8;

enum class DoorState
{
  Closed,
  Opening,
  Open,
  Closing,
  Stopped
};

enum class ButtonEvent
{
  Unhandled,
  Handled
};

struct DebouncedButton {
  int state;
  int last_state;
  unsigned long last_time;
  const unsigned long delay;
  ButtonEvent event_state = ButtonEvent::Handled;

  // Constructor
  DebouncedButton(unsigned long d = 50) : state(HIGH), last_state(HIGH), last_time(0), delay(d) {}

  // returns valid when LOW is detected
  bool update(int current_reading) {
    bool triggered = false;

    // If the switch changed, due to noise or pressing:
    if (current_reading != last_state) {
      last_time = millis();
    }

    if ((millis() - last_time) > delay) {
      // If the state has changed and it's been stable longer than the delay
      if (current_reading != state) {
        state = current_reading;

        // Only trigger if the new state is LOW (assuming pull-up resistor)
        if (state == LOW) {
          triggered = true;
        }
      }
    }

    last_state = current_reading;
    return triggered;
  }
};

// Variables
DoorState cur_door_state = DoorState::Closed;
DoorState tar_door_state = DoorState::Closed;
DoorState last_door_state = DoorState::Closed;
DoorState last_tar_door_state = DoorState::Closed;
DebouncedButton red_btn(50);
DebouncedButton green_btn(50);
Servo servo;
int cur_servo_angle = 0;
int tar_servo_angle = 0;
DoorState led_state = DoorState::Stopped;
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);
DoorState lcd_state = DoorState::Closed;
DoorState buzz_state = DoorState::Closed;
unsigned long buzzer_start = 0;
const unsigned long buzzer_duration = 100;
bool is_buzzing = false;

// tasks
void update_door_angle() {
  if (cur_door_state == tar_door_state) {
    return;
  }

  switch (tar_door_state) {
    case DoorState::Open:
      if (cur_servo_angle == 180) {
        cur_door_state = DoorState::Open;
        Serial.println("Door Opened.");
        return;
      }
      cur_servo_angle += 5;
      servo.write(cur_servo_angle);
      break;
    case DoorState::Closed:
      if (cur_servo_angle == 0) {
        cur_door_state = DoorState::Closed;
        Serial.println("Door Closed.");
        return;
      }
      cur_servo_angle -= 5;
      servo.write(cur_servo_angle);
      break;
    case DoorState::Stopped:
      break;
  }
}

void process_button() {
  if (green_btn.event_state == ButtonEvent::Handled) {
    return;
  }

  // mark this press as handled
  green_btn.event_state = ButtonEvent::Handled;

  Serial.println("Updating state");
  switch (cur_door_state) {
    case DoorState::Closed:
      cur_door_state = DoorState::Opening;
      tar_door_state = DoorState::Open;
      break;
    case DoorState::Open:
      cur_door_state = DoorState::Closing;
      tar_door_state = DoorState::Closed;
      break;
    case DoorState::Opening:
      cur_door_state = DoorState::Stopped;
      tar_door_state = DoorState::Stopped;
      break;
    case DoorState::Closing:
      cur_door_state = DoorState::Stopped;
      tar_door_state = DoorState::Stopped;
      break;
    case DoorState::Stopped:
      cur_door_state = last_door_state;
      tar_door_state = last_tar_door_state;
      break;
  }

  // only save last state if current state is not at stopped position
  if (cur_door_state != DoorState::Stopped) {
    last_door_state = cur_door_state;
    last_tar_door_state = tar_door_state;
  }
}

void process_sensor() {
  if (red_btn.event_state == ButtonEvent::Handled) {
    return;
  }
  // mark this press as handled
  red_btn.event_state = ButtonEvent::Handled;
  // invalidate the green button as well
  green_btn.event_state = ButtonEvent::Handled;

  if (cur_door_state == DoorState::Closing) {
    cur_door_state = DoorState::Opening;
    tar_door_state = DoorState::Open;
    Serial.println("Sensor detected something, force opening door!");
  }
}

void update_leds() {
  if (led_state == cur_door_state) {
    return;
  }

  switch (cur_door_state) {
    case DoorState::Open:
      digitalWrite(R_LED_PIN, LOW);
      digitalWrite(G_LED_PIN, HIGH);
      digitalWrite(Y_LED_PIN, LOW);
      led_state = DoorState::Open;
      break;
    case DoorState::Closed:
      digitalWrite(R_LED_PIN, HIGH);
      digitalWrite(G_LED_PIN, LOW);
      digitalWrite(Y_LED_PIN, LOW);
      led_state = DoorState::Closed;
      break;
    case DoorState::Opening:
      digitalWrite(R_LED_PIN, LOW);
      digitalWrite(G_LED_PIN, LOW);
      digitalWrite(Y_LED_PIN, HIGH);
      led_state = DoorState::Opening;
      break;
    case DoorState::Closing:
      digitalWrite(R_LED_PIN, LOW);
      digitalWrite(G_LED_PIN, LOW);
      digitalWrite(Y_LED_PIN, HIGH);
      led_state = DoorState::Closing;
      break;
    case DoorState::Stopped:
      digitalWrite(R_LED_PIN, LOW);
      digitalWrite(G_LED_PIN, LOW);
      digitalWrite(Y_LED_PIN, LOW);
      led_state = DoorState::Stopped;
      break;
  }
}

void update_lcd() {
  if (lcd_state == cur_door_state) {
    return;
  }

  switch (cur_door_state) {
    case DoorState::Open:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("State: ");
      lcd.setCursor(0, 1);
      lcd.print("Open");
      lcd_state = DoorState::Open;
      break;
    case DoorState::Opening:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("State: ");
      lcd.setCursor(0, 1);
      lcd.print("Opening");
      lcd_state = DoorState::Opening;
      break;
    case DoorState::Closed:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("State: ");
      lcd.setCursor(0, 1);
      lcd.print("Closed");
      lcd_state = DoorState::Closed;
      break;
    case DoorState::Closing:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("State: ");
      lcd.setCursor(0, 1);
      lcd.print("Closing");
      lcd_state = DoorState::Closing;
      break;
    case DoorState::Stopped:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("State: ");
      lcd.setCursor(0, 1);
      lcd.print("Stopped");
      lcd_state = DoorState::Stopped;
      break;
  }

}

void buzzer_routine() {
  // turn off beep
  if (is_buzzing && (millis() - buzzer_start >= buzzer_duration)) {
    digitalWrite(BUZZER_PIN, LOW);
    is_buzzing = false;
  }

  if (buzz_state == cur_door_state) {
    if (is_buzzing) {
      digitalWrite(BUZZER_PIN, HIGH);
    }
    return;
  }

  switch (cur_door_state) {
    case DoorState::Open:
      is_buzzing = true;
      buzzer_start = millis();
      buzz_state = DoorState::Open;
      break;
    case DoorState::Closed:
      is_buzzing = true;
      buzzer_start = millis();
      buzz_state = DoorState::Closed;
      break;
  }
}

struct Task {
  unsigned long previous;
  unsigned long period;
  void (*function)();
};

Task tasks[] = {
  {0, 20, update_door_angle},
  {0, 50, process_button},
  {0, 10, process_sensor},
  {0, 100, update_leds},
  {0, 100, update_lcd},
  {0, 100, buzzer_routine}
};

void setup() {
  Serial.begin(9600);

  // buttons
  pinMode(G_BTN_PIN, INPUT_PULLUP);
  pinMode(R_BTN_PIN, INPUT_PULLUP);

  // Servo
  servo.attach(S_PIN);
  servo.write(cur_servo_angle);

  // LEDs
  pinMode(R_LED_PIN, OUTPUT);
  pinMode(G_LED_PIN, OUTPUT);
  pinMode(Y_LED_PIN, OUTPUT);
  // Start with red light on
  digitalWrite(R_LED_PIN, HIGH);
  digitalWrite(G_LED_PIN, LOW);
  digitalWrite(Y_LED_PIN, LOW);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("State: ");
  lcd.setCursor(0, 1);
  lcd.print("Closed");
}

void loop() {
  unsigned long now = millis();

  int red_value = digitalRead(R_BTN_PIN);
  int green_value = digitalRead(G_BTN_PIN);

  if (red_btn.update(red_value)) {
    red_btn.event_state = ButtonEvent::Unhandled;
  }

  if (green_btn.update(green_value)) {
    green_btn.event_state = ButtonEvent::Unhandled;
  }

  for (auto& task : tasks) {
    if (now - task.previous >= task.period) {
      task.previous += task.period;
      task.function();
    }
  }
}
