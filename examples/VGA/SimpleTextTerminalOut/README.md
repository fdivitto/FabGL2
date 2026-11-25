# SimpleTextTerminalOut

Same as [SimpleTerminalOut](../SimpleTerminalOut), but using **`fabgl::VGATextController`**, the text-only VGA
mode (80x25 characters, 16 colors) that needs only a few KB of RAM.

The demo cycles through: welcome screen (screen and terminal size, free memory), the 16 ANSI foreground and
background colors, the supported styles (bold, underlined, blinking, inverse) and a fast VT/ANSI animation.
There are no controls.

## What it demonstrates

- `fabgl::Terminal` on top of `fabgl::VGATextController`
- What the text-only mode can and cannot do compared to a graphical controller: no italic, no double width or
  double height lines, no graphics, but a much smaller memory footprint (useful when the application needs RAM
  for other purposes, e.g. WiFi)

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
