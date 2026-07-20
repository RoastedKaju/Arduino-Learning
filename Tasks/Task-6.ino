// In Arduino only pin 2 and 3 (INT 0) and (INT 1) can have interrupts.
const int BTN_PIN = 2;
const int LED_PIN = 7;
const int BUZ_PIN = 8;

volatile int doorbell_trigger_count = 0;
volatile unsigned long last_int_time = 0;

int pending_led_sessions = 0;
bool is_led_on = false;
bool blink_active = false;
unsigned long blink_start_time = 0;
const unsigned long blink_duration = 800; // How long it blinks total
const unsigned long blink_rate = 100;     // Speed of the blink toggle
unsigned long last_blink = 0;

int pending_serial_prints = 0;

int pending_buzzer_sessions = 0;
bool is_buzzer_on = false;
bool buzzer_active = false;
unsigned long buzzer_start_time = 0;
const unsigned long buzzer_duration = 300;
const unsigned long buzzer_rate = 50;
unsigned long last_buz = 0;


// ISR
void button_pressed() {
  unsigned long now = millis();

  if (now - last_int_time > 200) {
    ++doorbell_trigger_count;
    last_int_time = now;
  }
}

void process_events() {
  if (doorbell_trigger_count > 0) {
    noInterrupts();
    int presses = doorbell_trigger_count;
    doorbell_trigger_count = 0;
    interrupts();

    // Distribute the event to whichever tasks need it
    pending_led_sessions += presses;
    pending_serial_prints += presses;
    pending_buzzer_sessions += presses;
  }
}

void update_led() {
  unsigned long now = millis();

  // Start a session if there's a pending press and we aren't already blinking
  if (pending_led_sessions > 0 && !blink_active) {
    pending_led_sessions--; // Consume one press request
    blink_active = true;
    blink_start_time = now;
    last_blink = now;
  }

  if (blink_active) {
    if (now - blink_start_time >= blink_duration) {
      blink_active = false;
      digitalWrite(LED_PIN, LOW);
      is_led_on = false;
    }
    else if (now - last_blink >= blink_rate) {
      is_led_on = !is_led_on;
      digitalWrite(LED_PIN, is_led_on ? HIGH : LOW);
      last_blink = now;
    }
  }
}

void serial_print() {
  if (pending_serial_prints > 0) {
    Serial.println("Doorbell pressed!");
    pending_serial_prints--;
  }
}

void update_buzzer() {
  unsigned long now = millis();

  if (pending_buzzer_sessions > 0 && !buzzer_active) {
    pending_buzzer_sessions--; // Consume one press request
    buzzer_active = true;
    buzzer_start_time = now;
    last_buz = now;
  }

  if (buzzer_active) {
    if (now - buzzer_start_time >= buzzer_duration) {
      buzzer_active = false;
      digitalWrite(BUZ_PIN, LOW);
      is_buzzer_on = false;
    }
    else if (now - last_buz >= buzzer_rate) {
      is_buzzer_on = !is_buzzer_on;
      digitalWrite(BUZ_PIN, is_buzzer_on ? HIGH : LOW);
      last_buz = now;
    }
  }
}

struct Task {
  unsigned long previous;
  unsigned long period;
  void (*func)();
};

Task tasks[] = {
  {0, 10, process_events},
  {0, 20, update_led},
  {0, 50, serial_print},
  {0, 20, update_buzzer}
};

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZ_PIN, OUTPUT);
  // 0 is INT 0: Digital pin 2, button_pressed is our callback, FALLING is our trigger
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), button_pressed, FALLING);

}

void loop() {
  unsigned long now = millis();

  for (auto& task : tasks) {
    if (now - task.previous >= task.period) {
      task.func();
      task.previous = now;
    }
  }
}