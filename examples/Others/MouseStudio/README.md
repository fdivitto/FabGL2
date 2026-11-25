# MouseStudio

Diagnostic tool for a **PS/2 mouse**. It has no video output: everything is printed on the serial console.

Every movement packet received from the mouse is printed as X/Y/wheel deltas and button states, and the mouse
parameters (sample rate, resolution, scaling) can be changed on the fly to see their effect.

## What it demonstrates

- `fabgl::PS2Controller` default configuration (keyboard on port 0, mouse on port 1)
- `fabgl::Mouse` low-level API: `deltaAvailable()`, `getNextDelta()`, `identify()`, `reset()`,
  `setSampleRate()`, `setResolution()`, `setScaling()`
- Detection of wheel and 5-button mice

## Hardware

- PS/2 mouse on port 1 (CLK = GPIO26, DATA = GPIO27)
- USB serial connection (115200 baud)

## Usage

Open a serial monitor on the board port (for example `idf.py -p PORT monitor`). Move the mouse and press its
buttons:

```
deltaX = 3  deltaY = -1  deltaZ = 0  leftBtn = 0  midBtn = 0  rightBtn = 0
```

Commands (single characters typed in the serial monitor):

| Key | Command |
|---|---|
| `h` | Print the help, chip info and device identification |
| `r` | Reset the mouse |
| `s` | Next sample rate: 10, 20, 40, 60, 80, 100, 200 samples/s (cyclic, default 100) |
| `e` | Next resolution: 1, 2, 4, 8 counts/mm (cyclic, default 4) |
| `c` | Toggle scaling 1:1 / 1:2 |

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
