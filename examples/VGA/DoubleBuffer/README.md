# DoubleBuffer

Benchmark of the drawing primitives with **double buffering** (flicker-free full screen animation) at 320x200,
64 colors.

Three tests run one after the other in an endless loop. Each test adds a new object every 5 frames until its
maximum is reached, then moves to the next test. The yellow label at the top shows the number of objects, the
test name and the measured frames per second:

| Test | Content |
|---|---|
| **Balls** | Up to 300 filled circles bouncing around |
| **Polygons** | Rotating filled polygons with random shape and color |
| **Sprites** | Moving sprites |

The demo runs on its own: there are no controls.

## What it demonstrates

- `VGAController::setResolution(..., doubleBuffered = true)` and `Canvas::swapBuffers()`
- Full-screen redraw at every frame with `Canvas::clear()`, `fillEllipse()`, `fillPath()` and sprites
- Measuring the drawing throughput (FPS) as the scene gets heavier

Set `DOUBLEBUFFERING` to `0` in `DoubleBuffer.cpp` to see the same scene without double buffering
(the flickering becomes visible).

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
