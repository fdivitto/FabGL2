# DirectVGA_ElMuro

**"El Muro"**: a small brick-breaker game rendered entirely with direct scanline drawing
(`fabgl::VGADirectController`, 320x200, no frame buffer), with sound effects.

Contributed by Carles Oriol (author of [ClassicRacer](../ClassicRacer)).

## What it demonstrates

- `fabgl::VGADirectController`: the whole game screen (bricks, ball, paddle, borders, lives) is generated line by
  line inside an `IRAM_ATTR` callback
- Game logic synchronized to the vertical retrace through a task notification
- `fabgl::SoundGenerator` with a `SquareWaveformGenerator` for simple sound effects
- Reusable game controller classes (`controllers.h`) for keyboard, mouse and GPIO joysticks

## Hardware

- VGA output (default pins)
- Audio output on GPIO25 (optional)
- PS/2 keyboard on port 0 and/or PS/2 mouse on port 1

## How to play

Break all the 96 bricks by bouncing the ball with the paddle. You have 3 spare paddles (shown at the top right);
when you lose them all, or when all the bricks are destroyed, the game restarts.
The bounce angle depends on where the ball hits the paddle.

| Input | Action |
|---|---|
| Mouse movement | Move the paddle |
| `Left` / `Right` arrows | Move the paddle |
| Left mouse button or right `Shift` | Launch the ball (it also launches by itself after 4 seconds) |

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
