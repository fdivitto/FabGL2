Bitmaps, sprites and games {#guide_sprites}
==========================

[TOC]

# Bitmaps {#sprites_bitmaps}

A fabgl::Bitmap describes an image stored in memory. Three pixel formats are supported:

| PixelFormat | Bytes per pixel | Notes |
|---|---|---|
| PixelFormat::Mask | 1 bit | 0 = transparent, 1 = foreground color given in the constructor |
| PixelFormat::RGBA2222 | 1 | 2 bits per channel plus 2 bits of alpha (0 = transparent, 3 = opaque) |
| PixelFormat::RGBA8888 | 4 | 8 bits per channel plus alpha |

```cpp
const uint8_t heart_data[] = { 0x66, 0xff, 0xff, 0x7e, 0x3c, 0x18 };       // 8x6, 1 bit per pixel
Bitmap heart(8, 6, heart_data, PixelFormat::Mask, RGB888(255, 0, 0));

cv.drawBitmap(100, 50, &heart);
```

The `tools/img2bitmap` converter generates the C arrays from image files.
Bitmap data is not copied unless the `copy` constructor parameter is true, so the array must stay valid while
the bitmap is in use.


# Sprites {#sprites_sprites}

A fabgl::Sprite is an image that moves over the background without altering it: the display controller saves
the background under the sprite and restores it when the sprite moves. A sprite holds one or more bitmaps
(animation frames) and shows one of them at a time.

```cpp
Sprite sprites[2];

sprites[0].addBitmap(&heart);
sprites[0].moveTo(10, 10);
sprites[0].visible = true;

sprites[1].addBitmap(&frame1)->addBitmap(&frame2);  // two animation frames
sprites[1].moveTo(50, 10);
sprites[1].visible = true;

DisplayController.setSprites(sprites, 2);

while (true) {
  sprites[0].moveBy(1, 0);
  sprites[1].nextFrame();
  DisplayController.refreshSprites();               // apply the changes
  vTaskDelay(pdMS_TO_TICKS(20));
}
```

Rules:
- After changing the position, frame or visibility of a sprite, call refreshSprites().
- The sprite array passed to setSprites() must remain valid. To add application data to a sprite, derive from
  Sprite and pass an array of the derived type (setSprites() is a template that handles the element size).
- There is no limit on the number of sprites, but many or large sprites slow down the frame rate.
- With double buffering, sprites are drawn on the back buffer: call refreshSprites() before swapBuffers().

The mouse pointer is a special sprite, set with fabgl::BitmappedDisplayController::setMouseCursor().


# Scenes {#sprites_scene}

fabgl::Scene is a base class for games. It runs a task that calls your update() method at a fixed rate and
includes a fabgl::CollisionDetector. Derive from it and implement three methods:

```cpp
struct MyScene : public Scene {

  Sprite sprites[2];

  MyScene() : Scene(2, 20, DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight()) { }

  void init() override {                      // called once, when the scene starts
    sprites[0].addBitmap(&player)->moveTo(10, 100);
    sprites[1].addBitmap(&enemy)->moveTo(200, 100);
    for (auto & s : sprites) {
      s.visible = true;
      addSprite(&s);                          // add to the collision detector
    }
    DisplayController.setSprites(sprites, 2);
  }

  void update(int updateCount) override {     // called every 20 ms
    sprites[0].moveBy(1, 0);
    updateSpriteAndDetectCollisions(&sprites[0]);
    DisplayController.refreshSprites();
  }

  void collisionDetected(Sprite * a, Sprite * b, Point collisionPoint) override {
    b->visible = false;
  }
};

MyScene scene;
scene.start();      // suspends the calling task until stop() is called
```

The constructor parameters are the maximum number of sprites, the update period in milliseconds and the scene
size. Collisions are checked at pixel level, using the actual shape of the bitmaps. See the
`VGA/CollisionDetection` and `VGA/SpaceInvaders` examples.
