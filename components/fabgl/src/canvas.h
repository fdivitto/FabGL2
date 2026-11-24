/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2026 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */
 

#pragma once



/**
 * @file
 *
 * @brief This file contains the fabgl::Canvas definition.
 */


#include <stdint.h>
#include <stddef.h>

#include "displaycontroller.h"


namespace fabgl {



/**
* @brief Provides a set of drawing methods.
*
* Canvas interfaces directly with the display controller and provides
* primitives to draw lines, circles, etc., scroll regions, copy
* rectangles and draw glyphs.<br>
* By default the origin is at the top-left corner: coordinates range from (0, 0) to (Canvas Width - 1, Canvas Height - 1).
*
* Example:
*
*     // Set up pins and resolution (5 GPIOs, so up to 8 colors are available)
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
*     VGAController.setResolution(VGA_640x350_70Hz);
*
*     // Paint a green rectangle with a red border
*     fabgl::Canvas cv(&VGAController);
*     cv.setPenColor(Color::BrightRed);
*     cv.setBrushColor(Color::BrightGreen);
*     cv.fillRectangle(0, 0, cv.getWidth() - 1, cv.getHeight() - 1);
*     cv.drawRectangle(0, 0, cv.getWidth() - 1, cv.getHeight() - 1);
*
* @see @ref guide_canvas
*
* @ingroup graphics
*/
class Canvas {

public:

  Canvas(BitmappedDisplayController * displayController);

  /**
   * @brief Determines the canvas width in pixels.
   *
   * This is equivalent to the display controller viewport width.
   *
   * @return The canvas width in pixels.
   */
  int getWidth() { return m_displayController->getViewPortWidth(); }

  /**
   * @brief Determines the canvas height in pixels.
   *
   * This is equivalent to the display controller viewport height.
   *
   * @return The canvas height in pixels.
   */
  int getHeight() { return m_displayController->getViewPortHeight(); }

  /**
   * @brief Waits for the drawing queue to become empty.
   *
   * @param waitVSync If true, drawings are performed during the vertical retrace (slower); if false, they are performed immediately (faster, but may flicker).
   */
  void waitCompletion(bool waitVSync = true);

  /**
   * @brief Suspends drawings.
   *
   * After calling beginUpdate(), adding new primitives may cause a deadlock.<br>
   * To avoid this, call waitCompletion(false) frequently.
   */
  void beginUpdate();

  /**
   * @brief Resumes drawings after beginUpdate().
   */
  void endUpdate();

  /**
   * @brief Swaps the screen buffers when double buffering is enabled.
   *
   * Double buffering is enabled by calling the display controller setResolution() with doubleBuffered = true.<br>
   * Buffers are always swapped during the vertical retrace (at the VSync pulse).
   */
  void swapBuffers();

  /**
   * @brief Sets the axes origin.
   *
   * Once the origin is set, every coordinate is translated by the specified amount (sprites excluded).
   *
   * @param X Horizontal translation (0 = no translation).
   * @param Y Vertical translation (0 = no translation).
   */
  void setOrigin(int X, int Y);

  /**
   * @brief Sets the axes origin.
   *
   * Once the origin is set, every coordinate is translated by the specified amount (sprites excluded).
   *
   * @param origin Origin coordinates.
   */
  void setOrigin(Point const & origin);

  /**
   * @brief Gets the origin set with setOrigin().
   *
   * @return The current origin.
   */
  Point getOrigin() { return m_origin; }

  /**
   * @brief Sets the clipping rectangle, relative to the origin.
   *
   * The default clipping rectangle covers the entire canvas area.
   *
   * @param rect Clipping rectangle relative to the origin.
   */
  void setClippingRect(Rect const & rect);

  /**
   * @brief Gets the clipping rectangle set with setClippingRect().
   *
   * @return The current clipping rectangle.
   */
  Rect getClippingRect();


  /**
   * @brief Fills the entire canvas with the brush color.
   */
  void clear();

  /**
   * @brief Resets the paint state and other display controller settings.
   */
  void reset();

  /**
   * @brief Defines the scrolling region.
   *
   * The scrolling region is the rectangular area where scroll() operates.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordinate.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordinate.
   */
  void setScrollingRegion(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Scrolls pixels horizontally and/or vertically.
   *
   * Scrolling occurs inside the scrolling region defined by setScrollingRegion().<br>
   * Vertical scrolling moves line pointers, so it is very fast.<br>
   * Horizontal scrolling moves pixels one by one, so it is CPU intensive.
   *
   * @param offsetX Number of pixels to scroll left (offsetX < 0) or right (offsetX > 0).
   * @param offsetY Number of pixels to scroll up (offsetY < 0) or down (offsetY > 0).
   */
  void scroll(int offsetX, int offsetY);

  /**
   * @brief Moves the current pen position to the specified coordinates.
   *
   * Some methods, such as lineTo(), start drawing from the current pen position.
   *
   * @param X Horizontal pen position.
   * @param Y Vertical pen position.
   *
   * Example:
   *
   *     // Paint a triangle
   *     Canvas.moveTo(10, 12);
   *     Canvas.lineTo(30, 40);
   *     Canvas.lineTo(10, 12);
   */
  void moveTo(int X, int Y);

  /**
   * @brief Sets the pen (foreground) color from its color components.
   *
   * @param red Red component (0..255). The value is mapped to the nearest color available on the display.
   * @param green Green component (0..255).
   * @param blue Blue component (0..255).
   *
   * Example:
   *
   *     // Set white pen
   *     Canvas.setPenColor(255, 255, 255);
   */
  void setPenColor(uint8_t red, uint8_t green, uint8_t blue);

  /**
   * @brief Sets the pen (foreground) color using a color name.
   *
   * @param color Color name.
   *
   * Example:
   *
   *      // Set white pen
   *      Canvas.setPenColor(Color::BrightWhite);
   */
  void setPenColor(Color color);

  /**
   * @brief Sets the pen (foreground) color from an RGB888 value.
   *
   * @param color RGB888 color.
   *
   * Example:
   *
   *      // Set white pen
   *      Canvas.setPenColor(Color::BrightWhite);
   */
  void setPenColor(RGB888 const & color);

  /**
   * @brief Sets the brush (background) color from its color components.
   *
   * @param red Red component (0..255). The value is mapped to the nearest color available on the display.
   * @param green Green component (0..255).
   * @param blue Blue component (0..255).
   *
   * Example:
   *
   *     // Set a blue brush
   *     Canvas.setBrushColor(0, 0, 255);
   */
  void setBrushColor(uint8_t red, uint8_t green, uint8_t blue);

  /**
   * @brief Sets the brush (background) color using a color name.
   *
   * @param color The color name.
   *
   * Example:
   *
   *      // Set blue brush
   *      Canvas.setBrushColor(Color::BrightBlue);
   */
  void setBrushColor(Color color);

  /**
   * @brief Sets the brush (background) color from an RGB888 value.
   *
   * @param color RGB888 color.
   *
   * Example:
   *
   *      // Set blue brush
   *      Canvas.setBrushColor(Color::BrightBlue);
   */
  void setBrushColor(RGB888 const & color);

  /**
   * @brief Sets the pen width for lines, rectangles and paths
   *
   * @param value Pen width (minimum is 1).
   *
   * Example:
   * 
   *     Canvas.setPenWidth(4);
   *     Canvas.drawLine(10, 10, 100, 10);
   */
  void setPenWidth(int value);

  /**
   * @brief Sets the shape of line ends
   *
   * @param value Line end shape.
   *
   * Example:
   *
   *     Canvas.setPenWidth(4);
   *     Canvas.setLineEnds(LineEnds::Circle);
   *     Canvas.drawLine(10, 10, 100, 10);
   */
  void setLineEnds(LineEnds value);

  /**
   * @brief Sets a single pixel to the pen color.
   *
   * @param X Horizontal pixel position.
   * @param Y Vertical pixel position.
   */
  void setPixel(int X, int Y);

  /**
   * @brief Sets a single pixel to the specified color.
   *
   * @param X Horizontal pixel position.
   * @param Y Vertical pixel position.
   * @param color Pixel color.
   */
  void setPixel(int X, int Y, RGB888 const & color);

  /**
   * @brief Sets a single pixel to the specified color.
   *
   * @param pos Pixel position.
   * @param color Pixel color.
   */
  void setPixel(Point const & pos, RGB888 const & color);

  /**
   * @brief Draws a line starting from the current pen position.
   *
   * The starting position is set by moveTo() or is the end
   * position of the previous lineTo() call.<br>
   * The line is drawn with the current pen color.
   *
   * @param X Horizontal ending line position.
   * @param Y Vertical ending line position.
   *
   * Example:
   *
   *     // Paint a white triangle
   *     Canvas.setPenColor(Color::BrightWhite);
   *     Canvas.moveTo(10, 12);
   *     Canvas.lineTo(30, 40);
   *     Canvas.lineTo(10, 12);
   */
  void lineTo(int X, int Y);

  /**
   * @brief Draws a line between the specified start and end coordinates.
   *
   * The line is drawn with the current pen color.
   *
   * @param X1 Starting horizontal coordinate.
   * @param Y1 Starting vertical coordinate.
   * @param X2 Ending horizontal coordinate.
   * @param Y2 Ending vertical coordinate.
   *
   * Example:
   *
   *     // Paint a blue X over the whole canvas
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.drawLine(0, 0, Canvas.getWidth() - 1, Canvas.getHeight() - 1);
   *     Canvas.drawLine(Canvas.getWidth() - 1, 0, 0, Canvas.getHeight() - 1);
   */
  void drawLine(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Draws a rectangle using the current pen color.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordinate.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordinate.
   *
   * Example:
   *
   *     // Paint a yellow rectangle
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawRectangle(10, 10, 100, 100);
   */
  void drawRectangle(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Draws a rectangle using the current pen color.
   *
   * @param rect Rectangle coordinates.
   */
  void drawRectangle(Rect const & rect);

  /**
   * @brief Fills a rectangle using the current brush color.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordinate.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordinate.
   *
   * Example:
   *
   *     // Paint a filled yellow rectangle
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(10, 10, 100, 100);
   *
   *     // Paint a yellow rectangle with a blue border
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(10, 10, 100, 100);
   *     Canvas.drawRectangle(10, 10, 100, 100);
   */
  void fillRectangle(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Fills a rectangle using the current brush color.
   *
   * @param rect Rectangle coordinates.
   *
   * Example:
   *
   *     // Paint a filled yellow rectangle
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillRectangle(Rect(10, 10, 100, 100));
   */
  void fillRectangle(Rect const & rect);

  /**
   * @brief Inverts a rectangle.
   *
   * The logical NOT operator is applied to the pixels inside the rectangle.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordinate.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordinate.
   */
  void invertRectangle(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Inverts a rectangle.
   *
   * The logical NOT operator is applied to the pixels inside the rectangle.
   *
   * @param rect Rectangle coordinates.
   */
  void invertRectangle(Rect const & rect);

  /**
   * @brief Swaps the pen and brush colors inside the specified rectangle.
   *
   * @param X1 Top left horizontal coordinate.
   * @param Y1 Top left vertical coordinate.
   * @param X2 Bottom right horizontal coordinate.
   * @param Y2 Bottom right vertical coordinate.
   */
  void swapRectangle(int X1, int Y1, int X2, int Y2);

  /**
   * @brief Draws an ellipse with the specified center and size, using the current pen color.
   *
   * @param X Horizontal coordinate of the ellipse center.
   * @param Y Vertical coordinate of the ellipse center.
   * @param width Ellipse width.
   * @param height Ellipse height.
   *
   * Example:
   *
   *     // Paint a yellow ellipse
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawEllipse(100, 100, 40, 40);
   */
  void drawEllipse(int X, int Y, int width, int height);

  /**
   * @brief Fills an ellipse with the specified center and size, using the current brush color.
   *
   * @param X Horizontal coordinate of the ellipse center.
   * @param Y Vertical coordinate of the ellipse center.
   * @param width Ellipse width.
   * @param height Ellipse height.
   *
   * Example:
   *
   *     // Paint a filled yellow ellipse
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillEllipse(100, 100, 40, 40);
   *
   *     // Paint a yellow ellipse with a blue border
   *     Canvas.setPenColor(Color::BrightBlue);
   *     Canvas.setBrushColor(Color::BrightYellow);
   *     Canvas.fillEllipse(100, 100, 40, 40);
   *     Canvas.drawEllipse(100, 100, 40, 40);
   */
  void fillEllipse(int X, int Y, int width, int height);

  /**
   * @brief Draws a glyph at the specified position.
   *
   * A glyph is a monochrome bitmap (1 bit per pixel) painted with the pen (foreground) and brush (background) colors.<br>
   * Drawing options can be set with setGlyphOptions().<br>
   * Glyphs are used by Terminal to render characters.
   *
   * @param X Horizontal coordinate of the glyph.
   * @param Y Vertical coordinate of the glyph.
   * @param width Horizontal size of the glyph.
   * @param height Vertical size of the glyph.
   * @param data Memory buffer containing the glyph. Each line is byte aligned, so the buffer size must be "(width + 7) / 8 * height" (integer division).
   * @param index Optional glyph index inside data, used when the buffer contains multiple glyphs.
   *
   * Example:
   *
   *     // draw an 'A' using the predefined 8x8 font
   *     Canvas.setPenColor(Color::BrightGreen);
   *     const fabgl::FontInfo * f = &fabgl::FONT_8x8;
   *     Canvas.drawGlyph(0, 0, f->width, f->height, f->data, 0x41);
   *
   *     // draw a 12x8 sprite
   *     const uint8_t enemy[] = {
   *       0x0f, 0x00, 0x7f, 0xe0, 0xff, 0xf0, 0xe6, 0x70, 0xff,
   *       0xf0, 0x39, 0xc0, 0x66, 0x60, 0x30, 0xc0,
   *     };
   *     Canvas.setPenColor(Color::BrightYellow);
   *     Canvas.drawGlyph(50, 80, 12, 8, enemy);
   *
   */
  void drawGlyph(int X, int Y, int width, int height, uint8_t const * data, int index = 0);

  /**
   * @brief Sets drawing options for the next glyphs.
   *
   * Glyph options change how glyphs are rendered, applying
   * effects such as bold, italic, inversion, double width or double height.<br>
   * Since Canvas draws text using glyphs, these options also affect
   * how text is rendered.
   *
   * @param options Bit field specifying multiple options
   *
   * Example:
   *
   *     // Draw "Hello World!"
   *     Canvas.setGlyphOptions(GlyphOptions().FillBackground(true).DoubleWidth(1));
   *     Canvas.drawText(20, 20, "Hello World!");
   */
  void setGlyphOptions(GlyphOptions options);

  /**
   * @brief Resets glyph options.
   */
  void resetGlyphOptions();

  void renderGlyphsBuffer(int itemX, int itemY, GlyphsBuffer const * glyphsBuffer);

  /**
   * @brief Sets paint options.
   */
  void setPaintOptions(PaintOptions options);

  /**
   * @brief Resets paint options.
   */
  void resetPaintOptions();

  /**
   * @brief Gets information about the currently selected font.
   *
   * @return FontInfo structure containing font information and data.
   */
  FontInfo const * getFontInfo() { return m_fontInfo; }

  /**
   * @brief Selects the font used by subsequent text drawing operations.
   *
   * @param fontInfo Pointer to the font structure containing font information and glyph data.
   *
   * Examples:
   *
   *     // Set a font suitable for a text screen of about 40x14 characters
   *     Canvas.selectFont(fabgl::getPresetFontInfo(Canvas.getWidth(), Canvas.getHeight(), 40, 14));
   *
   *     // Set the 8x8 predefined font
   *     Canvas.selectFont(&fabgl::FONT_8x8);
   */
  void selectFont(FontInfo const * fontInfo);

  /**
   * @brief Draws a character at the specified position.
   *
   * drawChar() uses the font selected with selectFont() and the glyph options set with setGlyphOptions().
   *
   * @param X Horizontal position of the left side of the character.
   * @param Y Vertical position of the top side of the character.
   * @param c Character to draw (an index into the font glyph set).
   *
   * Example:
   *
   *     // Draw a 'C' at position 100, 100
   *     Canvas.drawChar(100, 100, 'C');
   */
  void drawChar(int X, int Y, char c);

  /**
   * @brief Draws a string at the specified position.
   *
   * drawText() uses the font selected with selectFont() and the glyph options set with setGlyphOptions().
   *
   * @param X Horizontal position of the left side of the first character.
   * @param Y Vertical position of the top side of the first character.
   * @param text String to draw (indexes into the font glyph set).
   * @param wrap If true, text wraps at the end of the line.
   *
   * Example:
   *
   *     // Draw a 'Hello World!' at position 100, 100
   *     Canvas.drawText(100, 100, "Hello World!");
   */
  void drawText(int X, int Y, char const * text, bool wrap = false);

  /**
   * @brief Draws a string at the specified position, using the specified font.
   *
   * drawText() uses the specified font and the glyph options set with setGlyphOptions().
   *
   * @param fontInfo Pointer to the font structure containing font information and glyph data.
   * @param X Horizontal position of the left side of the first character.
   * @param Y Vertical position of the top side of the first character.
   * @param text String to draw (indexes into the font glyph set).
   * @param wrap If true, text wraps at the end of the line.
   *
   * Example:
   *
   *     // Draw a 'Hello World!' at position 100, 100
   *     Canvas.drawText(&fabgl::FONT_8x8, 100, 100, "Hello World!");
   */
  void drawText(FontInfo const * fontInfo, int X, int Y, char const * text, bool wrap = false);

  /**
   * @brief Draws a string at the specified position, truncating it with an ellipsis when it exceeds maxX.
   *
   * @param fontInfo Pointer to the font structure containing font information and glyph data.
   * @param X Horizontal position of the left side of the first character.
   * @param Y Vertical position of the top side of the first character.
   * @param text String to draw (indexes into the font glyph set).
   * @param maxX Maximum allowed horizontal coordinate.
   */
  void drawTextWithEllipsis(FontInfo const * fontInfo, int X, int Y, char const * text, int maxX);

  /**
   * @brief Calculates the width of a text, in pixels.
   *
   * @param fontInfo Pointer to the font structure containing font information and glyph data.
   * @param text String to measure (indexes into the font glyph set).
   */
  int textExtent(FontInfo const * fontInfo, char const * text);

  /**
   * @brief Calculates the width of a text in pixels, using the current font.
   *
   * @param text String to measure (indexes into the font glyph set).
   */
  int textExtent(char const * text);

  /**
   * @brief Draws formatted text at the specified position.
   *
   * drawTextFmt() uses the font selected with selectFont() and the glyph options set with setGlyphOptions().
   *
   * @param X Horizontal position of the left side of the first character.
   * @param Y Vertical position of the top side of the first character.
   * @param format printf-like format string.
   *
   * Example:
   *
   *     Canvas.drawTextFmt(100, 100, "Free DMA memory: %d", heap_caps_get_free_size(MALLOC_CAP_DMA));
   */
  void drawTextFmt(int X, int Y, const char *format, ...);

  /**
   * @brief Copies a screen rectangle to the specified position.
   *
   * Source and destination rectangles may overlap. The copy is performed pixel by pixel, so it is a slow operation.
   *
   * @param sourceX Horizontal position of the left side of the source rectangle.
   * @param sourceY Vertical position of the top side of the source rectangle.
   * @param destX Horizontal position of the left side of the destination rectangle.
   * @param destY Vertical position of the top side of the destination rectangle.
   * @param width Rectangle width.
   * @param height Rectangle height.
   */
  void copyRect(int sourceX, int sourceY, int destX, int destY, int width, int height);

  /**
   * @brief Draws a bitmap at the specified position.
   *
   * A bitmap is a rectangular image. Depending on its format it can be monochrome, use 64 colors with 2 bits of
   * transparency (RGBA2222) or use 8 bits per channel (RGBA8888). See the Bitmap and PixelFormat documentation.
   *
   * @param X Horizontal position of the left side of the bitmap.
   * @param Y Vertical position of the top side of the bitmap.
   * @param bitmap Pointer to bitmap structure.
   */
  void drawBitmap(int X, int Y, Bitmap const * bitmap);

  /**
   * @brief Draws a sequence of lines.
   *
   * @param points Pointer to an array of Point objects. The array is copied to a temporary buffer.
   * @param pointsCount Number of points in the array.
   *
   * Example:
   *
   *     Point points[3] = { {10, 10}, {20, 10}, {15, 20} };
   *     Canvas.setPenColor(Color::Red);
   *     Canvas.drawPath(points, 3);
   */
  void drawPath(Point const * points, int pointsCount);

  /**
   * @brief Fills the polygon enclosed in a sequence of lines.
   *
   * @param points Pointer to an array of Point objects. The array is copied to a temporary buffer.
   * @param pointsCount Number of points in the array.
   *
   * Example:
   *
   *     Point points[3] = { {10, 10}, {20, 10}, {15, 20} };
   *     Canvas.setBrushColor(Color::Red);
   *     Canvas.fillPath(points, 3);
   */
  void fillPath(Point const * points, int pointsCount);

  /**
   * @brief Reads the pixel at the specified position.
   *
   * The screen may be read while other drawings are still in progress, so the result may be outdated. To avoid this, call getPixel() after
   * waitCompletion(), or disable background drawing.
   *
   * @param X Horizontal coordinate.
   * @param Y Vertical coordinate.
   *
   * @return Pixel color as an RGB888 structure.
   */
  RGB888 getPixel(int X, int Y);

private:

  BitmappedDisplayController * m_displayController;

  FontInfo const *    m_fontInfo;
  uint8_t             m_textHorizRate; // specify character size: 1 = m_fontInfo.width, 2 = m_fontInfo.width * 2, etc.

  Point               m_origin;
  Rect                m_clippingRect;
};


} // end of namespace







