# KeyboardVirtualKeys

Diagnostic tool that shows how PS/2 keyboard events are translated into FabGL **virtual keys**.

For each key press and release the example prints the virtual key name (`VK_a`, `VK_LSHIFT`, ...), the
resulting ASCII code and character, the direction (`DN` = pressed, `UP` = released) and the raw scancodes.
It is the quickest way to check a keyboard layout or to find out which `VirtualKey` a key generates.
The output goes both to the VGA screen (640x480, 16 colors) and to the serial console.

## What it demonstrates

- `fabgl::PS2Controller` with virtual key generation (`Keyboard::getNextVirtualKey()`, `VirtualKeyItem`)
- Run-time selection of the keyboard layout (`Keyboard::setLayout()`)
- Keyboard identification, reset, scancode set selection and LED control
- `fabgl::VGA16Controller` with a `fabgl::Terminal`

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 (CLK = GPIO33, DATA = GPIO32)
- USB serial connection (115200 baud) to send commands

## Usage

Open a serial monitor on the board port (for example `idf.py -p PORT monitor`) and type one of these
single-character commands:

| Key | Command |
|---|---|
| `h` | Print the help, chip info, keyboard identification and current layout |
| `1` ... `8` | Select the layout: `1` US, `2` UK, `3` German, `4` Italian, `5` Spanish, `6` French, `7` Belgian, `8` Norwegian |
| `r` | Reset the keyboard |
| `q` / `w` | Select scancode set 1 / 2 |
| `l` | Test the keyboard LEDs |

Then press keys on the PS/2 keyboard. Example of output line:

```
VK_a:   ASCII = 0x61   'a'   DN   [1C ]
```

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
