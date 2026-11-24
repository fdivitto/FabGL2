Drawing with Canvas {#guide_canvas}
===================

[TOC]

# How drawing works {#canvas_queue}

fabgl::Canvas does not draw directly into the frame buffer. Each call (drawLine(), drawText(), ...) puts a
*primitive* into the display controller queue and returns immediately. The primitives are executed in the
background, normally during the vertical retrace, so the image never flickers.

Consequences:
- Drawing calls are fast, but the result appears a little later. Call fabgl::Canvas::waitCompletion() when you
  must be sure that everything has been drawn (for example before reading pixels with getPixel()).
- If the queue is full, the next drawing call waits until there is room. The queue size is set by
  fabgl::BitmappedDisplayController::queueSize, before calling `begin()`.
- The pen, brush, font and glyph options are part of the drawing state, which is shared by all Canvas objects
  that use the same display controller. Drawing from several tasks at the same time can therefore mix their
  settings: draw from a single task, or protect drawing with a mutex.

Canvas objects are lightweight: creating one on the stack, as in the examples, costs nothing.


# Coordinates and colors {#canvas_coords}

The origin (0, 0) is the top-left corner; `getWidth()` and `getHeight()` return the size of the drawable area.
setOrigin() translates all coordinates (sprites excluded) and setClippingRect() limits drawing to a rectangle.

There are two current colors:
- the **pen** color (setPenColor()), used for lines, outlines, pixels and text;
- the **brush** color (setBrushColor()), used for filled shapes, for clear() and for the text background.

```cpp
fabgl::Canvas cv(&DisplayController);
cv.setBrushColor(Color::Black);
cv.clear();                                    // fill the screen with the brush color

cv.setPenColor(RGB888(255, 128, 0));
cv.drawRectangle(10, 10, 100, 60);             // outline with the pen color
cv.setBrushColor(Color::Blue);
cv.fillEllipse(160, 100, 80, 50);              // filled with the brush color

cv.setPenWidth(3);
cv.setLineEnds(LineEnds::Circle);
cv.drawLine(0, 0, cv.getWidth() - 1, cv.getHeight() - 1);
```

Rectangles are specified by their top-left and bottom-right corners (both included); ellipses by their center
and size. drawPath() and fillPath() draw polygons from an array of fabgl::Point.


# Text and fonts {#canvas_text}

FabGL embeds many fixed and variable width fonts (`FONT_8x8`, `FONT_8x16`, `FONT_std_12`, ...). Select one with
selectFont(), or let FabGL choose with fabgl::getPresetFontInfo():

```cpp
cv.selectFont(&fabgl::FONT_8x16);
cv.setPenColor(Color::BrightWhite);
cv.drawText(10, 10, "Hello");
cv.drawTextFmt(10, 30, "Free memory: %d bytes", (int) heap_caps_get_free_size(MALLOC_CAP_8BIT));

// font that fits 80 columns and 25 rows on this screen
cv.selectFont(fabgl::getPresetFontInfo(cv.getWidth(), cv.getHeight(), 80, 25));
```

By default only the glyph foreground is painted. Glyph options add effects, including a filled background:

```cpp
cv.setGlyphOptions(GlyphOptions().FillBackground(true).Bold(true));
cv.drawText(10, 50, "Bold text on brush color");
cv.resetGlyphOptions();
```

textExtent() returns the width in pixels of a string, useful to center or right-align text.


# Scrolling and copying {#canvas_scroll}

setScrollingRegion() and scroll() move the content of a rectangular area; vertical scrolling is very fast
because only line pointers are moved. copyRect() copies an area to another position.


# Performance tips {#canvas_perf}

- Prefer few large primitives to many small ones (fillRectangle() instead of many setPixel() calls).
- beginUpdate() / endUpdate() suspend background drawing, so that a group of primitives appears at once. Keep
  the group small: while drawing is suspended a full queue blocks the caller (call waitCompletion(false) to
  flush it).
- For frame-by-frame animations use double buffering (see @ref display_doublebuffer) or sprites
  (see @ref guide_sprites), which do not require redrawing the background.
- fabgl::BitmappedDisplayController::enableBackgroundPrimitiveExecution(false) executes primitives immediately,
  which is faster but may flicker.
