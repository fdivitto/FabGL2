# Sprites

The simplest **sprite** example: three sprites move along sine waves over a grid background at 320x200.

Each sprite uses a different bitmap pixel format:

| Sprite | Format |
|---|---|
| 1 | `PixelFormat::Mask` (1 bit per pixel, single color) |
| 2 | `PixelFormat::RGBA2222` (64 colors + 2 alpha bits) |
| 3 | `PixelFormat::RGBA8888` (true color + alpha, converted to the display format) |

The demo runs on its own and speeds up over time: there are no controls.

## What it demonstrates

- Creating `fabgl::Bitmap` objects from arrays in the supported pixel formats
- `fabgl::Sprite`, `VGAController::setSprites()` and `refreshSprites()`
- Sprites move without damaging the background (the controller saves and restores it)

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
