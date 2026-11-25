# MouseOnScreen

Shows how to display a **mouse pointer** on a VGA screen and use it to draw.

The screen (640x350, 64 colors) shows the current pointer coordinates in a small yellow box.

## What it demonstrates

- `Mouse::setupAbsolutePositioner()`: converts the relative PS/2 mouse movements into absolute screen
  coordinates and moves the hardware-like mouse cursor drawn by the display controller
- `VGAController::setMouseCursor()` and the predefined `CursorName` shapes
- Blocking read of the mouse status (`Mouse::getNextStatus()`), buttons and wheel

## Hardware

- VGA output (default pins)
- PS/2 mouse on port 1 (CLK = GPIO26, DATA = GPIO27); a wheel mouse is optional

## Controls

| Mouse action | Effect |
|---|---|
| Left button (hold and move) | Draws red pixels at the pointer position |
| Right button | Cycles through the available pointer shapes |
| Middle button | Clears the screen |
| Wheel | Moves the coordinates box up and down |

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
