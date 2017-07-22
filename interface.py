import serial

class Arduino(object):
    def __init__(self, path):
        self.port = serial.Serial(path, 9600)
        (self.fw_version,) = self._command('version')

    def set_led(self, state):
        self._command('led', 'on' if state else 'off')

    def drive_servo(self, servo, width):
        self._command('servo', servo, width)

    def depower_servo(self, servo):
        self.drive_servo(servo, 0)

    def digital_read(self, pin):
        (output,) = self._command('gpio-read', pin)
        return {
            'high': True,
            'low': False,
        }[output]

    def digital_write(self, pin, level):
        self._command('gpio-write', pin, 'high' if level else 'low')

    def set_input(self, pin):
        self._command('gpio-write', pin, 'hi-z')

    def set_input_pullup(self, pin):
        self._command('gpio-write', pin, 'pullup')

    def analogue_read(self):
        settings = [
            x.split(' ')
            for x in self._command('analogue-read')
        ]

        return {
            name: float(value)
            for name, value in settings
        }

    def _command(self, command, *args):
        full_command = ' '.join([command] + [str(x) for x in args])
        self.port.write((full_command + '\n').encode('utf-8'))
        self.port.flush()

        results = []

        for line in self.port:
            line = line.decode('utf-8').strip()
            if not line:
                continue

            line_class = line[0]

            if line_class == '#':
                # Comment
                continue
            elif line_class == '>':
                # Significant result
                results.append(line[1:].strip())
            elif line_class == '+':
                # OK
                return results
            elif line_class == '-':
                raise RuntimeError(line[1:].strip())
            else:
                raise RuntimeError("Unknown line class: {0!r}".format(line_class))


if __name__ == '__main__':
    import sys
    argument = sys.argv[1]
    port = Arduino(argument)
    print(port.fw_version)

    from IPython import embed
    embed()
