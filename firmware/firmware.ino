#include <Adafruit_PWMServoDriver.h>

static const String FIRMWARE_VERSION = "SourceBots PWM/GPIO v0.0.1";

typedef String CommandError;

static const CommandError OK = "";

#define COMMAND_ERROR(x) ((x))

static Adafruit_PWMServoDriver SERVOS = Adafruit_PWMServoDriver();

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.setTimeout(5);

  SERVOS.begin();
  SERVOS.setPWMFreq(50);
}

class CommandHandler {
public:
  String command;
  CommandError (*run)(String argument);
  String helpMessage;

  CommandHandler(String cmd, CommandError (*runner)(String), String help);
};

CommandHandler::CommandHandler(String cmd, CommandError (*runner)(String), String help)
: command(cmd), run(runner), helpMessage(help)
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

static CommandError run_help(String argument);

static CommandError led(String argument) {
  if (argument == "on") {
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (argument == "off") {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    return COMMAND_ERROR("unknown argument");
  }
  return OK;
}

static CommandError servo(String argument) {
  String servoArg = pop_option(argument);
  String widthArg = pop_option(argument);

  if (argument.length() || !servoArg.length() || !widthArg.length()) {
    return COMMAND_ERROR("servo takes exactly two arguments");
  }

  auto width = widthArg.toInt();
  auto servo = servoArg.toInt();
  if (servo < 0 || servo > 15) {
    return COMMAND_ERROR("servo index out of range");
  }
  if (width != 0 && (width < 150 || width > 550)) {
    return COMMAND_ERROR("width must be 0 or between 150 and 550");
  }
  SERVOS.setPWM(servo, 0, width);
  return OK;
}

static CommandError write_pin(String argument) {
  String pinIDArg = pop_option(argument);
  String pinStateArg = pop_option(argument);

  if (argument.length() || !pinIDArg.length() || !pinStateArg.length()) {
    return COMMAND_ERROR("need exactly two arguments: <pin> <high/low/hi-z/pullup>");
  }

  int pin = pinIDArg.toInt();

  if (pin < 2 || pin > 12) {
    return COMMAND_ERROR("pin must be between 2 and 12");
  }

  if (pinStateArg == "high") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  } else if (pinStateArg == "low") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else if (pinStateArg == "hi-z") {
    pinMode(pin, INPUT);
  } else if (pinStateArg == "pullup") {
    pinMode(pin, INPUT_PULLUP);
  } else {
    return COMMAND_ERROR("unknown drive state");
  }
  return OK;
}

static CommandError read_pin(String argument) {
  String pinIDArg = pop_option(argument);

  if (argument.length() || !pinIDArg.length()) {
    return COMMAND_ERROR("need exactly one argument: <pin>");
  }

  int pin = pinIDArg.toInt();

  if (pin < 2 || pin > 12) {
    return COMMAND_ERROR("pin must be between 2 and 12");
  }

  auto state = digitalRead(pin);

  if (state == HIGH) {
    Serial.write("> high\n");
  } else {
    Serial.write("> low\n");
  }

  return OK;
}

static CommandError get_version(String argument) {
  Serial.write("> ");
  Serial.write(FIRMWARE_VERSION.c_str());
  Serial.write('\n');
  return OK;
}

static const CommandHandler commands[] = {
  CommandHandler("help", &run_help, "show information"),
  CommandHandler("led", &led, "control the debug LED (on/off)"),
  CommandHandler("servo", &servo, "control a servo <num> <width>"),
  CommandHandler("version", &get_version, "get firmware version"),
  CommandHandler("gpio-write", &write_pin, "set output from GPIO pin"),
  CommandHandler("gpio-read", &read_pin, "get digital input from GPIO pin"),
};

static void dispatch_command(const class CommandHandler& handler, const String& argument) {
  auto err = handler.run(argument);
  if (err == OK) {
    Serial.write("+ OK\n");
  } else {
    Serial.write("- Error: ");
    Serial.write(err.c_str());
    Serial.write('\n');
  }
}

static void handle_command(const String& cmd) {
  for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
    const CommandHandler& handler = commands[i];

    if (handler.command == cmd) {
      dispatch_command(handler, "");
      return;
    } else if (cmd.startsWith(handler.command + " ")) {
      dispatch_command(
        handler,
        cmd.substring(handler.command.length() + 1)
      );
      return;
    }
  }

  Serial.write("- Error: unknown command\n");
}

static CommandError run_help(String argument) {
  if (argument == "") {
    Serial.write("# commands: \n");
    for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
      const CommandHandler& handler = commands[i];
      Serial.write("#   ");
      Serial.write(handler.command.c_str());
      for (int i = handler.command.length(); i < 30; ++i) {
        Serial.write(' ');
      }
      Serial.write(handler.helpMessage.c_str());
      Serial.write('\n');
    }
    return OK;
  } else {
    for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
      const CommandHandler& handler = commands[i];
      if (handler.command == argument) {
        Serial.write("# ");
        Serial.write(handler.command.c_str());
        Serial.write("\n#  ");
        Serial.write(handler.helpMessage.c_str());
        Serial.write('\n');
        return OK;
      }
    }
  }
  return COMMAND_ERROR("I do not know anything about that topic");
}

static String serialBuffer;
static boolean skipWS = false;

static void process_serial() {
  auto serialInput = Serial.read();

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

void loop() {
  while (Serial.available()) {
    process_serial();
  }
}
