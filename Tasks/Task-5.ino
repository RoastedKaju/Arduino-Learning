#include <ctype.h>
#include <string.h>

const int BUFFER_SIZE = 64;
const int LED_PIN = 2;

struct RingBuffer
{
  char buffer[BUFFER_SIZE];
  size_t head = 0;
  size_t tail = 0;
  size_t count = 0;
  bool full = false;

  void push(char c)
  {
    if (full)
    {
      tail = (tail + 1) % BUFFER_SIZE;
    }
    else
    {
      ++count;
    }

    buffer[head] = c;

    head = (head + 1) % BUFFER_SIZE;
    full = (head == tail);
  }

  bool pop(char &c)
  {
    if (empty())
    {
      return false;
    }

    c = buffer[tail];
    full = false;
    tail = (tail + 1) % BUFFER_SIZE;
    --count;

    return true;
  }

  bool empty()
  {
    return (!full && (head == tail));
  }
};

RingBuffer console_buffer{};
int incoming_byte = 0;
size_t command_size = 0;
char command[BUFFER_SIZE];
bool is_led_on = false;

void led_command(char *arg)
{
  if (strcmp(arg, "on") == 0)
  {
    digitalWrite(LED_PIN, HIGH);
    is_led_on = true;
    return;
  }
  if (strcmp(arg, "off") == 0)
  {
    digitalWrite(LED_PIN, LOW);
    is_led_on = false;
    return;
  }
}

void echo_command()
{
  strtok(command, " ");
  char *statement = strtok(NULL, "");
  Serial.println(statement);
}

/**
 * You can turn this into a table, where key is command name and have a function pointer as value.
 * Then simply iterate over the table and call the function pointer if the command matches.
 */
void process_command()
{
  // tokens
  char tokens[BUFFER_SIZE];
  strcpy(tokens, command);

  char *first_token = strtok(tokens, " ");
  char *second_token = strtok(NULL, " ");

  if (strcmp(first_token, "help") == 0)
  {
    Serial.println("Available commands: led on, led off, status, clear");
    return;
  }
  if (strcmp(first_token, "clear") == 0)
  {
    for (int i = 0; i < 30; ++i)
    {
      Serial.println();
    }
    Serial.println("Type your commands!");
    return;
  }
  if (strcmp(first_token, "status") == 0)
  {
    Serial.println(is_led_on ? "LED is On" : "LED is Off");
    return;
  }
  if (strcmp(first_token, "led") == 0)
  {
    led_command(second_token);
    return;
  }
  if (strcmp(first_token, "echo") == 0)
  {
    echo_command();
    return;
  }

  Serial.println("Unknown command!");
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Type your commands!");
}

void loop()
{
  while (Serial.available())
  {
    incoming_byte = Serial.read();
    if (incoming_byte == '\n' || incoming_byte == '\r')
    {
      command_size = console_buffer.count;
      for (auto i = 0; i < command_size; ++i)
      {
        console_buffer.pop(command[i]);
        command[i] = tolower(command[i]);
      }

      command[command_size] = '\0';

      process_command();

      memset(command, 0, BUFFER_SIZE);
    }
    else
    {
      console_buffer.push(incoming_byte);
    }
  }
}
