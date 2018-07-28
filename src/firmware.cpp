#include <limits.h>

#include <Adafruit_PWMServoDriver.h>

static Adafruit_PWMServoDriver SERVOS = Adafruit_PWMServoDriver();

typedef String CommandResponse;
static const CommandResponse OK = "";

#define COMMAND_ERROR(x) ((x))

// Multiplying by this converts round-trip duration in microseconds to distance to object in millimetres.
static const float ULTRASOUND_COEFFICIENT = 1e-6 * 343.0 * 0.5 * 1e3;

static const String FIRMWARE_VERSION = "SourceBots PWM/GPIO v2.0.0";

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

static void serialWrite(int commandId, char lineType, const String& str) {
    if (commandId != 0) {
        Serial.write('@');
        Serial.print(commandId, DEC);
        Serial.write(' ');
    }

    Serial.write(lineType);
    Serial.write(' ');

    Serial.println(str);
}

// The actual commands

static CommandResponse led(int commandId, String argument) {
  if (argument == "H") {
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (argument == "L") {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    return COMMAND_ERROR("Unknown LED State: " + argument);
  }
  return OK;
}

static CommandResponse servo(int commandId, String argument) {
  String servoArg = pop_option(argument);
  String widthArg = pop_option(argument);

  if (argument.length() || !servoArg.length() || !widthArg.length()) {
    return COMMAND_ERROR("Bad number of arguments.");
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

static CommandResponse version(int commandId, String argument) {
  serialWrite(commandId, '>', FIRMWARE_VERSION);
  return OK;
}

static CommandResponse writePin(int commandId, String argument) {
  String pinIDArg = pop_option(argument);
  String pinStateArg = pop_option(argument);

  if (argument.length() || !pinIDArg.length() || !pinStateArg.length()) {
    return COMMAND_ERROR("need exactly two arguments: <pin> <high/low/hi-z/pullup>");
  }

  int pin = pinIDArg.toInt();

  if (pin < 2 || pin > 13) {
    return COMMAND_ERROR("pin must be between 2 and 12");
  }

  switch(pinStateArg.charAt(0)){
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
      return COMMAND_ERROR("Unknown pin mode.");
    
  }
  return OK;
}

// Process the commands and execute them.

static const CommandHandler commands[] = {
  CommandHandler('L', &led), // Control the debug LED (H/L)
  CommandHandler('S', &servo), // Control a servo <num> <width>
  CommandHandler('V', &version), // Get firmware version
  CommandHandler('W', &writePin), // Write to or  a GPIO pin <number> <state>
};

static void dispatch_command(int commandId, const class CommandHandler& handler, const String& argument) {
  auto err = handler.run(commandId, argument);
  if (err == OK) {
    serialWrite(commandId, '+', "OK");
  } else {
    serialWrite(commandId, '-', "Error: " + err);
  }
}

static void handle_actual_command(int commandId, const String& cmd) {
  
    char commandIssued = cmd.charAt(0);
    
    for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
      const CommandHandler& handler = commands[i];

      if (handler.command == commandIssued) {
        dispatch_command(commandId, handler, cmd.substring(1));
        return;
      }
    }

    serialWrite(commandId, '-', String("Error, unknown command: ") + commandIssued);
}

static void handle_command(const String& cmd) {
  // A command is prefixed with @ if an identifier is used to prevent race conditions
  if (cmd.startsWith("@")) {
    auto spaceIndex = cmd.indexOf(' ');
    auto commandId = cmd.substring(1, spaceIndex).toInt();
    handle_actual_command(commandId, cmd.substring(spaceIndex + 1));
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
  for (int pin = 2; pin <= 13; pin++) {
    pinMode(pin, INPUT);
  }

  Serial.begin(9600);
  Serial.setTimeout(5);

  SERVOS.begin();
  SERVOS.setPWMFreq(50);

  Serial.write("# Booted.\n");

}

void loop() {
  while (Serial.available()) {
   process_serial();
  }
}

