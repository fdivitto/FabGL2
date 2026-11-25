# AnsiTerminal

A complete **serial VT/ANSI terminal**: connect the board to a retro computer, a Z80/CP/M board (e.g. MBC2,
RC2014), a Raspberry Pi, a Linux console or any device with a serial port, and use it with a VGA monitor and a
PS/2 keyboard and mouse. All the settings are configured on screen and saved in NVS.

At boot the terminal shows version, screen and terminal size, keyboard layout, terminal type and serial
parameters (this can be disabled with *Show Info*).

## Default settings

| Setting | Default |
|---|---|
| Serial port | TX = GPIO2, RX = GPIO34 (*FabGL Terminal* port), RTS = GPIO13, CTS = GPIO35 |
| Serial parameters | 115200 baud, 8N1, no flow control |
| Terminal type | ANSI Legacy |
| Keyboard layout | US |
| Resolution | 640x480 60 Hz, 16 colors, font chosen automatically for 80x25 |
| Colors | bright green on black |

## Keys

| Key | Action |
|---|---|
| `F12` | Opens the **Terminal Configuration** dialog |
| `Ctrl` + `Alt` + `F12` | Resets all settings to the defaults and reboots |
| `Pause/Break` | Sends a serial BREAK (233 ms) |
| `Shift` + `Pause/Break` | Sends a long serial BREAK (3.5 s) |

## Terminal Configuration dialog (`F12`)

Use `Tab` / `Shift+Tab` to move between fields, or the mouse.

| Field | Values |
|---|---|
| Terminal Type | ANSI, ADM 3A, ADM 31, Hazeltine 1500, Osborne, Kaypro, VT52, ANSI Legacy |
| Keyboard Layout | DE, IT, UK, US, ES, FR, BE, NO, JP |
| Background / Foreground Color | Any of the 16 terminal colors |
| Baud Rate | 110 ... 2000000 |
| Data Length / Parity / Stop Bits | 5-8 bits / None, Even, Odd / 1, 1.5, 2, 3 |
| Flow Control | None, XON/XOFF, RTS/CTS, Combi (both) |
| Resolution | 1280x768 B&W, 1024x720 4 colors, 800x600 8 colors, 720x520 16 colors, 640x480 73 Hz or 60 Hz 16 colors, 640x350 16 colors, 512x384 64 colors, 400x300 64 colors |
| Font | Auto, or one of the 31 included fonts (from 4x6 to 10x20, plus decorative fonts) |
| Columns / Rows | Max (as many as fit), 80 or 132 / Max, 24 or 25 |
| Port | *FabGL Terminal* (TX=2, RX=34), *USB* (TX=1, RX=3: the USB-serial chip, so the terminal can be driven by a PC), *PS/2 Mouse* (TX=27, RX=26: uses the mouse port, the mouse is disabled) |
| Show Info | Shows the information screen at boot |
| RTS / CTS | RTS can be toggled by clicking it; CTS shows the input line state |

Buttons:

| Button | Action |
|---|---|
| **Quit [ESC]** | Closes the dialog without saving |
| **Save & Quit [F10]** | Saves the settings. Changes of resolution, font, columns, rows, background color or port reboot the terminal |
| **Reboot [CTRL+ESC]** | Reboots the board |
| **Install Programs** | Sends a sample BASIC program to the host (see below) |

## Install Programs

When the host runs CP/M, *Install Programs* types `PIP <NAME>.BAS=CON:` on the host and then sends the program
text, so the file is saved on the host disk. Then run it with MBASIC (`MBASIC <NAME>`). The programs use the
FabGL-specific escape sequences to control GPIOs, read the ADC, play sounds, read the mouse and draw graphics and
sprites:

| Program | Description |
|---|---|
| BLINK.BAS | Sets GPIO12 as output and turns it on and off |
| GPIOREAD.BAS | Sets GPIO36 as digital input and continuously prints its value |
| ADC.BAS | Sets GPIO36 as analog input and draws its value |
| ADCVOLTS.BAS | Sets GPIO36 as analog input and prints its value in Volts |
| SOUND.BAS | Generates some sounds |
| MOUSE.BAS | Shows mouse position and buttons |
| GRAPH.BAS | Draws some graphics |
| SPRITE.BAS | Creates and moves sprites |

The same programs are in [Extras/Terminal BASIC Programs](../../../Extras/Terminal%20BASIC%20Programs).
The full list of FabGL sequences is in
[FabGL Specific Terminal Sequences](https://www.fabglib.org/special_term_escapes.html).

## Changing resolution from the host

The host can switch resolution by sending the user sequence `ESC _ # <mode> $`, where `<mode>` is one of
`1280x768x2`, `1024x720x4`, `800x600x8`, `720x520x16`, `640x480@73x16`, `640x480@60x16`, `640x350x16`,
`512x384x64`, `400x300x64`. The terminal reboots in the new mode for one session only (the saved resolution is
not changed). Example from MBASIC: `PRINT CHR$(27);"_#800x600x8$";`

## What it demonstrates

- `fabgl::Terminal` connected to a UART through `fabgl::SerialPort` and `fabgl::SerialPortTerminalConnector`
  (hardware and software flow control, BREAK)
- Terminal emulations, fonts, colors and keyboard layouts selectable at run time
- Choosing the display controller (2, 4, 8, 16 or 64 colors) at run time according to the resolution
- A configuration GUI (`uiApp`) shown on top of the terminal, `fabgl::Preferences` (NVS)
- Terminal user sequences (`Terminal.onUserSequence`)

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 and PS/2 mouse on port 1 (optional)
- Serial connection to the host (3.3 V logic levels; use a level shifter for RS-232 or 5 V devices)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
