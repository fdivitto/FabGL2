# DirectVGA

Minimal example of **direct scanline rendering** with `fabgl::VGADirectController`.

There is no frame buffer: the application provides a callback that draws each scanline just before it is sent to
the monitor. The demo shows a red square bouncing over a blue background at 640x480, with flashing random-color
borders at the top and bottom of the screen.

The demo runs on its own: there are no controls.

## What it demonstrates

- `fabgl::VGADirectController` with `setDrawScanlineCallback()` and `setScanlinesPerCallBack()`
- Writing raw pixels (`createRawPixel()`, `VGA_PIXELINROW()`) from an `IRAM_ATTR` callback that runs in interrupt
  context
- Synchronizing the main task with the end of frame (task notification from the callback)
- 640x480 at 64 colors using almost no RAM

Use this approach when the screen content can be computed line by line (games, emulators, demos) and a full
frame buffer would not fit in memory. See also [DirectVGA_ElMuro](../DirectVGA_ElMuro).

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
