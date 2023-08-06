# Arduino Firmware

The firmware for SourceBots' Arduino Uno board.

It communicates using commands sent over USB serial.

Commands consist of a string terminated by a newline character (\n).
Commands consist of multiple parts seperated by a colon character.

## Commands

| Command                           | Description                  | Parameters       |
|-----------------------------------|------------------------------|------------------|
| PIN:\<n>:MODE:GET?                | Read pin mode                | n = pin number   |
| PIN:\<n>:MODE:SET:\<value>        | Set pin mode                 | n = pin number, value=INPUT/INPUT_PULLUP/OUTPUT|
| PIN:\<n>:DIGITAL:GET?             | Digital read pin             | n = pin number   |
| PIN:\<n>:DIGITAL:SET:\<value>     | Digital write pin            | n = pin number, value = 1/0 |
| PIN:\<n>:ANALOG:GET?              | Analog read pin              | n = pin number   |
| ULTRASOUND:\<pulse>:\<echo>:MEASURE?| Measure range from ultrasound| pulse = pulse pin, echo = echo pin |

## Example Commands

Set pin mode to input and analog read pin
```
PIN:14:MODE:SET:INPUT
PIN:14:ANALOG:GET?
```

Set pin to output and set it high
```
PIN:2:MODE:SET:OUTPUT
PIN:2:DIGITAL:SET:1
```
