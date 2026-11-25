# SpaceInvaders

A complete **arcade shoot-'em-up game** (320x200, 64 colors) with sprites, collision detection and sampled
sounds. Watch it on [YouTube](https://www.youtube.com/watch?v=LL8J7tjxeXA).

## How to play

Destroy the waves of aliens before they reach the ground. Hide behind the four shields (they are damaged by both
your shots and the aliens' bombs). A mystery ship crosses the top of the screen from time to time.

| Target | Points |
|---|---|
| Mystery ship | ? (random bonus) |
| Top row aliens | 30 |
| Middle row aliens | 20 |
| Bottom row aliens | 10 |

You start with 3 lives. When a wave is destroyed the next level starts; the hi-score is kept until the board is
reset.

## Controls

On the intro screen press `Space` to play with the keyboard, or click to play with the mouse (only the available
device is proposed).

| Device | Move | Fire |
|---|---|---|
| Keyboard | `Left` / `Right` arrows | `Space` |
| Mouse | Move the mouse horizontally | Left button |

After *GAME OVER* press `Space` (or click) to return to the intro.

## What it demonstrates

- `fabgl::Scene` game loop, sprites with several frames, pixel-precise collision detection
- Drawing on the background bitmap (the shields are eroded pixel by pixel)
- `fabgl::SoundGenerator::playSamples()` for sampled sound effects
- Reading the keyboard state (`isVKDown()`) and the mouse (`getNextDelta()`, `updateAbsolutePosition()`) as game
  controllers

## Hardware

- VGA output (default pins)
- Audio output on GPIO25
- PS/2 keyboard on port 0 and/or PS/2 mouse on port 1

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
