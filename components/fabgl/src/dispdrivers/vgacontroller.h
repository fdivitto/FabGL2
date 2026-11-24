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
 * @brief This file contains the fabgl::VGAController definition.
 */


#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"

#include "fabglconf.h"
#include "fabutils.h"
#include "dispdrivers/vgabasecontroller.h"




namespace fabgl {




/**
* @brief VGA bitmapped display controller (64 colors)
*
* Use this class to set the screen resolution and to assign the VGA signals to ESP32 GPIOs.
*
* This example initializes the VGA controller with 64 colors at 640x350:
*
*     fabgl::VGAController VGAController;
*     // the default pinout assigns GPIO22 and GPIO21 to red, GPIO19 and GPIO18 to green, GPIO5 and GPIO4 to blue, GPIO23 to HSync and GPIO15 to VSync
*     VGAController.begin();
*     VGAController.setResolution(VGA_640x350_70Hz);
*
* This example initializes the VGA controller with 8 colors (5 GPIOs) at 640x350:
*
*     // Assign GPIO22 to red, GPIO21 to green, GPIO19 to blue, GPIO18 to HSync and GPIO5 to VSync
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
*
*     // Set 640x350@70Hz resolution
*     VGAController.setResolution(VGA_640x350_70Hz);
*
* This example initializes the VGA controller with 64 colors (8 GPIOs) at 640x350:
*
*     // Assign GPIO22 and GPIO21 to red, GPIO19 and GPIO18 to green, GPIO5 and GPIO4 to blue, GPIO23 to HSync and GPIO15 to VSync
*     VGAController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);
*
*     // Set 640x350@70Hz resolution
*     VGAController.setResolution(VGA_640x350_70Hz);
*
* @see @ref guide_display
*
* @ingroup display
*/
class VGAController : public VGABaseController {

public:

  VGAController();

  // unwanted methods
  VGAController(VGAController const&)   = delete;
  void operator=(VGAController const&)  = delete;


  /**
   * @brief Returns the singleton instance of the VGAController class
   *
   * @return A pointer to the VGAController singleton
   */
  static VGAController * instance() { return s_instance; }

  // abstract method of BitmappedDisplayController
  void suspendBackgroundPrimitiveExecution();

  // abstract method of BitmappedDisplayController
  void resumeBackgroundPrimitiveExecution();

  // abstract method of BitmappedDisplayController
  NativePixelFormat nativePixelFormat() { return NativePixelFormat::SBGR2222; }

  // import "modeline" version of setResolution
  using VGABaseController::setResolution;

  void setResolution(VGATimings const& timings, int viewPortWidth = -1, int viewPortHeight = -1, bool doubleBuffered = false);

  /**
   * @brief Reads pixels inside the specified rectangle.
   *
   * The screen may be read while other drawings are still in progress, so the result may be outdated.
   *
   * @param rect Screen rectangle to read. For performance reasons the rectangle is not validated.
   * @param destBuf Destination buffer. Its size must be at least rect.width() * rect.height().
   *
   * Example:
   *
   *     // paint a red rectangle
   *     Canvas.setBrushColor(Color::BrightRed);
   *     Rect rect = Rect(10, 10, 100, 100);
   *     Canvas.fillRectangle(rect);
   *
   *     // wait for vsync (hence actual drawing)
   *     VGAController.processPrimitives();
   *
   *     // read rectangle pixels into "buf"
   *     auto buf = new RGB222[rect.width() * rect.height()];
   *     VGAController.readScreen(rect, buf);
   *
   *     // write buf 110 pixels to the right
   *     VGAController.writeScreen(rect.translate(110, 0), buf);
   *     delete [] buf;
   */
  void readScreen(Rect const & rect, RGB222 * destBuf);

  void readScreen(Rect const & rect, RGB888 * destBuf);

  /**
   * @brief Writes pixels inside the specified rectangle.
   *
   * The screen may be written while other drawings are still in progress, so the written pixels may be overwritten or mixed.
   *
   * @param rect Screen rectangle to write. For performance reasons the rectangle is not validated.
   * @param srcBuf Source buffer. Its size must be at least rect.width() * rect.height().
   *
   * Example:
   *
   *     // paint a red rectangle
   *     Canvas.setBrushColor(Color::BrightRed);
   *     Rect rect = Rect(10, 10, 100, 100);
   *     Canvas.fillRectangle(rect);
   *
   *     // wait for vsync (hence actual drawing)
   *     VGAController.processPrimitives();
   *
   *     // read rectangle pixels into "buf"
   *     auto buf = new RGB222[rect.width() * rect.height()];
   *     VGAController.readScreen(rect, buf);
   *
   *     // write buf 110 pixels to the right
   *     VGAController.writeScreen(rect.translate(110, 0), buf);
   *     delete [] buf;
   */
  void writeScreen(Rect const & rect, RGB222 * srcBuf);

  /**
   * @brief Sets a raw pixel created with createRawPixel().
   *
   * A raw pixel (or raw color) is a byte (uint8_t) containing color information and synchronization signals.
   *
   * @param x Horizontal pixel position
   * @param y Vertical pixel position
   * @param rgb Raw pixel color
   *
   * Example:
   *
   *     // Set the color of the pixel at 100, 100
   *     VGAController.setRawPixel(100, 100, VGAController.createRawPixel(RGB222(3, 0, 0)));
   */
  void setRawPixel(int x, int y, uint8_t rgb)    { VGA_PIXEL(x, y) = rgb; }


private:

  void init();

  void allocateViewPort();
  void onSetupDMABuffer(lldesc_t volatile * buffer, bool isStartOfVertFrontPorch, int scan, bool isVisible, int visibleRow);

  // abstract method of BitmappedDisplayController
  void setPixelAt(PixelDesc const & pixelDesc, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void drawEllipse(Size const & size, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void clear(Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void VScroll(int scroll, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void HScroll(int scroll, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void drawGlyph(Glyph const & glyph, GlyphOptions glyphOptions, RGB888 penColor, RGB888 brushColor, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void invertRect(Rect const & rect, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void copyRect(Rect const & source, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void swapFGBG(Rect const & rect, Rect & updateRect);

  // abstract method of BitmappedDisplayController
  void rawDrawBitmap_Native(int destX, int destY, Bitmap const * bitmap, int X1, int Y1, int XCount, int YCount);

  // abstract method of BitmappedDisplayController
  void rawDrawBitmap_Mask(int destX, int destY, Bitmap const * bitmap, void * saveBackground, int X1, int Y1, int XCount, int YCount);

  // abstract method of BitmappedDisplayController
  void rawDrawBitmap_RGBA2222(int destX, int destY, Bitmap const * bitmap, void * saveBackground, int X1, int Y1, int XCount, int YCount);

  // abstract method of BitmappedDisplayController
  void rawDrawBitmap_RGBA8888(int destX, int destY, Bitmap const * bitmap, void * saveBackground, int X1, int Y1, int XCount, int YCount);

  // abstract method of BitmappedDisplayController
  void rawFillRow(int y, int x1, int x2, RGB888 color);

  void rawFillRow(int y, int x1, int x2, uint8_t pattern);

  void rawInvertRow(int y, int x1, int x2);

  void swapRows(int yA, int yB, int x1, int x2);

  // abstract method of BitmappedDisplayController
  void absDrawLine(int X1, int Y1, int X2, int Y2, RGB888 color);

  // abstract method of BitmappedDisplayController
  int getBitmapSavePixelSize() { return 1; }

  static void VSyncInterrupt(void * arg);
  
  #ifdef FABGL_EMULATED
  virtual void updateFrameBuffer(uint32_t * frameBuffer);
  #endif



  static VGAController * s_instance;

  volatile int16_t       m_maxVSyncISRTime; // Maximum us VSync interrupt routine can run

};



} // end of namespace







