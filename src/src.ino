String serial_line = "";
String current_arg = "";

void setup() {
	Serial.begin(115200);
}

void loop(){
	if (Serial.available() > 0) {
		// Read characters one at a time into a buffer
		// process the string as a command when we receive a newline
		char c = Serial.read();
		if (c == '\n') {
			processCommand();
			cleanUp();
		}
		else {
			serial_line += c;
		}
	}
}

// All comands have multiple segments seperated by colons
// current_arg becomes the next input segment
// serial_line is modifed to remove the start segment
void getSlice() {
	int pos = serial_line.indexOf(":");
	if (pos == -1) {
		current_arg = serial_line;
		serial_line = "";
	}
	else {
		// Current arg is the bit upto the colon;
		current_arg = serial_line.substring(0, pos);
		// serial line becomes the remaining bit;
		serial_line = serial_line.substring(pos + 1);
	}
}

// Used to cleanup the input buffer after a command has been processed
void cleanUp() {
	serial_line = "";
}

// Take the input buffer and process the command
void processCommand() {
	getSlice();

	if (current_arg.equals("*IDN?")) {
		// CMD: *IDN?
		Serial.print("SourceBots:Arduino:X:2.0\n");
		return;
	}

	else if (current_arg.equals("*STATUS?")) {
		// CMD: *STATUS?
		Serial.print("Yes\n");
		return;
	}

	else if (current_arg.equals("*RESET?")) {
		// CMD: *RESET?
		Serial.print("NACK:Reset not supported\n");
		return;
	}

	else if (current_arg.equals("PIN")) {
		getSlice();
		int pin = current_arg.toInt();
		getSlice();

		if (current_arg.equals("MODE")) {
			getSlice();
			if (current_arg.equals("GET?")) {
				// CMD: PIN:<n>:MODE:GET?
				int mode = getPinMode(pin);
				if (mode == INPUT) {
					Serial.print("INPUT\n");
					return;
				}
				else if (mode == INPUT_PULLUP) {
					Serial.print("INPUT_PULLUP\n");
					return;
				}
				else if (mode == OUTPUT) {
					Serial.print("OUTPUT\n");
					return;
				}
			}
			else if (current_arg.equals("SET")) {
				getSlice();
				if (current_arg.equals("INPUT")) {
					// CMD: PIN:<n>:MODE:SET:INPUT
					pinMode(pin, INPUT);
					Serial.print("ACK\n");
					return;
				}
				else if (current_arg.equals("INPUT_PULLUP")) {
					// CMD: PIN:<n>:MODE:SET:INPUT_PULLUP
					pinMode(pin, INPUT_PULLUP);
					Serial.print("ACK\n");
					return;
				}
				else if (current_arg.equals("OUTPUT")) {
					// CMD: PIN:<n>:MODE:SET:OUTPUT
					pinMode(pin, OUTPUT);
					Serial.print("ACK\n");
					return;
				}
			}
		}

		else if (current_arg.equals("DIGITAL")) {
			getSlice();
			if (current_arg.equals("GET?")) {
				// CMD: PIN:<n>:DIGITAL:GET?
				if (digitalRead(pin)) {
					Serial.print("1\n");
					return;
				}
				else {
					Serial.print("0\n");
					return;
				}
			}
			else if (current_arg.equals("SET")) {
				getSlice();
				if (current_arg.equals("1")) {
					// CMD: PIN:<n>:DIGITAL:SET:1
					digitalWrite(pin, HIGH);
					Serial.print("ACK\n");
					return;
				}
				else {
					// CMD: PIN:<n>:DIGITAL:SET:0
					digitalWrite(pin, LOW);
					Serial.print("ACK\n");
					return;
				}
			}
		}

		else if (current_arg.equals("ANALOG")) {
			getSlice();
			if (current_arg.equals("GET?")) {
				// CMD: PIN:<n>:ANALOG:GET?
				Serial.print(analogRead(pin));
				Serial.print("\n");
				return;
			}
		}
	}

	else if (current_arg.equals("ULTRASOUND")) {
		getSlice();
		int pulse = current_arg.toInt();
		getSlice();
		int echo = current_arg.toInt();
		getSlice();
		if (current_arg.equals("MEASURE?")) {
			// CMD: ULTRASOUND:<pulse_pin>:<echo_pin>:MEASURE?

			// config pins to correct modes
			pinMode(pulse, OUTPUT);
			pinMode(echo, INPUT);

			// provide pulse to trigger reading
			digitalWrite(pulse, LOW);
			delayMicroseconds(2);
			digitalWrite(pulse, HIGH);
			delayMicroseconds(5);
			digitalWrite(pulse, LOW);

			// measure the echo time on the echo pin
			int duration = pulseIn(echo, HIGH, 60000);
			Serial.print(microsecondsToMm(duration));
			Serial.print("\n");
			return;
		}
	}

	// If we dont hit a return statement in the if/else tree
	// we return a NACK due to it being an invalid command
	Serial.print("NACK:Invalid command\n");
}

long microsecondsToMm(long microseconds) {
	// The speed of sound is 340 m/s or 29 microseconds per centimeter.
	// The ping travels out and back, so to find the distance we need half
	// 10 x (us / 29 / 2)
	return (5 * microseconds / 29);
}

int getPinMode(uint8_t pin) {
	// arduino functions to map a pin to a port/bit
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);

	// check if the direction is input or output
	volatile uint8_t *reg = portModeRegister(port);
	if (*reg & bit) {
		return OUTPUT;
	}

	// If pin mode is input check if pullup is enabled
	volatile uint8_t *out = portOutputRegister(port);
	return ((*out & bit) ? INPUT_PULLUP : INPUT);
}
