# LoopbackTerminal

A **VT/ANSI terminal connected to itself**: whatever you type on the PS/2 keyboard is sent back to the terminal,
so you can try the terminal escape sequences without any host computer.

At startup the screen (VGA text mode, 80x25) shows the screen and terminal size, the keyboard status and the
free memory.

## What it demonstrates

- `fabgl::Terminal` in local mode (`Terminal.connectLocally()`, then `available()` / `read()` / `write()`)
- `fabgl::VGATextController`: the text-only VGA mode with the smallest memory footprint
- Handling of DEL (backspace + erase to end of line) and CR (CR + LF) in the loopback

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 (CLK = GPIO33, DATA = GPIO32)

## Usage

Just type. To send an escape sequence, press `ESC` and then type the rest of the sequence. Examples:

| Type | Effect |
|---|---|
| `ESC` `[2J` | Clear the screen |
| `ESC` `[1;1H` | Move the cursor to the top left corner |
| `ESC` `[31m` | Red foreground (`30`...`37` normal colors, `90`...`97` bright colors) |
| `ESC` `[44m` | Blue background |
| `ESC` `[0m` | Reset the attributes |
| `ESC` `[5m` | Blinking text |

The full list of supported sequences is in the [Terminal documentation](https://www.fabglib.org).

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
