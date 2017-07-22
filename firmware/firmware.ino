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

  yield();
}

class CommandHandler {
public:
  String command;
  CommandError (*run)(const String& argument);
  String helpMessage;

  CommandHandler(String cmd, CommandError (*runner)(const String&), String help);
};

CommandHandler::CommandHandler(String cmd, CommandError (*runner)(const String&), String help)
: command(cmd), run(runner), helpMessage(help)
{
}

static CommandError run_help(const String& argument);

static CommandError led(const String& argument) {
  if (argument == "on") {
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (argument == "off") {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    return COMMAND_ERROR("unknown argument");
  }
  return OK;
}

static CommandError servo(const String& argument) {
  // Do a little silly string hacking dance to extract both arguments
  auto spaceIndex = argument.indexOf(' ');
  if (spaceIndex == -1) {
    return COMMAND_ERROR("servo takes two arguments");
  }
  auto firstArgument = argument.substring(0, spaceIndex);
  auto secondArgument = argument.substring(spaceIndex + 1);
  auto width = secondArgument.toInt();
  auto servo = firstArgument.toInt();
  if (servo < 0 || servo > 15) {
    return COMMAND_ERROR("servo index out of range");
  }
  if (width != 0 && (width < 150 || width > 550)) {
    return COMMAND_ERROR("width must be 0 or between 150 and 550");
  }
  SERVOS.setPWM(servo, 0, width);
  return OK;
}

static CommandError get_version(const String& argument) {
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
};

static void handle_command(const String& cmd) {
  for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
    const CommandHandler& handler = commands[i];

    if (handler.command == cmd) {
      // no arguments
      auto err = handler.run("");
      if (err == OK) {
        Serial.write("+ OK\n");
      } else {
        Serial.write("- Error: ");
        Serial.write(err.c_str());
        Serial.write('\n');
      }
      return;
    } else if (cmd.startsWith(handler.command + " ")) {
      // has arguments
      auto err = handler.run(cmd.substring(handler.command.length() + 1));
      if (err == OK) {
        Serial.write("+ OK\n");
      } else {
        Serial.write("- Error: ");
        Serial.write(err.c_str());
        Serial.write('\n');
      }
      return;
    }
  }

  Serial.write("- Error: unknown command\n");
}

static CommandError run_help(const String& argument) {
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
  process_serial();
}
