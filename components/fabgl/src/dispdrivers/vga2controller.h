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
 * @brief This file contains the fabgl::VGA2Controller definition.
 */


#include <stdint.h>
#include <stddef.h>
#include <atomic>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "fabglconf.h"
#include "fabutils.h"
#include "devdrivers/swgenerator.h"
#include "displaycontroller.h"
#include "dispdrivers/vgapalettedcontroller.h"



// must be a power of 2
#define VGA2_LinesCount 4




namespace fabgl {



/**
* @brief VGA bitmapped display controller with 2 colors
*
* This VGA controller displays only 2 colors, but at the same resolution it requires 1/8 of the RAM used by VGAController. Each pixel is
* represented by 1 bit: an index into a palette of 2 colors. Each byte of the frame buffer contains eight pixels.
* For example, a 640x480 frame buffer requires about 40 KB of RAM.
* This controller also supports higher resolutions, up to 1280x768.
*
* VGA2Controller uses up to 10% of one CPU core (measured at 640x480@60Hz), leaving more free RAM to the application.
*
*
* This example initializes the VGA controller with 64 colors (2 visible at the same time) at 640x480:
*
*     fabgl::VGA2Controller displayController;
*     // the default pinout assigns GPIO22 and GPIO21 to red, GPIO19 and GPIO18 to green, GPIO5 and GPIO4 to blue, GPIO23 to HSync and GPIO15 to VSync
*     displayController.begin();
*     displayController.setResolution(VGA_640x480_60Hz);
*
* @see @ref guide_display
*
* @ingroup display
*/
class VGA2Controller : public VGAPalettedController {

public:

  VGA2Controller();
  ~VGA2Controller();

  // unwanted methods
  VGA2Controller(VGA2Controller const&) = delete;
  void operator=(VGA2Controller const&) = delete;


  /**
   * @brief Returns the singleton instance of the VGA2Controller class
   *
   * @return A pointer to the VGA2Controller singleton
   */
  static VGA2Controller * instance() { return s_instance; }

  void readScreen(Rect const & rect, RGB888 * destBuf);

  /**
   * @brief Sets the color of the specified palette item
   *
   * @param index Palette item (0..1)
   * @param color Color to assign to the item
   *
   * Example:
   *
   *     // Set palette item 0 to pure red
   *     displayController.setPaletteItem(0, RGB888(255, 0, 0));
   */
  void setPaletteItem(int index, RGB888 const & color);


protected:

  void setupDefaultPalette();


private:

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
  void rawFillRow(int y, int x1, int x2, uint8_t colorIndex);

  void rawInvertRow(int y, int x1, int x2);

  void rawCopyRow(int x1, int x2, int srcY, int dstY);

  void swapRows(int yA, int yB, int x1, int x2);

  // abstract method of BitmappedDisplayController
  void absDrawLine(int X1, int Y1, int X2, int Y2, RGB888 color);

  // abstract method of BitmappedDisplayController
  int getBitmapSavePixelSize() { return 1; }

  static void ISRHandler(void * arg);



  static VGA2Controller *     s_instance;

  volatile uint64_t *         m_packedPaletteIndexOctet_to_signals;

};



} // end of namespace







