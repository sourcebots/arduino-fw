#include <limits.h>

#include <Adafruit_PWMServoDriver.h>

// Multiplying by this converts round-trip duration in microseconds to distance to object in millimetres.
static const float ULTRASOUND_COEFFICIENT = 1e-6 * 343.0 * 0.5 * 1e3;

static const String FIRMWARE_VERSION = "SourceBots PWM/GPIO v2.0.0";

typedef String CommandError;

static const CommandError OK = "";

#define COMMAND_ERROR(x) ((x))

static Adafruit_PWMServoDriver SERVOS = Adafruit_PWMServoDriver();

class CommandHandler {
  public:
    String command;
    CommandError (*run)(int, String argument);

    CommandHandler(String cmd, CommandError(*runner)(int, String));
};

CommandHandler::CommandHandler(String cmd, CommandError (*runner)(int, String))
: command(cmd), run(runner)
{
  
}

static const CommandHandler commands[] = {
  
};

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

static void dispatch_command(int commandId, const class CommandHandler& handler, const String& argument) {
  auto err = handler.run(commandId, argument);
  if (err == OK) {
    serialWrite(commandId, '+', "OK");
  } else {
    serialWrite(commandId, '-', String("Error: ") + err);
  }
}

static void handle_actual_command(int commandId, const String& cmd) {
    for (int i = 0; i < sizeof(commands) / sizeof(CommandHandler); ++i) {
      const CommandHandler& handler = commands[i];

      if (handler.command == cmd) {
        dispatch_command(commandId, handler, "");
        return;
      } else if (cmd.startsWith(handler.command + " ")) {
        dispatch_command(
          commandId,
          handler,
          cmd.substring(handler.command.length() + 1)
        );
        return;
      }
    }

    serialWrite(commandId, '-', String("Error, unknown command: ") + cmd);
}

static void handle_command(const String& cmd) {
  // A commans is prefixed with @ if an identifier is used to prevent race conditions
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
  for (int pin = 2; pin <= 12; pin++) {
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

