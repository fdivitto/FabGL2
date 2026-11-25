# SimpleTerminalOut

Self-running showcase of the **VT/ANSI terminal output** on a graphical display (640x480, 16 colors).
The application writes text and escape sequences to `fabgl::Terminal`, exactly as a remote host would do.

The demo cycles through these screens, forever:

1. **Welcome**: screen, viewport and terminal size, free memory
2. **ANSI colors**: the 16 foreground and background colors (`ESC[30m`...`ESC[37m`, `ESC[90m`...`ESC[97m`, ...)
3. **Styles**: bold, italic, underlined, blinking, inverse and their combinations, double width and double height
   lines (`ESC#6`, `ESC#3`, `ESC#4`)
4. **Mixed text and graphics**: random points, lines, rectangles and ellipses drawn with `fabgl::Canvas` on the
   same screen, then the terminal scrolls over them
5. **Fast rendering**: a VT/ANSI text animation
6. **Colored attributes**: `Terminal::setColorForAttribute()` maps bold/italic/underline to colors, with and
   without the style

There are no controls.

## What it demonstrates

- `fabgl::Terminal` on top of `fabgl::VGA16Controller`
- The ANSI/VT escape sequences supported by the terminal
- Mixing terminal text and `Canvas` graphics on the same display

See [SimpleTextTerminalOut](../SimpleTextTerminalOut) for the same demo using the text-only VGA mode.

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
