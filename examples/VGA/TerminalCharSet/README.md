# TerminalCharSet

Prints the whole **character set** of the terminal font (codes 32 to 255) with its hexadecimal and decimal code,
one character at a time, then starts again after 10 seconds.

Useful to see which glyph corresponds to each code (box drawing characters, accented letters, symbols) when
writing programs that use the terminal.

The demo runs on its own: there are no controls.

## What it demonstrates

- `fabgl::Terminal` on a `fabgl::VGAController` (640x350)
- ANSI escape sequences to set colors, clear the screen and position the cursor
- Changing the colors with `Terminal::setForegroundColor()`

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
