# CollisionDetection

Animated space scene that shows the **sprite collision detection** of FabGL.

Ten spaceships and four asteroids fly across a starfield at 320x200 (64 colors) while Jupiter slowly crosses the
background. When a spaceship hits another spaceship or an asteroid, the spaceship plays an explosion animation
and the asteroid disappears; after the explosion both objects come back from a random screen side.
Asteroids do not collide with each other.

The demo runs on its own: there are no controls.

## What it demonstrates

- `fabgl::Scene`: a game loop running at a fixed rate (`init()`, `update()`, `collisionDetected()`)
- Pixel-precise collision detection between sprites (`Scene::addSprite()`,
  `updateSpriteAndDetectCollisions()`)
- Sprites with several frames (spaceship + 8-frame explosion), `Sprite::nextFrame()`
- Sprites drawn over a static background painted with `fabgl::Canvas`
- `fabgl::VGAController` (64 colors) at 320x200

## Hardware

- VGA output (default pins)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
