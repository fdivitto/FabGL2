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
 * @brief This file contains the fabgl::Terminal definition.
 */


#include <ctype.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"


#include "fabglconf.h"
#include "canvas.h"
#include "devdrivers/keyboard.h"
#include "terminfo.h"
#include "devdrivers/soundgen.h"



/**
 * @page vttest VTTEST VT100/VT102 Compatibility Test Score Sheet
 *
 * Results obtained running VTTEST remotely on a 640x350 screen, using the default 80 and 132 column fonts:
 *
 *     1. Test of cursor movements
 *
 *     [X]   1. Text inside frame of E's inside frame of *'s and +'s, 80 columns
 *     [X]   2. Text inside frame of E's inside frame of *'s and +'s, 132 columns
 *     [X]   3. Cursor-control chars inside ESC sequences
 *     [X]   4. Leading 0's in ESC sequences
 *
 *     2. Test of screen features
 *
 *     [X]   5. Three identical lines of *'s (test of wrap mode)
 *     [X]   6. Test of tab setting/resetting
 *     [X]   7. 132-column mode, light background
 *     [X]   8. 80-column mode, light background
 *     [X]   9. 132-column mode, dark background
 *     [X]  10. 80-column mode, dark background
 *     [X]  11. Soft scroll down
 *     [X]  12. Soft scroll up / down
 *     [X]  13. Jump scroll down
 *     [X]  14. Jump scroll up / down
 *     [X]  15. Origin mode test (2 parts)
 *
 *          Graphic Rendition test pattern, dark background
 *
 *     [X]  16. Normal ("vanilla")
 *     [X]  17. Normal underlined distinct from normal
 *     [X]  18. Normal blink distinct from all above
 *     [X]  19. Normal underline blink distinct from all above
 *     [X]  20. Normal reverse ("negative") distinct from all above
 *     [X]  21. Normal underline reverse distinct from all above
 *     [X]  22. Normal blink reverse distinct from all above
 *     [X]  23. Normal underline blink reverse distinct from all above
 *     [X]  24. Bold distinct from all above
 *     [X]  25. Bold underlined distinct from all above
 *     [X]  26. Bold blink distinct from all above
 *     [X]  27. Bold underline blink distinct from all above
 *     [X]  28. Bold reverse ("negative") distinct from all above
 *     [X]  29. Bold underline reverse distinct from all above
 *     [X]  30. Bold blink reverse distinct from all above
 *     [X]  31. Bold underline blink reverse distinct from all above
 *
 *          Graphic Rendition test pattern, light background
 *
 *     [X]  32. Normal ("vanilla")
 *     [X]  33. Normal underlined distinct from normal
 *     [X]  34. Normal blink distinct from all above
 *     [X]  35. Normal underline blink distinct from all above
 *     [X]  36. Normal reverse ("negative") distinct from all above
 *     [X]  37. Normal underline reverse distinct from all above
 *     [X]  38. Normal blink reverse distinct from all above
 *     [X]  39. Normal underline blink reverse distinct from all above
 *     [X]  40. Bold distinct from all above
 *     [X]  41. Bold underlined distinct from all above
 *     [X]  42. Bold blink distinct from all above
 *     [X]  43. Bold underline blink distinct from all above
 *     [X]  44. Bold reverse ("negative") distinct from all above
 *     [X]  45. Bold underline reverse distinct from all above
 *     [X]  46. Bold blink reverse distinct from all above
 *     [X]  47. Bold underline blink reverse distinct from all above
 *
 *          Save/Restore Cursor
 *
 *     [X]  48. AAAA's correctly placed
 *     [X]  49. Lines correctly rendered (middle of character cell)
 *     [X]  50. Diamonds correctly rendered
 *
 *     3. Test of character sets
 *
 *     [X]  51. UK/National shows Pound Sterling sign in 3rd position
 *     [X]  52. US ASCII shows number sign in 3rd position
 *     [X]  53. SO/SI works (right columns identical with left columns)
 *     [X]  54. True special graphics & line drawing chars, not simulated by ASCII
 *
 *     4. Test of double-sized chars
 *
 *          Test 1 in 80-column mode:
 *
 *     [X]  55. Left margin correct
 *     [X]  56. Width correct
 *
 *          Test 2 in 80-column mode:
 *
 *     [X]  57. Left margin correct
 *     [X]  58. Width correct
 *
 *          Test 1 in 132-column mode:
 *
 *     [X]  59. Left margin correct
 *     [X]  60. Width correct
 *
 *          Test 2 in 132-column mode:
 *
 *     [X]  61. Left margin correct
 *     [X]  62. Width correct
 *
 *     [X]  63. "The man programmer strikes again" test pattern
 *     [X]  64. "Exactly half the box should remain"
 *
 *     5. Test of keyboard
 *
 *     [X]  65. LEDs.
 *     [X]  66. Autorepeat
 *     [X]  67. "Press each key" (ability to send each ASCII graphic char)
 *     [X]  68. Arrow keys (ANSI/Cursor key mode reset)
 *     [X]  69. Arrow keys (ANSI/Cursor key mode set)
 *     [X]  70. Arrow keys VT52 mode
 *     [ ]  71. PF keys numeric mode
 *     [ ]  72. PF keys application mode
 *     [ ]  73. PF keys VT52 numeric mode
 *     [ ]  74. PF keys VT52 application mode
 *     [ ]  75. Send answerback message from keyboard
 *     [X]  76. Ability to send every control character
 *
 *     6. Test of Terminal Reports
 *
 *     [ ]  77. Respond to ENQ with answerback
 *     [X]  78. Newline mode set
 *     [X]  79. Newline mode reset
 *     [X]  80. Device status report 5
 *     [X]  81. Device status report 6
 *     [ ]  82. Device attributes report
 *     [ ]  83. Request terminal parameters 0
 *     [ ]  84. Request terminal parameters 1
 *
 *     7. Test of VT52 submode
 *
 *     [X]  85. Centered rectangle
 *     [X]  86. Normal character set
 *     [X]  87. Graphics character set
 *     [X]  88. Identify query
 *
 *     8. VT102 Features
 *
 *     [X]  89. Insert/delete line, 80 columns
 *     [X]  90. Insert (character) mode, 80 columns
 *     [X]  91. Delete character, 80 columns
 *     [X]  92. Right column staggered by 1 (normal chars), 80 columns
 *     [X]  93. Right column staggered by 1 (double-wide chars), 80 columns
 *     [X]  94. ANSI insert character, 80 columns
 *     [X]  95. Insert/delete line, 132 columns
 *     [X]  96. Insert (character) mode, 132 columns
 *     [X]  97. Delete character, 132 columns
 *     [X]  98. Right column staggered by 1 (normal chars), 132 columns
 *     [X]  99. Right column staggered by 1 (double-wide chars), 132 columns
 *     [X] 100. ANSI insert character, 132 columns
 *
 *     9. Extra credit
 *
 *     [X] 101. True soft (smooth) scroll
 *     [X] 102. True underline
 *     [X] 103. True blink
 *     [X] 104. True double-high/wide lines, not simulated
 *     [ ] 105. Reset terminal (*)
 *     [ ] 106. Interpret controls (debug mode) (*)
 *     [ ] 107. Send BREAK (250 msec) (*)
 *     [ ] 108. Send Long BREAK (1.5 sec) (*)
 *     [ ] 109. Host-controlled transparent / controller print (*)
 *     [ ] 110. Host-controlled autoprint (*) 
 */


 /**
 * @page specialTermEscapes FabGL Specific Terminal Sequences
 *
 * This page lists the FabGL-specific terminal sequences. They give access to features, such as graphics, audio and GPIO, that are not available through standard escape sequences.<br>
 * They are also useful when the active terminal emulation is unknown and the application still needs to control the screen with a known set of sequences.<br>
 * A FabGL-specific sequence begins with ESC (ASCII 27, 0x1B) followed by an underscore ("_", ASCII 95, 0x5F), and then by the command, which is a single letter.<br>
 * Parameters, when required, follow the command letter and are separated by semicolons (";"). A dollar sign ("$", ASCII 36, 0x24) terminates the sequence.<br>
 * Some commands send back a response: it has a fixed length and starts with a dollar sign ("$").<br><br>
 *
 * <br>
 * <hr>
 *
 * <br> <b> Setup Analog to Digital Converter (ADC) </b>
 *
 *     Sequence:
 *       ESC "_A" resolution ";" attenuation ";" gpio "$"
 *
 *     Parameters:
 *       resolution:
 *           "9", "10", "11", "12"
 *       attenuation:
 *           "0" = 0dB   (reduced to 1/1), full-scale voltage 1.1 V, accurate between 100 and 950 mV
 *           "1" = 2.5dB (reduced to 1/1.34), full-scale voltage 1.5 V, accurate between 100 and 1250 mV
 *           "2" = 6dB   (reduced to 1/2), full-scale voltage 2.2 V, accurate between 150 and 1750 mV
 *           "3" = 11dB  (reduced to 1/3.6), full-scale voltage 3.9 V (the input voltage must not exceed 3.3 V), accurate between 150 and 2450 mV
 *       gpio:
 *           "32"..."39"
 *
 *     Example:
 *       // set up GPIO 36 as analog input, 11dB attenuation (3) and 12-bit resolution
 *       Terminal.write("\e_A12;3;36$");
 *
 * <br> <b> Read analog input (ADC) from the specified GPIO </b>
 *
 *     Sequence:
 *       ESC "_C" gpio "$"
 *
 *     Parameters:
 *       gpio:
 *           "32"..."39"
 *
 *     Returns:
 *       "$"
 *       hex value (3 characters)
 *
 *     Example:
 *       // Read the ADC value of GPIO 36
 *       Terminal.write("\e_C36$");  // returns something like "$1A5" (value 421)
 *
 * <br> <b> Configure a digital pin as input or output </b>
 *
 *     Sequence:
 *       ESC "_D" mode gpio "$"
 *
 *     Parameters:
 *       mode:
 *           "-" = disable input/output
 *           "I" = input only
 *           "O" = output only
 *           "D" = output only with open-drain
 *           "E" = output and input with open-drain
 *           "X" = output and input
 *       gpio:
 *           "0"..."39" (not all GPIOs are usable)
 *
 *     Example:
 *       // Configure GPIO 12 as output (O)
 *       Terminal.write("\e_DO12$");
 *
 * <br> <b> Set digital output pin state </b>
 *
 *     Sequence:
 *       ESC "_W" value gpio "$"
 *
 *     Parameters:
 *       value:
 *           0, '0' or 'L' = low (any other value is also low)
 *           1, '1' or 'H' = high
 *       gpio:
 *           "0"..."39" (not all GPIOs are usable)
 *
 *     Example:
 *       // Set GPIO 12 high
 *       Terminal.write("\e_WH12$");
 *       // Set GPIO 12 low
 *       Terminal.write("\e_WL12$");
 *
 * <br> <b> Read digital input pin state </b>
 *
 *     Sequence:
 *       ESC "_R" gpio "$"
 *
 *     Parameters:
 *       gpio:
 *           "0"..."39" (not all GPIOs are usable)
 *
 *     Returns:
 *       "$"
 *       '0' = low, '1' = high
 *
 *     Example:
 *       // Read the state of GPIO 12
 *       Terminal.write("\e_R12$");  // returns "$0" or "$1"
 *
 * <br> <b> Clear the terminal area with the background color </b>
 *
 *     Sequence:
 *       ESC "_B" "$"
 *
 *     Example:
 *       // Clear terminal area
 *       Terminal.write("\e_B$");
 *
 * <br> <b> Enable or disable the cursor </b>
 *
 *     Sequence:
 *       ESC "_E" state "$"
 *
 *     Parameters:
 *       state:
 *           "0" = disable cursor
 *           "1" = enable cursor
 *
 *     Example:
 *       // Disable cursor
 *       Terminal.write("\e_E0$");
 *
 * <br> <b> Set the cursor position </b>
 *
 *     Sequence:
 *       ESC "_F" column ";" row "$"
 *
 *     Parameters:
 *       column:
 *           column (1 = first column)
 *       row:
 *           row (1 = first row)
 *
 *     Example:
 *       // Print "Hello" at column 1 of row 10
 *       Terminal.write("\e_F1;10$");
 *       Terminal.write("Hello");
 *
 * <br> <b> Show or hide the mouse pointer </b>
 *
 *     Sequence:
 *       ESC "_H" value "$"
 *
 *     Parameters:
 *       value:
 *           '1' = show the mouse pointer
 *           '0' (or any other value) = hide the mouse pointer
 *
 *     Example:
 *       // Show the mouse pointer
 *       Terminal.write("\e_H1$");
 *
 * <br> <b> Get the mouse position </b>
 *
 *     Sequence:
 *       ESC "_M" "$"
 *
 *     Returns:
 *       "$"
 *       X position: 3 hex digits
 *       ";"
 *       Y position: 3 hex digits
 *       ";"
 *       Scroll wheel delta: 1 hex digit (0..F)
 *       ";"
 *       Pressed buttons: 1 hex digit, composed as follows:
 *           bit 0 = left button
 *           bit 1 = middle button
 *           bit 2 = right button
 *
 *     Example:
 *       // Get the mouse status
 *       Terminal.write("\e_M$"); // returns something like "$1AB;08A;0;1"
 *
 * <br> <b> Delay </b>
 *
 *     Sequence:
 *       ESC "_Y" value "$"
 *
 *     Parameters:
 *       value:
 *           number (milliseconds)
 *
 *     Returns:
 *       "$": the dollar sign is sent back after the specified number of milliseconds
 *
 *     Example:
 *       // Wait 500 milliseconds
 *       Terminal.write("\e_Y500$"); // returns "$" after 500 milliseconds
 *
 * <br> <b> Play sound </b>
 *
 *     Sequence:
 *       ESC "_S" waveform ";" frequency ";" duration ";" volume "$"
 *
 *     Parameters:
 *       waveform:
 *           "0" = SINE
 *           "1" = SQUARE
 *           "2" = TRIANGLE
 *           "3" = SAWTOOTH
 *           "4" = NOISE
 *           "5" = VIC NOISE
 *       frequency:
 *           frequency in Hertz
 *       duration:
 *           duration in milliseconds
 *       volume:
 *           volume (min is 0, max is 127)
 *
 *     Example:
 *       // play a sine wave at 800 Hz for 1000 ms, volume 100
 *       Terminal.write("\e_S0;800;1000;100$");
 *
 * <br> <b> Clear the graphics screen with the background color and reset the scrolling region </b>
 *
 *     Sequence:
 *       ESC "_GCLEAR" "$"
 *
 *     Example:
 *       // clear the graphics screen, filling it with dark blue
 *       Terminal.write("\e_GBRUSH0;0;128$");
 *       Terminal.write("\e_GCLEAR$");
 *
 * <br> <b> Set the brush color for graphics </b>
 *
 *     Sequence:
 *       ESC "_GBRUSH" red ";" green ";" blue "$"
 *
 *     Parameters:
 *       red:   '0'..'255'
 *       green: '0'..'255'
 *       blue:  '0'..'255'
 *
 *     Example:
 *       // set pure red (255,0,0) as background color for graphics
 *       Terminal.write("\e_GBRUSH255;0;0$");
 *
 * <br> <b> Set the pen color for graphics </b>
 *
 *     Sequence:
 *       ESC "_GPEN" red ";" green ";" blue "$"
 *
 *     Parameters:
 *       red:   '0'..'255'
 *       green: '0'..'255'
 *       blue:  '0'..'255'
 *
 *     Example:
 *       // set yellow (255,255,0) as pen color for graphics
 *       Terminal.write("\e_GPEN255;255;0$");
 *
 * <br> <b> Set the pen width </b>
 *
 *     Sequence:
 *       ESC "_GPENW" width "$"
 *
 *     Parameters:
 *       width: pen width (from 1)
 *
 *     Example:
 *       // set pen width to 2
 *       Terminal.write("\e_GPENW2$");
 *
 * <br> <b> Set a pixel using the pen color </b>
 *
 *     Sequence:
 *       ESC "_GPIXEL" X ";" Y "$"
 *
 *     Parameters:
 *       X: horizontal coordinate
 *       Y: vertical coordinate
 *
 *     Example:
 *       // set the pixel at 89, 31 to blue
 *       Terminal.write("\e_GPEN0;0;255$"); // pen = blue
 *       Terminal.write("\e_GPIXEL89;31$"); // draw pixel
 *
 * <br> <b> Draw a line using the pen color </b>
 *
 *     Sequence:
 *       ESC "_GLINE" X1 ";" Y1 ";" X2 ";" Y2 "$"
 *
 *     Parameters:
 *       X1: starting horizontal coordinate
 *       Y1: starting vertical coordinate
 *       X2: ending horizontal coordinate
 *       Y2: ending vertical coordinate
 *
 *     Example:
 *       // draw a red line from 10, 10 to 150, 150
 *       Terminal.write("\e_GPEN255;0;0$");        // pen = red
 *       Terminal.write("\e_GLINE10;10;150;150$"); // draw line
 *
 * <br> <b> Draw a rectangle using the pen color </b>
 *
 *     Sequence:
 *       ESC "_GRECT" X1 ";" Y1 ";" X2 ";" Y2 "$"
 *
 *     Parameters:
 *       X1: starting horizontal coordinate
 *       Y1: starting vertical coordinate
 *       X2: ending horizontal coordinate
 *       Y2: ending vertical coordinate
 *
 *     Example:
 *       // draw a white rectangle from 10, 10 to 150, 150
 *       Terminal.write("\e_GPEN255;255;255$");    // pen = white
 *       Terminal.write("\e_GRECT10;10;150;150$"); // draw rectangle
 *
 * <br> <b> Fill a rectangle using the brush color </b>
 *
 *     Sequence:
 *       ESC "_GFILLRECT" X1 ";" Y1 ";" X2 ";" Y2 "$"
 *
 *     Parameters:
 *       X1: starting horizontal coordinate
 *       Y1: starting vertical coordinate
 *       X2: ending horizontal coordinate
 *       Y2: ending vertical coordinate
 *
 *     Example:
 *       // fill a yellow rectangle from 10, 10 to 150, 150
 *       Terminal.write("\e_GBRUSH255;255;0$");         // brush = yellow
 *       Terminal.write("\e_GFILLRECT10;10;150;150$");  // fill rectangle
 *
 * <br> <b> Draw an ellipse using the pen color </b>
 *
 *     Sequence:
 *       ESC "_GELLIPSE" X ";" Y ";" width ";" height "$"
 *
 *     Parameters:
 *       X:      horizontal coordinate of ellipse center
 *       Y:      vertical coordinate of ellipse center
 *       width:  ellipse width
 *       height: ellipse height
 *
 *     Example:
 *       // draw a green ellipse centered at 100, 120, 50 pixels wide and 80 pixels high
 *       Terminal.write("\e_GPEN0;255;0$");            // pen = green
 *       Terminal.write("\e_GELLIPSE100;120;50;80$");  // draw ellipse
 *
 * <br> <b> Fill an ellipse using the brush color </b>
 *
 *     Sequence:
 *       ESC "_GFILLELLIPSE" X ";" Y ";" width ";" height "$"
 *
 *     Parameters:
 *       X:      horizontal coordinate of ellipse center
 *       Y:      vertical coordinate of ellipse center
 *       width:  ellipse width
 *       height: ellipse height
 *
 *     Example:
 *       // fill a red ellipse centered at 100, 120, 50 pixels wide and 80 pixels high
 *       Terminal.write("\e_GBRUSH255;0;0$");              // brush = red
 *       Terminal.write("\e_GFILLELLIPSE100;120;50;80$");  // fill ellipse
 *
 * <br> <b> Draw a polygon (path) using the pen color </b>
 *
 *     Sequence:
 *       ESC "_GPATH" X1 ";" Y1 ";" X2 ";" Y2 [";" Xn ";" Yn...] "$"
 *
 *     Parameters:
 *       X1: first horizontal coordinate
 *       Y1: first vertical coordinate
 *       X2: second horizontal coordinate
 *       Y2: second vertical coordinate
 *       Xn: optional n-th horizontal coordinate
 *       Yn: optional n-th vertical coordinate
 *
 *     Notes:
 *       The maximum number of points is 32 (configurable in terminal.h)
 *
 *     Example:
 *       // draw a red triangle at (5,5)-(12,18)-(6,16)
 *       Terminal.write("\e_GPEN255;0;0$");                // pen = red
 *       Terminal.write("\e_GPATH5;5;12;18;6;16$");        // draw path
 *
 * <br> <b> Fill a polygon (path) using the brush color </b>
 *
 *     Sequence:
 *       ESC "_GFILLPATH" X1 ";" Y1 ";" X2 ";" Y2 [";" Xn ";" Yn...] "$"
 *
 *     Parameters:
 *       X1: first horizontal coordinate
 *       Y1: first vertical coordinate
 *       X2: second horizontal coordinate
 *       Y2: second vertical coordinate
 *       Xn: optional n-th horizontal coordinate
 *       Yn: optional n-th vertical coordinate
 *
 *     Notes:
 *       The maximum number of points is 32 (configurable in terminal.h)
 *
 *     Example:
 *       // fill a green triangle at (5,5)-(12,18)-(6,16)
 *       Terminal.write("\e_GBRUSH0;255;0$");              // brush = green
 *       Terminal.write("\e_GFILLPATH5;5;12;18;6;16$");    // fill path
 *
 * <br> <b> Set the number of sprites to allocate </b>
 *
 *     Sequence:
 *       ESC "_GSPRITECOUNT" count "$"
 *
 *     Parameters:
 *       count: number of sprites that will be defined by "_GSPRITEDEF"
 *
 *     Example:
 *       // allocate two sprites
 *       Terminal.write("\e_GSPRITECOUNT2$");
 *
 * <br> <b> Add a bitmap to an allocated sprite </b>
 *
 *     Sequence:
 *       ESC "_GSPRITEDEF" spriteIndex ";" width ";" height ";" format ";" [R ";" G ";" B ";"] data... "$"
 *
 *     Parameters:
 *       spriteIndex: sprite index (0...)
 *       width:       bitmap width
 *       height:      bitmap height
 *       format:
 *           "M" = bitmap format is monochrome (1 bit per pixel)
 *           "2" = bitmap format is 64 colors (6 bits per pixel, 2 bits per channel with transparency)
 *           "8" = bitmap format is true color (32 bits per pixel, 8 bits per channel with transparency)
 *      [R]:          red channel when bitmap format is monochrome
 *      [G]:          green channel when bitmap format is monochrome
 *      [B]:          blue channel when bitmap format is monochrome
 *       data:        bitmap data as a sequence of 2-digit hexadecimal numbers (e.g. 002A3BFF2C...).
 *                    Each bitmap row is byte aligned.
 *
 *     Example:
 *       // allocate one sprite and assign it an 8x4 red monochrome bitmap
 *       Terminal.write("\e_GSPRITECOUNT1$");
 *       Terminal.write("\e_GSPRITEDEF0;8;4;M;255;0;0;AABBCCDD$");
 *
 * <br> <b> Set the sprite visibility, position and current frame (bitmap) </b>
 *
 *     Sequence:
 *       ESC "_GSPRITESET" spriteIndex ";" visible ";" frameIndex ";" X ";" Y "$"
 *
 *     Parameters:
 *       spriteIndex: sprite index (0...)
 *       visible:     "H" = hidden, "V" = visible
 *       frameIndex:  current frame (bitmap) to show (0...)
 *       X:           horizontal position
 *       Y:           vertical position
 *
 *     Example:
 *       // make sprite 0 visible at position 50, 120, showing the first bitmap
 *       Terminal.write("\e_GSPRITESET0;V;0;50;120$");
 *
 * <br> <b> Scroll the screen by pixels </b>
 *
 *     Sequence:
 *       ESC "_GSCROLL" offsetX ";" offsetY "$"
 *
 *     Parameters:
 *       offsetX: number of pixels to scroll (<0 = scroll left, >0 = scroll right)
 *       offsetY: number of pixels to scroll (<0 = scroll up, >0 = scroll down)
 *
 *     Example:
 *       // scroll left by 8 pixels
 *       Terminal.write("\e_GSCROLL-8;0$");
 *
 * <br><br>
 */


namespace fabgl {




// used by saveCursorState / restoreCursorState
struct TerminalCursorState {
  TerminalCursorState *   next;
  int16_t                 cursorX;
  int16_t                 cursorY;
  uint8_t *               tabStop;
  bool                    cursorPastLastCol;
  bool                    originMode;
  GlyphOptions            glyphOptions;
  uint8_t                 characterSetIndex;
  uint8_t                 characterSet[4];
};


enum KeypadMode {
  Application,  // DECKPAM
  Numeric,      // DECKPNM
};


/** \ingroup Enumerations
 * @brief Character styles
 */
enum CharStyle {
  Bold,               /**< Bold */
  ReducedLuminosity,  /**< Reduced luminosity */
  Italic,             /**< Italic */
  Underline,          /**< Underlined */
  Blink,              /**< Blinking text */
  Blank,              /**< Invisible text */
  Inverse,            /**< Swap background and foreground colors */
};


/** \ingroup Enumerations
 * @brief Terminal transition effects
 */
enum class TerminalTransition {
  None,
  LeftToRight,    /**< Left to right */
  RightToLeft,    /**< Right to left */
};


struct EmuState {

  // Index of characterSet[], 0 = G0 (Standard)  1 = G1 (Alternate),  2 = G2,  3 = G3
  uint8_t      characterSetIndex;

  // 0 = DEC Special Character and Line Drawing   1 = United States (USASCII)
  uint8_t      characterSet[4];

  Color        foregroundColor;
  Color        backgroundColor;

  // cursor position (topleft = 1,1)
  int          cursorX;
  int          cursorY;

  bool         cursorPastLastCol;

  bool         originMode;

  bool         wraparound;

  // top and down scrolling regions (1 = first row)
  int          scrollingRegionTop;
  int          scrollingRegionDown;

  bool         cursorEnabled;

  // true = blinking cursor, false = steady cursor
  bool         cursorBlinkingEnabled;

  // 0,1,2 = block  3,4 = underline  5,6 = bar
  int          cursorStyle;

  // column 1 at m_emuState.tabStop[0], column 2 at m_emuState.tabStop[1], etc.  0=no tab stop,  1 = tab stop
  uint8_t *    tabStop;

  // IRM (Insert Mode)
  bool         insertMode;

  // NLM (Automatic CR LF)
  bool         newLineMode;

  // DECSCLM (Smooth scroll)
  // Smooth scroll is effective only when vertical sync refresh is enabled,
  // hence must be BitmappedDisplayController::enableBackgroundPrimitiveExecution(true),
  // that is the default.
  bool         smoothScroll;

  // DECKPAM (Keypad Application Mode)
  // DECKPNM (Keypad Numeric Mode)
  KeypadMode   keypadMode;

  // DECCKM (Cursor Keys Mode)
  bool         cursorKeysMode;

  // DESSCL (1 = VT100 ... 5 = VT500)
  int          conformanceLevel;

  // two values allowed: 7 and 8
  int          ctrlBits;

  bool         keyAutorepeat;

  bool         allow132ColumnMode;

  bool         reverseWraparoundMode;

  // DECBKM (false = BACKSPACE sends BS, false BACKSPACE sends DEL)
  bool         backarrowKeyMode;

  // DECANM (false = VT52 mode, true = ANSI mode)
  bool         ANSIMode;

  // VT52 Graphics Mode
  bool         VT52GraphicsMode;

  // Allow FabGL specific sequences (ESC FABGLEXT_STARTCODE .....)
  int          allowFabGLSequences;  // >0 allow, 0 = don't allow
};



/**
 * @brief An ANSI-VT100 compatible display terminal.
 *
 * Implements most of the common ANSI, VT52, VT100, VT200, VT300, VT420 and VT500 escape codes: non-CSI codes (RIS, IND, DECID, DECDHL, etc.),
 * CSI codes (private modes, CUP, TBC, etc.), CSI-SGR codes (bold, italic, blinking, etc.) and DCS codes (DECRQSS, etc.).<br>
 * PS/2 keyboard virtual keys are translated to the corresponding ANSI or VT codes (keypad, cursor keys, function keys, etc.).<br>
 *
 * Terminal can receive the codes to display from a serial port, or it can be driven directly by the application. Likewise,
 * keyboard codes can be sent to a serial port or delivered directly to the application.<br>
 *
 * By default it supports 80x25 or 132x25 characters at 640x350. Any other resolution and text buffer size is supported
 * by specifying a custom font.<br>
 *
 * Three cursor styles are available (block, underline and bar), each either blinking or steady.
 *
 * Terminal derives from Stream, so applications can use all the Stream and Print input and output methods.
 *
 * Terminal passes 95 of the 110 tests of the VTTEST VT100/VT102 Compatibility Test Score Sheet.
 *
 *
 * Example:
 *
 *     fabgl::VGAController VGAController;
 *     fabgl::PS2Controller PS2Controller;
 *     fabgl::Terminal      Terminal;
 *
 *     // 80x25 loop-back terminal: what you type on the keyboard is shown on the display
 *     extern "C" void app_main()
 *     {
 *       PS2Controller.begin(PS2Preset::KeyboardPort0);
 *
 *       VGAController.begin();
 *       VGAController.setResolution(VGA_640x350_70HzAlt1);
 *
 *       Terminal.begin(&VGAController);
 *       Terminal.connectLocally();      // enables Terminal.read(), available(), etc.
 *       Terminal.enableCursor(true);
 *
 *       while (true) {
 *         int c = Terminal.read(-1);    // wait for a key
 *         switch (c) {
 *           case 0x7F:       // DEL -> backspace + ESC[K
 *             Terminal.write("\\b\e[K");
 *             break;
 *           case 0x0D:       // CR  -> CR + LF
 *             Terminal.write("\r\n");
 *             break;
 *           case 32 ... 126: // printable chars
 *             Terminal.write(c);
 *             break;
 *         }
 *       }
 *     }
 *
 * @see @ref guide_terminal
 *
 * @ingroup terminal
 */
class Terminal : public Stream {

public:

  Terminal();

  ~Terminal();

  /**
   * @brief Initializes the terminal.
   *
   * Applications must call this method after the display resolution has been set and before calling any other Terminal method.
   *
   * @param displayController The output display controller
   * @param maxColumns Maximum number of columns (-1 = determined by the horizontal display resolution)
   * @param maxRows Maximum number of rows (-1 = determined by the vertical display resolution)
   * @param keyboard Keyboard device. nullptr = use the keyboard of PS2Controller
   *
   * @return False if the required memory could not be allocated
   */
  bool begin(BaseDisplayController * displayController, int maxColumns = -1, int maxRows = -1, Keyboard * keyboard = nullptr);

  /**
   * @brief Finalizes the terminal.
   *
   * Applications must call this method before changing the screen resolution.
   */
  void end();

  /**
   * @brief Connects the keyboard to the terminal
   *
   * Creates a task that reads keyboard scancodes and generates the corresponding terminal codes.
   */
  void connectKeyboard();
  
  /**
   * @brief Allows the terminal to be used locally.
   *
   * Creates a queue that receives the ANSI codes decoded from the keyboard and the replies to terminal queries.<br>
   * The queue is accessed with the read(), available() and peek() methods.
   *
   * Example:
   *
   *     Terminal.begin(&VGAController);
   *     Terminal.connectLocally();
   *     // from here you can use Terminal.read() to receive keys from keyboard
   *     // and Terminal.write() to control the display.
   */
  void connectLocally();

  /**
   * @brief Disables local use of the terminal.
   *
   * This is the opposite of connectLocally().
   */
  void disconnectLocally();

  /**
   * @brief Appends a key to the keyboard queue.
   *
   * Characters added with localWrite() are received through the read(), available() and peek() methods.
   *
   * @param c ASCII code to append to the queue.
   */
  void localWrite(uint8_t c);

  /**
   * @brief Appends a string of keys to the keyboard queue.
   *
   * Characters added with localWrite() are received through the read(), available() and peek() methods.
   *
   * @param str A string of ASCII codes to append to the queue.
   */
  void localWrite(char const * str);

  /**
   * @brief Inserts a key at the head of the keyboard queue.
   *
   * Characters inserted with localInsert() are received through the read(), available() and peek() methods.
   *
   * @param c ASCII code to insert into the queue.
   */
  void localInsert(uint8_t c);

  /**
   * @brief Inserts a key at the head of the keyboard queue.
   *
   * Same as localInsert().
   * Inserted characters are received through the read(), available() and peek() methods.
   *
   * @param c ASCII code to insert into the queue.
   */
  void unRead(uint8_t c) { localInsert(c); }

  /**
   * @brief Sets the stream that receives debug logs.
   *
   * The information sent to the log stream is selected by the FABGLIB_TERMINAL_DEBUG_REPORT_... macros in the fabglconf.h configuration file.
   *
   * @param stream The logging stream.
   *
   * Example:
   *
   *     fabgl::UARTStream Console;
   *     Console.begin();
   *     Terminal.begin(&VGAController);
   *     Terminal.setLogStream(Console);
   */
  void setLogStream(Stream & stream) { m_logStream = &stream; }

  void logFmt(const char * format, ...);
  void log(const char * txt);
  void log(char c);

  /**
   * @brief Sets the font to use.
   *
   * Terminal automatically chooses the most suitable font for the screen resolution and the required
   * number of columns and rows.<br>
   * When a specific font is required, applications can set it with loadFont().
   * Only fixed-width fonts are supported.
   *
   * @param font Font to use.
   */
  void loadFont(FontInfo const * font);

  /**
   * @brief Sets the background color.
   *
   * Sets the background color by sending an SGR ANSI code and, optionally, sets the
   * default background color (used when the terminal is reset).
   *
   * @param color Current or default background color.
   * @param setAsDefault If true, the specified color is also used as default.
   *
   * Example:
   *
   *     Terminal.setBackgroundColor(Color::Black);
   */
  void setBackgroundColor(Color color, bool setAsDefault = true);

  /**
   * @brief Sets the foreground color.
   *
   * Sets the foreground color by sending an SGR ANSI code and, optionally, sets the
   * default foreground color (used when the terminal is reset).
   *
   * @param color Current or default foreground color.
   * @param setAsDefault If true, the specified color is also used as default.
   *
   * Example:
   *
   *     Terminal.setForegroundColor(Color::White);
   */
  void setForegroundColor(Color color, bool setAsDefault = true);

  /**
   * @brief Clears the screen.
   *
   * Clears the screen by sending the "CSI 2 J" command.
   *
   * @param moveCursor If true, the cursor is moved to position 1, 1.
   *
   * Example:
   *
   *     // Fill the screen with blue
   *     Terminal.setBackgroundColor(Color::Blue);
   *     Terminal.clear();
   */
  void clear(bool moveCursor = true);

  /**
   * @brief Waits until all codes sent to the display have been processed.
   *
   * @param waitVSync If true, codes are processed during the vertical retrace (from VSync up to about the first visible row).
   *                  If false, all codes are processed immediately.
   */
  void flush(bool waitVSync);

  /**
   * @brief Returns the number of columns.
   *
   * @return The number of columns (in characters).
   */
  int getColumns() { return m_columns; }

  /**
   * @brief Returns the number of lines.
   *
   * @return The number of lines (in characters).
   */
  int getRows()    { return m_rows; }

  /**
   * @brief Enables or disables the cursor.
   *
   * @param value If true, the cursor becomes visible.
   */
  void enableCursor(bool value);

  /**
   * @brief Determines how many codes the display input queue can still accept.
   *
   * @param fromISR If true, the call comes from an ISR. In this case the return value is 1 if the queue can be written, 0 otherwise.
   *
   * @return The free space in the display queue, in characters.
   */
  int availableForWrite(bool fromISR = false);

  /**
   * @brief Sets the terminal type to emulate
   *
   * @param value A terminal to emulate
   *
   * The default and native mode is ANSI/VT100. Other terminals are emulated by translating their codes to the native mode.
   */
  void setTerminalType(TermType value);

  /**
   * @brief Determines the current terminal type
   *
   * @return Terminal type
   */
  TermInfo const & terminalType() { return *m_termInfo; }


  //////////////////////////////////////////////
  //// Stream abstract class implementation ////

  /**
   * @brief Gets the number of codes available in the keyboard queue.
   *
   * The keyboard queue is available only after connectLocally() has been called.
   *
   * @return The number of codes available to read.
   */
  int available();

  /**
   * @brief Reads codes from keyboard.
   *
   * The keyboard queue is available only after connectLocally() has been called.
   *
   * @return The first available code, or -1 if no data is available.
   */
  int read();

  /**
   * @brief Reads codes from the keyboard, with a timeout.
   *
   * The keyboard queue is available only after connectLocally() has been called.
   *
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (infinite wait).
   *
   * @return The first available code, or -1 if no data has been received before the timeout expired.
   */
  int read(int timeOutMS);

  /**
   * @brief Waits for a specific code from the keyboard, discarding all other codes.
   *
   * The keyboard queue is available only after connectLocally() has been called.
   *
   * @param value Char code to wait for.
   * @param timeOutMS Timeout in milliseconds. -1 = no timeout (infinite wait).
   *
   * @return True if the specified value has been received before the timeout expired.
   */
  bool waitFor(int value, int timeOutMS = -1);

  /**
   * @brief Reads a code from the keyboard without advancing to the next one.
   *
   * The keyboard queue is available only after connectLocally() has been called.
   *
   * @return The next code, or -1 if none is available.
   */
  int peek();

  /**
   * @brief Waits until all codes sent to the display have been processed.
   *
   * Codes are processed during the vertical retrace (from VSync up to about the first visible row).
   */
  void flush();

  /**
   * @brief Sends the specified number of codes to the display.
   *
   * Codes can be ANSI/VT codes or ASCII characters.
   *
   * @param buffer Pointer to codes buffer.
   * @param size Number of codes in the buffer.
   *
   * @return The number of codes written.
   *
   * Example:
   *
   *     // Clear the screen and print "Hello World!"
   *     Terminal.write("\e[2J", 4);
   *     Terminal.write("Hello World!\r\n", 14);
   *
   *     // The same without size specified
   *     Terminal.write("\e[2J");
   *     Terminal.write("Hello World!\r\n");
   */
  size_t write(const uint8_t * buffer, size_t size);

  /**
   * @brief Sends a single code to the display.
   *
   * @param c The code to send.
   *
   * @return The number of codes written.
   */
  size_t write(uint8_t c);
  
  /**
   * @brief Sends a single code to the display.
   *
   * @param c The code to send.
   * @param fromISR True if the call comes from an ISR.
   */
  void write(uint8_t c, bool fromISR);

  using Print::write;

  /**
   * @brief Like localWrite(), but also sends the data to the serial port, if connected
   *
   * @param c Character code to send
   */
  void send(uint8_t c);

  /**
   * @brief Like localWrite(), but also sends the data to the serial port, if connected
   *
   * @param str String to send
   */
  void send(char const * str);


  /**
   * @brief Gets the associated keyboard object.
   *
   * @return The Keyboard object.
   */
  Keyboard * keyboard() { return m_keyboard; }

  /**
   * @brief Gets the associated canvas object.
   *
   * @return The Canvas object.
   */
  Canvas * canvas() { return m_canvas; }

  /**
   * @brief Activates this terminal for input and output.
   *
   * Only one terminal at a time can be active for input and output.
   * Activating a terminal deactivates the currently active one.
   *
   * @param transition Optional transition effect
   */
  void activate(TerminalTransition transition = TerminalTransition::None);

  /**
   * @brief Deactivates this terminal.
   */
  void deactivate();

  /**
   * @brief Determines whether this terminal is active.
   *
   * @return True if this terminal is active for input and output.
   */
  bool isActive() { return s_activeTerminal == this; }

  /**
   * @brief Selects a color for the specified attribute
   *
   * Specifies the color used when the terminal prints a character with the given attribute.
   * If a character has multiple attributes, the resulting color is undefined.
   * To disable the attribute color, call the other setColorForAttribute() overload.
   *
   * @param attribute Style/attribute whose color is set. Only CharStyle::Bold, CharStyle::ReducedLuminosity, CharStyle::Italic and CharStyle::Underline are supported
   * @param color Color of the attribute
   * @param maintainStyle If true, the style is applied as well; if false, only the specified color is applied.
   */
  void setColorForAttribute(CharStyle attribute, Color color, bool maintainStyle);

  /**
   * @brief Disables the color for the specified attribute
   *
   * Removes the color previously associated with the specified attribute.
   *
   * @param attribute Style/attribute whose color is disabled. Only CharStyle::Bold, CharStyle::ReducedLuminosity, CharStyle::Italic and CharStyle::Underline are supported
   */
  void setColorForAttribute(CharStyle attribute);

  /**
   * @brief Gets the embedded sound generator
   *
   * @return SoundGenerator object
   */
  SoundGenerator * soundGenerator();



  //// Delegates ////

  /**
   * @brief Delegate called whenever a new virtual key is received from the keyboard
   *
   * The first parameter is a pointer to the decoded virtual key.
   * The second parameter is true when the key is pressed and false when it is released.
   */
  Delegate<VirtualKey *, bool> onVirtualKey;


  /**
   * @brief Delegate called whenever a new virtual key is received from the keyboard, including shift states
   *
   * The parameter is a pointer to the decoded virtual key information.
   */
  Delegate<VirtualKeyItem *> onVirtualKeyItem;


  /**
   * @brief Delegate called whenever a new user sequence has been received
   *
   * The parameter is a zero-terminated string containing the user sequence payload.
   * A user sequence starts with ESC + "_#" and ends with "$". For example, when the terminal
   * receives "ESC_#mycommand$", the delegate receives the string "mycommand".
   */
  Delegate<char const *> onUserSequence;
  
  
  /**
   * @brief Delegate called whenever the terminal needs to send a character
   *
   * The parameter contains the character to send.
   */
  Delegate<uint8_t> onSend;
  
  
  /**
   * @brief Delegate called whenever the terminal receives a character
   *
   * The parameter contains the received character.
   */
  Delegate<uint8_t> onReceive;
  
  /**
   * @brief Delegate called whenever the terminal is ready to send
   *
   * The parameter is a reference to a bool, which the delegate can set to false if the receiver is not ready.
   */
  Delegate<bool *> onReadyToSend;



  // statics (used for common default properties)


  /**
   * @brief Number of characters the terminal can accept without blocking (increase it if characters are lost on the serial port).
   *
   * Applications must change this value before calling begin().
   *
   * Default value is FABGLIB_DEFAULT_TERMINAL_INPUT_QUEUE_SIZE (defined in fabglconf.h)
   */
  static int inputQueueSize;

  /**
   * @brief Stack size of the task that processes the Terminal input stream.
   *
   * Applications must change this value before calling begin().
   *
   * Default value is FABGLIB_DEFAULT_TERMINAL_INPUT_CONSUMER_TASK_STACK_SIZE (defined in fabglconf.h)
   */
  static int inputConsumerTaskStackSize;

  /**
   * @brief Stack size of the task that reads keys from the keyboard and sends the corresponding ANSI/VT codes to the Terminal output stream.
   *
   * Applications must change this value before calling begin().
   *
   * Default value is FABGLIB_DEFAULT_TERMINAL_KEYBOARD_READER_TASK_STACK_SIZE (defined in fabglconf.h)
   */
  static int keyboardReaderTaskStackSize;


private:

  void reset();
  void int_clear();
  void clearMap(uint32_t * map);

  void freeFont();
  void freeTabStops();
  void freeGlyphsMap();

  void set132ColumnMode(bool value);

  bool moveUp();
  bool moveDown();
  void move(int offset);
  void setCursorPos(int X, int Y);
  int getAbsoluteRow(int Y);

  void int_setBackgroundColor(Color color);
  void int_setForegroundColor(Color color);

  void syncDisplayController();

  // tab stops
  void nextTabStop();
  void setTabStop(int column, bool set);
  void resetTabStops();

  // scroll control
  void scrollDown();
  void scrollDownAt(int startingRow);
  void scrollUp();
  void scrollUpAt(int startingRow);
  void setScrollingRegion(int top, int down, bool resetCursorPos = true);
  void updateCanvasScrollingRegion();

  // multilevel save/restore cursor state
  void saveCursorState();
  void restoreCursorState();
  void clearSavedCursorStates();

  void erase(int X1, int Y1, int X2, int Y2, uint8_t c, bool maintainDoubleWidth, bool selective);

  void consumeInputQueue();
  void consumeESC();
  void consumeCSI();
  void consumeOSC();
  void consumeFabGLSeq();
  void consumeFabGLGraphicsSeq();
  void consumeCSIQUOT(int * params, int paramsCount);
  void consumeCSISPC(int * params, int paramsCount);
  uint8_t consumeParamsAndGetCode(int * params, int * paramsCount, bool * questionMarkFound);
  void consumeDECPrivateModes(int const * params, int paramsCount, uint8_t c);
  void consumeDCS();
  void execSGRParameters(int const * params, int paramsCount);
  void consumeESCVT52();

  void execCtrlCode(uint8_t c);

  static void charsConsumerTask(void * pvParameters);
  static void keyboardReaderTask(void * pvParameters);

  static void blinkTimerFunc(TimerHandle_t xTimer);
  void blinkText();
  bool enableBlinkingText(bool value);
  void blinkCursor();
  bool int_enableCursor(bool value);

  uint8_t getNextCode(bool processCtrlCodes);

  bool setChar(uint8_t c);
  GlyphOptions getGlyphOptionsAt(int X, int Y);

  void insertAt(int column, int row, int count);
  void deleteAt(int column, int row, int count);

  bool multilineInsertChar(int charsToMove);
  void multilineDeleteChar(int charsToMove);

  void reverseVideo(bool value);

  void refresh();
  void refresh(int X, int Y);
  void refresh(int X1, int Y1, int X2, int Y2);

  void setLineDoubleWidth(int row, int value);
  int getCharWidthAt(int row);
  int getColumnsAt(int row);

  void useAlternateScreenBuffer(bool value);

  void sendCSI();
  void sendDCS();
  void sendSS3();
  void sendCursorKeyCode(uint8_t c);
  void sendKeypadCursorKeyCode(uint8_t applicationCode, const char * numericCode);

  void ANSIDecodeVirtualKey(VirtualKeyItem const & item);
  void VT52DecodeVirtualKey(VirtualKeyItem const & item);

  void convHandleTranslation(uint8_t c, bool fromISR);
  void convSendCtrl(ConvCtrl ctrl, bool fromISR);
  void convQueue(const char * str, bool fromISR);
  void TermDecodeVirtualKey(VirtualKeyItem const & item);

  bool addToInputQueue(uint8_t c, bool fromISR);
  bool insertToInputQueue(uint8_t c, bool fromISR);

  void enableFabGLSequences(bool value);

  void int_setTerminalType(TermType value);
  void int_setTerminalType(TermInfo const * value);

  void sound(int waveform, int frequency, int duration, int volume);

  uint8_t extGetByteParam();
  int extGetIntParam();
  void extGetCmdParam(char * cmd);

  void freeSprites();

  uint32_t makeGlyphItem(uint8_t c, GlyphOptions * glyphOptions, Color * newForegroundColor);
  
  // indicates which is the active terminal when there are multiple instances of Terminal
  static Terminal *  s_activeTerminal;


  BaseDisplayController * m_displayController;
  Canvas *           m_canvas;
  bool               m_bitmappedDisplayController;  // true = bitmapped, false = textual

  Keyboard *         m_keyboard;

  Stream *           m_logStream;

  // characters, characters attributes and characters colors container
  // you may also call this the "text screen buffer"
  GlyphsBuffer       m_glyphsBuffer;

  // used to implement alternate screen buffer
  uint32_t *         m_alternateMap;

  // true when m_alternateMap and m_glyphBuffer.map has been swapped
  bool               m_alternateScreenBuffer;

  // just to restore some properties when swapping screens (alternate screen)
  int                m_alternateCursorX;
  int                m_alternateCursorY;
  int                m_alternateScrollingRegionTop;
  int                m_alternateScrollingRegionDown;
  bool               m_alternateCursorBlinkingEnabled;

  FontInfo           m_font;

  PaintOptions       m_paintOptions;
  GlyphOptions       m_glyphOptions;

  EmuState           m_emuState;

  Color              m_defaultForegroundColor;
  Color              m_defaultBackgroundColor;

  // states of cursor and blinking text before consumeInputQueue()
  bool               m_prevCursorEnabled;
  bool               m_prevBlinkingTextEnabled;

  // task that reads and processes incoming characters
  TaskHandle_t       m_charsConsumerTaskHandle;

  // task that reads keyboard input and send ANSI/VT100 codes to serial port
  TaskHandle_t       m_keyboardReaderTaskHandle;

  // true = cursor in reverse state (visible), false = cursor invisible
  volatile bool      m_cursorState;

  // timer used to blink
  TimerHandle_t      m_blinkTimer;

  // main terminal mutex
  volatile SemaphoreHandle_t m_mutex;

  volatile bool      m_blinkingTextVisible;    // true = blinking text is currently visible
  volatile bool      m_blinkingTextEnabled;

  volatile int       m_columns;
  volatile int       m_rows;

  // checked in loadFont() to limit m_columns and m_rows (-1 = not checked)
  int                m_maxColumns;
  int                m_maxRows;

  // contains characters to be processed (from write() calls)
  volatile QueueHandle_t    m_inputQueue;

  // contains characters received and decoded from keyboard (or as replies from ANSI-VT queries)
  QueueHandle_t             m_outputQueue;

  // linked list that contains saved cursor states (first item is the last added)
  TerminalCursorState *     m_savedCursorStateList;

  // a reset has been requested
  bool                      m_resetRequested;

  // used to implement m_emuState.keyAutorepeat
  VirtualKey                m_lastPressedKey;

  uint8_t                   m_convMatchedCount;
  uint8_t                   m_convMatchedChars[EmuTerminalMaxChars];
  TermInfoVideoConv const * m_convMatchedItem;
  TermInfo const *          m_termInfo;

  // last char added with write()
  volatile uint8_t          m_lastWrittenChar;

  // when a FabGL sequence has been detected in write()
  volatile bool             m_writeDetectedFabGLSeq;

  // used by extGetIntParam(), extGetCmdParam(), extGetByteParam() to store next item (to avoid insertToInputQueue() which can cause dead-locks)
  int                       m_extNextCode; // -1 = no code

  SoundGenerator *          m_soundGenerator;

  Sprite *                  m_sprites;
  int                       m_spritesCount;

  bool                      m_coloredAttributesMaintainStyle;
  int                       m_coloredAttributesMask;    // related bit 1 if enabled
  Color                     m_coloredAttributesColor[4];
  
  // true if inside ::end()
  volatile bool             m_endingState;

};



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
// TerminalController


/**
 * @brief TerminalController controls a Terminal object directly, without the application having to send escape sequences
 *
 * TerminalController requires FabGL-specific sequences to be enabled (the default) and works
 * regardless of the selected terminal emulation.
 *
 * Example:
 *
 *     // Write "Hello" at 10, 10
 *     TerminalController termctrl(&Terminal);
 *     termctrl.setCursorPos(10, 10);
 *     Terminal.write("Hello");
 *
 * @ingroup terminal
 */
class TerminalController {

public:

  /**
   * @brief Object constructor
   *
   * @param terminal The Terminal instance to control. If not specified, the read and write delegates must be set.
   */
  TerminalController(Terminal * terminal = nullptr);

  ~TerminalController();

  /**
   * @brief Clears the screen
   */
  void clear();

  /**
   * @brief Sets the terminal to control
   *
   * @param terminal The Terminal instance to control. If not specified, the read and write delegates must be set.
   */
  void setTerminal(Terminal * terminal = nullptr);

  /**
   * @brief Enables or disables the cursor
   *
   * @param value True to enable the cursor, false to disable it
   */
  void enableCursor(bool value);

  /**
   * @brief Sets the current cursor position
   *
   * @param col Cursor column (1 = left-most position).
   * @param row Cursor row (1 = top-most position).
   */
  void setCursorPos(int col, int row);

  /**
   * @brief Gets the current cursor position
   *
   * @param col Pointer to a variable that receives the current cursor column (1 = left-most position).
   * @param row Pointer to a variable that receives the current cursor row (1 = top-most position).
   *
   * Example:
   *
   *     int col, row;
   *     termctrl.getCursorPos(&col, &row);
   */
  void getCursorPos(int * col, int * row);

  /**
   * @brief Moves the cursor to the left
   *
   * The cursor may move across lines.
   *
   * @param count Number of positions to move.
   */
  void cursorLeft(int count);

  /**
   * @brief Moves the cursor to the right
   *
   * The cursor may move across lines.
   *
   * @param count Number of positions to move.
   */
  void cursorRight(int count);

  /**
   * @brief Gets the current cursor column
   *
   * @return Cursor column (1 = left-most position).
   */
  int getCursorCol();

  /**
   * @brief Gets the current cursor row
   *
   * @return Cursor row (1 = top-most position).
   */
  int getCursorRow();

  /**
   * @brief Inserts a blank character, shifting the specified number of characters to the right
   *
   * Shifted characters may wrap across multiple lines.
   *
   * @param charsToMove Number of characters to shift to the right, starting from the current cursor position.
   *
   * @return True if the screen scrolled vertically.
   */
  bool multilineInsertChar(int charsToMove);

  /**
   * @brief Deletes a character, shifting the specified number of characters to the left
   *
   * Shifted characters may wrap across multiple lines.
   *
   * @param charsToMove Number of characters to shift to the left, starting from the current cursor position.
   */
  void multilineDeleteChar(int charsToMove);

  /**
   * @brief Writes a raw character at the current cursor position
   *
   * The cursor then advances by one position to the right.
   *
   * @param c Raw character code. It is not interpreted as a control character or escape sequence.
   *
   * @return True if the screen scrolled vertically.
   */
  bool setChar(uint8_t c);

  /**
   * @brief Checks if a virtual key is currently down
   *
   * @param vk Virtual key code
   *
   * @return True if the specified key is pressed
   */
  bool isVKDown(VirtualKey vk);

  /**
   * @brief Disables FabGL-specific sequences
   */
  void disableFabGLSequences();

  /**
   * @brief Sets the terminal type to emulate
   *
   * @param value A terminal to emulate
   */
  void setTerminalType(TermType value);

  /**
   * @brief Sets the foreground color
   *
   * @param value Foreground color
   */
  void setForegroundColor(Color value);

  /**
   * @brief Sets the background color
   *
   * @param value Background color
   */
  void setBackgroundColor(Color value);

  /**
   * @brief Enables or disables the specified character style
   *
   * @param style Style to enable or disable
   * @param enabled If true, the style is enabled, if false the style is disabled
   */
  void setCharStyle(CharStyle style, bool enabled);


  //// Delegates ////

  /**
   * @brief Read character delegate
   *
   * This delegate is called whenever a character must be read and no Terminal has been specified.
   * The delegate must block until a character is received.
   *
   * The first parameter is a pointer to the variable that receives the character code.
   */
  Delegate<int *> onRead;

  /**
   * @brief Write character delegate
   *
   * This delegate is called whenever a character must be written and no Terminal has been specified.
   *
   * The first parameter is the character code to send.
   */
  Delegate<int> onWrite;


private:

  void waitFor(int value);
  void write(uint8_t c);
  void write(char const * str);
  int read();


  Terminal * m_terminal;
};



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
// LineEditor


/** \ingroup Enumerations
 * @brief Special characters reported by the LineEditor::onSpecialChar delegate
 */
enum class LineEditorSpecialChar {
  CursorUp,    /**< Cursor Up */
  CursorDown,  /**< Cursor Down */
};


/**
 * @brief LineEditor is a single-line editor, spanning multiple rows if needed, that uses a Terminal object for input and output.
 *
 * The editor supports the following control keys:
 *
 * \li <b>Left and Right Arrow keys</b>: move the cursor left and right, even across rows
 * \li <b>CTRL + Left and Right Arrow keys</b>: move the cursor to the beginning of the previous or next word
 * \li <b>Home key</b>: move the cursor to the beginning of the line
 * \li <b>End key</b>: move the cursor to the end of the line
 * \li <b>Delete key</b>: delete the character under the cursor
 * \li <b>Insert key</b>: toggle insert mode
 * \li <b>Backspace key</b>: delete the character to the left of the cursor
 * \li <b>Enter/Return</b>: move the cursor to the beginning of the next line and exit the editor
 *
 * Example:
 *
 *     Terminal.write("> ");  // show prompt
 *     LineEditor ed(&Terminal);
 *     char const * txt = ed.edit();
 *     Terminal.printf("Your input is: %s\r\n", txt);
 *
 * @ingroup terminal
 */
class LineEditor {

public:

  /**
   * @brief Object constructor
   *
   * @param terminal Pointer to the Terminal object
   */
  LineEditor(Terminal * terminal);

  ~LineEditor();

  /**
   * @brief Sets initial text
   *
   * Call this method to pre-fill the input with some text.
   * It can also be called while editing to replace the current text.
   *
   * @param text Initial text.
   * @param moveCursor If true, the cursor is moved to the end of the initial text.
   *
   * Example:
   * 
   *     LineEditor ed(&Terminal);
   *     ed.setText("Initial text ");
   *     char const * txt = ed.edit();
   *     Terminal.printf("Your input is: %s\r\n", txt);
   */
  void setText(char const * text, bool moveCursor = true);

  /**
   * @brief Sets the initial text, specifying its length
   *
   * Call this method to pre-fill the input with some text.
   * It can also be called while editing to replace the current text.
   *
   * @param text Initial text.
   * @param length Text length.
   * @param moveCursor If true, the cursor is moved to the end of the initial text.
   */
  void setText(char const * text, int length, bool moveCursor = true);

  /**
   * @brief Simulates user typing
   *
   * Unlike setText(), this method accepts control characters and generates onChar events.
   *
   * @param text Text to type
   */
  void typeText(char const * text);

  /**
   * @brief Reads user input and returns the entered line
   *
   * This method returns when the user presses ENTER/RETURN.
   *
   * @param maxLength Maximum number of characters the user can type. 0 = unlimited.
   *
   * @return The text typed and edited by the user.
   */
  char const * edit(int maxLength = 0);

  /**
   * @brief Gets the current content
   *
   * @return The text typed by the user.
   */
  char const * get() { return m_text; }

  /**
   * @brief Sets the insert mode
   *
   * @param value True to enable insert mode (default), false to disable it.
   */
  void setInsertMode(bool value) { m_insertMode = value; }


  // delegates

  /**
   * @brief Read character delegate
   *
   * This delegate is called whenever a character must be read and no Terminal has been specified.
   * The delegate must block until a character is received.
   *
   * The first parameter is a pointer to the variable that receives the character code.
   */
  Delegate<int *> onRead;

  /**
   * @brief Write character delegate
   *
   * This delegate is called whenever a character must be written and no Terminal has been specified.
   *
   * The first parameter is the character code to send.
   */
  Delegate<int> onWrite;

  /**
   * @brief Delegate called whenever a character has been received
   *
   * The first parameter is a pointer to the received character code.
   */
  Delegate<int *>  onChar;

  /**
   * @brief Delegate called whenever ENTER (carriage return) has been pressed
   *
   * The delegate sets the first parameter to the action to perform:
   *
   *    0 = new line and end editing (default)
   *    1 = end editing
   *    2 = continue editing
   */
  Delegate<int *> onCarriageReturn;

  /**
   * @brief Delegate called whenever a special key has been pressed
   *
   * The first parameter receives the special key, of type LineEditorSpecialChar.
   */
  Delegate<LineEditorSpecialChar> onSpecialChar;


private:

  void beginInput();
  void endInput();
  void setLength(int newLength);

  void write(uint8_t c);
  int read();

  void performCursorUp();
  void performCursorDown();
  void performCursorLeft();
  void performCursorRight();
  void performCursorHome();
  void performCursorEnd();
  void performDeleteRight();
  void performDeleteLeft();


  Terminal *          m_terminal;
  TerminalController  m_termctrl;
  char *              m_text;
  int                 m_textLength;
  int                 m_allocated;
  int16_t             m_inputPos;
  int16_t             m_state;     // -1 = begin input, 0 = normal input, 1 = ESC, 2 = CTRL-Q, >=31 = CSI (actual value specifies the third char if present)
  int16_t             m_homeCol;
  int16_t             m_homeRow;
  bool                m_insertMode;
  char *              m_typeText;
  int                 m_typingIndex;
};



} // end of namespace

