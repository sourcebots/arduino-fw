# Servo Firmware

The firmware for SourceBots' servo board.

It communicates using commands sent over the USB serial pins.

Commands consist of a single character, followed by up to two arguments. The command is not separated from the first argument, but the first and second argument are separated by a space.

Request IDs can be specified by prefacing your command with `@int`, for example `@8335`. This is then returned with the command response and can be used to prevent race conditions.

## Commands

| Command | Description                 | Parameter 1  | Parameter 2            |
|---------|-----------------------------|--------------|------------------------|
| A       | Read the analogue pins      |              |                        |
| L       | Control the debug LED       | State {H, L} |                        |
| R       | Read a digital pin          | Pin Number   |                        |
| S       | Control a servo             | Servo Number | Width                  |
| T       | Read a raw ultrasound timing | Trigger Pin  | Echo Pin               |
| U       | Read an ultrasound distance | Trigger Pin  | Echo Pin               |
| V       | Get the firmware version    |              |                        |
| W       | Write to a Pin              | Pin Number   | Pin State {H, L, P, Z} |

## Example Commands

Read the analogue pins: `A`

Turn on the debug LED: `LH`

Read the ultrasound distance: `U4 5`

Set pin 6 High: `W6 H`
