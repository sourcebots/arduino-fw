#include <limits.h>

#include <Adafruit_PWMServoDriver.h>

static Adafruit_PWMServoDriver SERVOS = Adafruit_PWMServoDriver();

typedef String CommandResponse;
static const CommandResponse OK = "";

#define COMMAND_ERROR(x) ((x))

// Multiplying by this converts round-trip duration in microseconds to distance to object in millimetres.
static const float ULTRASOUND_COEFFICIENT = 1e-6 * 343.0 * 0.5 * 1e3;

static const String FIRMWARE_VERSION = "SourceBots PWM/GPIO v2019.6.0";

// Helpful things to process commands.

class CommandHandler {
  public:
    char command;
    CommandResponse (*run)(int, String argument);
    CommandHandler(char cmd, CommandResponse(*runner)(int, String));
};

CommandHandler::CommandHandler(char cmd, CommandResponse (*runner)(int, String))
  : command(cmd), run(runner)
{

}

static String pop_option(String& argument) {
  argument.trim();
  
  int separatorIndex = argument.indexOf(' ');
  if (separatorIndex == -1) {
    String copy(argument);
    argument = "";
    return copy;
  } else {
    String first_argument(argument.substring(0, separatorIndex));
    argument = argument.substring(separatorIndex + 1);
    return first_argument;
  }
}

static void serialWrite(int requestID, char lineType, const String& str) {
  if (requestID != 0) {
    Serial.write('@');
    Serial.print(requestID, DEC);
    Serial.write(' ');
  }

  Serial.write(lineType);
  Serial.write(' ');

  Serial.println(str);
}

// The actual commands

static void readAnaloguePin(int requestID, const String& name, int pin) {
  serialWrite(requestID, '>', name + " " + String(analogRead(pin)));
}

static CommandResponse analogueRead(int requestID, String argument) {
  readAnaloguePin(requestID, "a0", A0);
  readAnaloguePin(requestID, "a1", A1);
  readAnaloguePin(requestID, "a2", A2);
  readAnaloguePin(requestID, "a3", A3);
  return OK;
}

static CommandResponse led(int requestID, String argument) {

  char state = argument.charAt(0);

  switch (state) {
    case 'H':
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    case 'L':
      digitalWrite(LED_BUILTIN, LOW);
      break;
    default:
      return COMMAND_ERROR("Unknown LED State: " + argument);
  }

  return OK;
}

static CommandResponse readPin(int requestID, String argument) {
  String pinIDArg = pop_option(argument);

  if (argument.length() || !pinIDArg.length()) {
    return COMMAND_ERROR("Bad number of arguments");
  }

  int pin = pinIDArg.toInt();

  if (pin < 2 || pin > 13) {
    return COMMAND_ERROR("pin must be between 2 and 13");
  }

  int state = digitalRead(pin);

  if (state == HIGH) {
    serialWrite(requestID, '>', "H");
  } else {
    serialWrite(requestID, '>', "L");
  }

  return OK;
}

static CommandResponse servo(int requestID, String argument) {
  String servoArg = pop_option(argument);
  String widthArg = pop_option(argument);

  if (argument.length() || !servoArg.length() || !widthArg.length()) {
    return COMMAND_ERROR("Arguments Required: <num> <width>");
  }

  auto width = widthArg.toInt();
  auto servo = servoArg.toInt();
  if (servo < 0 || servo > 15) {
    return COMMAND_ERROR("Servo out of range");
  }
  if (width != 0 && (width < 150 || width > 550)) {
    return COMMAND_ERROR("Width must be 0 or between 150 and 550");
  }
  SERVOS.setPWM(servo, 0, width);
  return OK;
}

static void ultrasoundTriggerIO(int triggerPin, int echoPin){
  // Reset trigger pin.
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Pulse trigger pin.
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Set echo pin to input now (we don't do it earlier, since it's allowable
  // for triggerPin and echoPin to be the same pin).
  pinMode(echoPin, INPUT);
}

static CommandResponse ultrasoundReadTiming(int requestID, String argument){
  String triggerPinStr = pop_option(argument);
  String echoPinStr = pop_option(argument);

  if (argument.length() || !triggerPinStr.length() || !echoPinStr.length()) {
    return COMMAND_ERROR("Arguments required: <trigger> <echo>");
  }

  int triggerPin = triggerPinStr.toInt();
  int echoPin = echoPinStr.toInt();

  if (triggerPin < 2 || triggerPin > 13 || echoPin < 2 || echoPin > 13) {
    return COMMAND_ERROR("Pins must be between 2 and 13");
  }

  // Reset trigger pin.
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Pulse trigger pin.
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Set echo pin to input now (we don't do it earlier, since it's allowable
  // for triggerPin and echoPin to be the same pin).
  pinMode(echoPin, INPUT);

  // Read return pulse.
  float duration = (float) pulseIn(echoPin, HIGH);  // In microseconds.

  serialWrite(requestID, '>', String(duration));

  return OK;
}

static CommandResponse ultrasoundReadDistance(int requestID, String argument) {
  String triggerPinStr = pop_option(argument);
  String echoPinStr = pop_option(argument);

  if (argument.length() || !triggerPinStr.length() || !echoPinStr.length()) {
    return COMMAND_ERROR("Arguments required: <trigger> <echo>");
  }

  int triggerPin = triggerPinStr.toInt();
  int echoPin = echoPinStr.toInt();

  if (triggerPin < 2 || triggerPin > 13 || echoPin < 2 || echoPin > 13) {
    return COMMAND_ERROR("Pins must be between 2 and 13");
  }

  // Reset trigger pin.
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Pulse trigger pin.
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Set echo pin to input now (we don't do it earlier, since it's allowable
  // for triggerPin and echoPin to be the same pin).
  pinMode(echoPin, INPUT);

  // Read return pulse.
  float duration = (float) pulseIn(echoPin, HIGH);       // In microseconds.
  float distance = duration * ULTRASOUND_COEFFICIENT;    // In millimetres.
  distance = constrain(distance, 0.0, (float) UINT_MAX); // Ensure that the next line won't overflow.
  unsigned int distanceInt = (unsigned int) distance;

  serialWrite(requestID, '>', String(distanceInt));

  return OK;
}

static CommandResponse version(int requestID, String argument) {
  serialWrite(requestID, '>', FIRMWARE_VERSION);
  return OK;
}

static CommandResponse writePin(int requestID, String argument) {
  String pinIDArg = pop_option(argument);
  String pinStateArg = pop_option(argument);

  if (argument.length() || !pinIDArg.length() || !pinStateArg.length()) {
    return COMMAND_ERROR("Arguments Needed: <num> <state>");
  }

  int pin = pinIDArg.toInt();

  if (pin < 2 || pin > 13) {
    return COMMAND_ERROR("pin must be between 2 and 13");
  }

  switch (pinStateArg.charAt(0)) {
    case 'H':
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      break;
    case 'L':
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      break;
    case 'P':
      pinMode(pin, INPUT_PULLUP);
      break;
    case 'Z':
      pinMode(pin, INPUT); // High Impedance
      break;
    default:
      return COMMAND_ERROR("Unknown pin mode");

  }
  return OK;
}

// Process the commands and execute them.

static const CommandHandler commands[] = {
  CommandHandler('A', &analogueRead), // Read the analogue pins
  CommandHandler('L', &led), // Control the debug LED (H/L)
  CommandHandler('R', &readPin), // Read a digital pin <number>
  CommandHandler('S', &servo), // Control a servo <num> <width>
  CommandHandler('T', &ultrasoundReadTiming), // Read an ultrasound raw timing
  CommandHandler('U', &ultrasoundReadDistance), // Read an ultrasound distance
  CommandHandler('V', &version), // Get firmware version
  CommandHandler('W', &writePin), // Write to or  a GPIO pin <number> <state>
};

static void dispatch_command(int requestID, const class CommandHandler& handler, const String& argument) {
  auto response = handler.run(requestID, argument);
  if (response == OK) {
    serialWrite(requestID, '+', "OK");
  } else {
    serialWrite(requestID, '-', "Error: " + response);
  }
}

static void handle_actual_command(int requestID, const String& cmd) {
  char commandIssued = cmd.charAt(0);

  for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
    const CommandHandler& handler = commands[i];

    if (handler.command == commandIssued) {
      dispatch_command(requestID, handler, cmd.substring(1));
      return;
    }
  }

  serialWrite(requestID, '-', String("Error, unknown command: ") + commandIssued);
}

static void handle_command(const String& cmd) {
  // A command is prefixed with @ if an identifier is used to prevent race conditions
  if (cmd.startsWith("@")) {
    auto spaceIndex = cmd.indexOf(' ');
    auto requestID = cmd.substring(1, spaceIndex).toInt();
    handle_actual_command(requestID, cmd.substring(spaceIndex + 1));
  } else {
    handle_actual_command(0, cmd);
  }
}

static String serialBuffer;
static boolean skipWS = false;

static void process_serial() {
  auto serialInput = Serial.read();

  // Allow resetting the buffer by sending a NULL. This allows for recovery
  // from partial commands being sent or received.
  if (serialInput == 0) {
    serialBuffer = "";
    return;
  }

  if (serialInput == -1) {
    return;
  }

  if (serialInput == '\r') {
    return; // ignore CR, just take the LF
  }

  if (serialInput == '\t') {
    serialInput = ' '; // treat tabs as equivalent to spaces
  }

  if (serialInput == '\n') {
    serialBuffer.trim();
    Serial.write("# ");
    Serial.write(serialBuffer.c_str());
    Serial.write('\n');
    handle_command(serialBuffer);
    serialBuffer = "";
    Serial.flush();
    return;
  }

  if (serialInput == ' ' && skipWS) {
    return; // ignore junk whitespace
  } else {
    skipWS = (serialInput == ' '); // ignore any successive whitespace
  }

  serialBuffer += (char)serialInput;
}


void setup() {
  // Setup the pins.
  pinMode(LED_BUILTIN, OUTPUT);
  for (int pin = 2; pin <= 13; ++pin) {
    pinMode(pin, INPUT);
  }

  Serial.begin(115200);
  Serial.setTimeout(5);

  SERVOS.begin();
  SERVOS.setPWMFreq(50);

  Serial.write("# Booted\n");
  serialWrite(0, '#', FIRMWARE_VERSION);
}

void loop() {
  while (Serial.available()) {
    process_serial();
  }
}
