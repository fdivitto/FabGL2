# KeyboardScanCodes

Diagnostic tool that shows the **raw scancodes** sent by a PS/2 keyboard.

Every byte received from the keyboard is printed in hexadecimal, one key event per line (prefix bytes `E0` and
`F0` are kept on the same line as the code they belong to). The output goes both to the VGA screen (80x25 text
mode) and to the serial console.

## What it demonstrates

- `fabgl::VGATextController` (text-only VGA mode, very small memory footprint) with a `fabgl::Terminal`
- `fabgl::PS2Controller` with `KbdMode::NoVirtualKeys`: the keyboard driver delivers scancodes without translating them
- Keyboard low-level commands: identification, reset, scancode set selection and LED control
- `fabgl::UARTStream` to read commands from the serial console

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 (CLK = GPIO33, DATA = GPIO32)
- USB serial connection (115200 baud) to send commands

## Usage

Open a serial monitor on the board port (for example `idf.py -p PORT monitor`) and type one of these
single-character commands (no Enter needed):

| Key | Command |
|---|---|
| `h` | Print the help, chip info and keyboard identification |
| `q` | Select scancode set 1 |
| `w` | Select scancode set 2 |
| `l` | Test the keyboard LEDs (cycles through all 8 combinations, one per second, then turns them off) |
| `r` | Reset the keyboard and print its identification |

Then press keys on the PS/2 keyboard: the scancodes appear on screen and on the serial monitor. For example
pressing and releasing `A` in set 2 prints `1C` and `F0 1C`.

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
