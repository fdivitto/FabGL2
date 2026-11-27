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


/*
 * Optional SD Card connections:
 *   MISO => GPIO 16  (2 for PICO-D4)
 *   MOSI => GPIO 17  (12 for PICO-D4)
 *   CLK  => GPIO 14
 *   CS   => GPIO 13
 *
 * To change above assignment fill other paramaters of FileBrowser::mountSDCard().
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "fabgl.h"
#include "fabutils.h"
#include "network/netutils.h"

#include "src/machine.h"



// embedded (free) games
#include "progs/embedded/blue_star.h"
#include "progs/embedded/omega_fury.h"
#include "progs/embedded/quikman.h"
#include "progs/embedded/splatform.h"
#include "progs/embedded/demo_birthday.h"
#include "progs/embedded/arukanoido.h"
#include "progs/embedded/demo_bah_bah_final.h"
#include "progs/embedded/demo_digit.h"
#include "progs/embedded/dragonwing.h"
#include "progs/embedded/kikstart.h"
#include "progs/embedded/kweepoutmc.h"
#include "progs/embedded/nibbler.h"
#include "progs/embedded/pooyan.h"
#include "progs/embedded/popeye.h"
#include "progs/embedded/pulse.h"
#include "progs/embedded/spikes.h"
#include "progs/embedded/tank_battalion.h"
#include "progs/embedded/tetris_plus.h"
#include "progs/embedded/astro-panic.h"
#include "progs/embedded/frogger07.h"
#include "progs/embedded/froggie.h"
#include "progs/embedded/tammerfors.h"

// from the web
#include "progs/weblist.h"


// Flash and SDCard configuration
#define FORMAT_ON_FAIL     true
#define SPIFFS_MOUNT_PATH  "/flash"
#define SDCARD_MOUNT_PATH  "/SD"


// base path (can be SPIFFS_MOUNT_PATH or SDCARD_MOUNT_PATH depending from what was successfully mounted first)
char const * basepath = nullptr;


// name of embedded programs directory
char const * EMBDIR  = "VIC20Store";

// name of downloaded programs directory
char const * DOWNDIR = "VIC20Store";

// name of user directory (where LOAD and SAVE works)
char const * USERDIR = "VIC20Store";    // same of EMBDIR


struct EmbeddedProgDef {
  char const *    filename;
  uint8_t const * data;
  int             size;
};


// Embedded programs
//   To convert a PRG to header file use...
//         xxd -i myprogram.prg >myprogram.h
//   ...modify like files in progs/embedded, and put filename and binary data in following list.
const EmbeddedProgDef embeddedProgs[] = {
  { arukanoido_filename, arukanoido_prg, sizeof(arukanoido_prg) },
  { nibbler_filename, nibbler_prg, sizeof(nibbler_prg) },
  { pooyan_filename, pooyan_prg, sizeof(pooyan_prg) },
  { popeye_filename, popeye_prg, sizeof(popeye_prg) },
  { tetris_plus_filename, tetris_plus_prg, sizeof(tetris_plus_prg) },
  { spikes_filename, spikes_prg, sizeof(spikes_prg) },
  { astro_panic_filename, astro_panic_prg, sizeof(astro_panic_prg) },
  { splatform_filename, splatform_prg, sizeof(splatform_prg) },
  { kweepoutmc_filename, kweepoutmc_prg, sizeof(kweepoutmc_prg) },
  { dragonwing_filename, dragonwing_prg, sizeof(dragonwing_prg) },
  { tank_battalion_filename, tank_battalion_prg, sizeof(tank_battalion_prg) },
  { blue_star_filename, blue_star_prg, sizeof(blue_star_prg) },
  { quikman_filename, quikman_prg, sizeof(quikman_prg) },
  { frogger07_filename, frogger07_prg, sizeof(frogger07_prg) },
  { tammerfors_filename, tammerfors_prg, sizeof(tammerfors_prg) },
  { pulse_filename, pulse_prg, sizeof(pulse_prg) },
  { omega_fury_filename, omega_fury_prg, sizeof(omega_fury_prg) },
};


#if DEBUGMACHINE
static volatile int  scycles = 0;
static volatile bool restart = false;
void vTimerCallback1SecExpired(xTimerHandle pxTimer)
{
  printf("%d - ", scycles / 5);
  printf("Free memory (total, min, largest): %d, %d, %d\n", heap_caps_get_free_size(0), heap_caps_get_minimum_free_size(0), heap_caps_get_largest_free_block(0));
  restart = true;
}
#endif


fabgl::VGAController DisplayController;
fabgl::PS2Controller PS2Controller;


// where to store WiFi info, etc...
Preferences preferences;
fabgl::WiFiConnection wifi;


// copies embedded programs into SPIFFS/SDCard
void copyEmbeddedPrograms()
{
  FileBrowser dir(basepath);
  if (!dir.exists(EMBDIR, false)) {
    // there isn't a EMBDIR folder, create and populate it
    dir.makeDirectory(EMBDIR);
    dir.changeDirectory(EMBDIR);
    for (int i = 0; i < sizeof(embeddedProgs) / sizeof(EmbeddedProgDef); ++i) {
      int fullpathlen = dir.getFullPath(embeddedProgs[i].filename);
      char fullpath[fullpathlen];
      dir.getFullPath(embeddedProgs[i].filename, fullpath, fullpathlen);
      FILE * f = fopen(fullpath, "wb");
      if (f) {
        fwrite(embeddedProgs[i].data, 1, embeddedProgs[i].size, f);
        fclose(f);
      }
    }
  }
}


using fabgl::uiWindow;
using fabgl::uiObject;
using fabgl::uiStyle;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Theme
//
// The display has 64 colors (VGAController): each RGB channel has four levels (0, 85, 170, 255).


namespace Theme {
  static const RGB888 Background(  0,   0,   0);
  static const RGB888 Panel     (  0,   0,  85);
  static const RGB888 Edge      (  0,  85, 170);
  static const RGB888 Text      (170, 170, 255);
  static const RGB888 Dim       ( 85,  85, 170);
  static const RGB888 White     (255, 255, 255);
  static const RGB888 Accent    (  0, 255, 255);   // the cyan of the VIC-20 screen border
  static const RGB888 DarkAccent(  0,  85,  85);
  static const RGB888 Folder    (255, 170,   0);
  static const RGB888 Cartridge (255,  85,   0);
  static const RGB888 Run       (  0, 170,  85);
  static const RGB888 Danger    (170,   0,   0);

  // the stripes of the VIC-20 badge
  static const RGB888 Rainbow[5] = { RGB888(255, 0, 0), RGB888(255, 85, 0), RGB888(255, 255, 0), RGB888(0, 170, 0), RGB888(0, 85, 255) };
}


static fabgl::FontInfo const * const SmallFont  = &fabgl::FONT_std_12;
static fabgl::FontInfo const * const MediumFont = &fabgl::FONT_std_14;
static fabgl::FontInfo const * const TitleFont  = &fabgl::FONT_std_24;
static fabgl::FontInfo const * const TinyFont   = &fabgl::FONT_6x8;


// style classes (last parameter of the control constructors)
enum StyleClass {
  STYLE_NONE,
  STYLE_PROGRAMS,       // program list
  STYLE_TILES,          // RAM expansion and joystick selectors
  STYLE_STEPPER,        // keyboard layout
  STYLE_BUTTON,
  STYLE_BUTTON_RUN,
  STYLE_BUTTON_DANGER,
  STYLE_VALUE,          // accent text on a panel, right aligned
  STYLE_VALUE_LEFT,     // accent text on a panel
  STYLE_DIM_VALUE,      // dim text on a panel, right aligned
  STYLE_STORAGE,        // used space bar
  STYLE_DIALOG,         // dialog frames (message boxes too)
  STYLE_DIALOG_LABEL,
  STYLE_DIALOG_PANEL,   // panel with the message box buttons
  STYLE_EDIT,
  STYLE_PROGRESS,       // download progress bar
};


// keyboard focus: a cyan border. The border is always there (invisible when not focused), so the content doesn't move
static void setFocusBorder(uiWindow * window, RGB888 background)
{
  auto & s = window->windowStyle();
  s.borderSize         = 1;
  s.focusedBorderSize  = 1;
  s.borderColor        = background;
  s.activeBorderColor  = background;
  s.focusedBorderColor = Theme::Accent;
}


struct VIC20Style : uiStyle {

  void setStyle(uiObject * object, uint32_t styleClassID)
  {
    switch (styleClassID) {

      case STYLE_PROGRAMS:
      {
        auto list = (uiFileBrowser *) object;
        list->windowStyle().borderSize         = 1;
        list->windowStyle().focusedBorderSize  = 1;
        list->windowStyle().borderColor        = Theme::Edge;
        list->windowStyle().activeBorderColor  = Theme::Edge;
        list->windowStyle().focusedBorderColor = Theme::Accent;
        auto & s = list->listBoxStyle();
        s.backgroundColor                = Theme::Background;
        s.focusedBackgroundColor         = Theme::Background;
        s.selectedBackgroundColor        = Theme::Edge;
        s.focusedSelectedBackgroundColor = Theme::Accent;
        s.itemHeight                     = 16;
        s.textFont                       = SmallFont;
        s.textColor                      = Theme::Text;
        s.selectedTextColor              = Theme::White;
        auto & sb = list->scrollableControlStyle();
        sb.scrollBarBackgroundColor          = Theme::Panel;
        sb.scrollBarForegroundColor          = Theme::Edge;
        sb.mouseOverScrollBarForegroundColor = Theme::Accent;
        sb.scrollBarSize                     = 8;
        break;
      }

      case STYLE_TILES:
      {
        auto tiles = (uiTileSelector *) object;
        setFocusBorder(tiles, Theme::Panel);
        tiles->tileSelectorProps().tileGap = 3;
        auto & s = tiles->tileSelectorStyle();
        s.backgroundColor         = Theme::Panel;
        s.tileBackgroundColor     = Theme::Background;
        s.tileBorderColor         = Theme::Edge;
        s.selectedTileBorderColor = Theme::Accent;
        s.textFont                = SmallFont;
        s.textColor               = Theme::Dim;
        s.selectedTextColor       = Theme::Accent;
        break;
      }

      case STYLE_STEPPER:
      {
        auto stepper = (uiStepper *) object;
        setFocusBorder(stepper, Theme::Panel);
        auto & s = stepper->stepperStyle();
        s.backgroundColor                = Theme::Background;
        s.buttonBackgroundColor          = Theme::Panel;
        s.mouseDownButtonBackgroundColor = Theme::Accent;
        s.buttonBorderColor              = Theme::Edge;
        s.arrowColor                     = Theme::Text;
        s.mouseDownArrowColor            = Theme::Panel;
        s.textFont                       = MediumFont;
        s.textColor                      = Theme::Accent;
        break;
      }

      case STYLE_BUTTON:
      case STYLE_BUTTON_RUN:
      case STYLE_BUTTON_DANGER:
      {
        auto button = (uiButton *) object;
        bool run    = styleClassID == STYLE_BUTTON_RUN;
        button->windowStyle().borderSize         = 1;
        button->windowStyle().focusedBorderSize  = 1;
        button->windowStyle().borderColor        = run ? Theme::Run : Theme::Edge;
        button->windowStyle().activeBorderColor  = button->windowStyle().borderColor;
        button->windowStyle().focusedBorderColor = Theme::Accent;
        auto & s = button->buttonStyle();
        s.backgroundColor          = run ? Theme::Run : Theme::Panel;
        s.mouseOverBackgroundColor = run ? RGB888(0, 255, 85) : Theme::Edge;
        s.mouseDownBackgroundColor = styleClassID == STYLE_BUTTON_DANGER ? Theme::Danger : Theme::Accent;
        s.downBackgroundColor      = Theme::Accent;
        s.textColor                = run ? Theme::White : Theme::Text;
        s.mouseOverTextColor       = run ? Theme::Panel : Theme::White;
        s.downTextColor            = Theme::Panel;
        s.textFont                 = SmallFont;
        break;
      }

      case STYLE_VALUE:
      case STYLE_VALUE_LEFT:
      case STYLE_DIM_VALUE:
      {
        auto & s = ((uiLabel *) object)->labelStyle();
        s.backgroundColor = Theme::Panel;
        s.textFont        = SmallFont;
        s.textColor       = styleClassID == STYLE_DIM_VALUE ? Theme::Dim : Theme::Accent;
        s.textAlign       = styleClassID == STYLE_VALUE_LEFT ? uiHAlign::Left : uiHAlign::Right;
        break;
      }

      case STYLE_STORAGE:
      case STYLE_PROGRESS:
      {
        auto bar = (uiProgressBar *) object;
        bar->windowStyle().borderSize  = 1;
        bar->windowStyle().borderColor = Theme::Edge;
        bar->progressBarProps().showPercentage = styleClassID == STYLE_PROGRESS;
        auto & s = bar->progressBarStyle();
        s.backgroundColor = Theme::Background;
        s.foregroundColor = styleClassID == STYLE_PROGRESS ? Theme::Edge : Theme::Accent;
        s.textFont        = SmallFont;
        s.textColor       = Theme::White;
        break;
      }

      case STYLE_DIALOG:
      {
        auto frame = (uiFrame *) object;
        frame->windowStyle().borderSize         = 1;
        frame->windowStyle().borderColor        = Theme::Accent;
        frame->windowStyle().activeBorderColor  = Theme::Accent;
        frame->windowStyle().focusedBorderColor = Theme::Accent;
        auto & s = frame->frameStyle();
        s.backgroundColor                = Theme::Panel;
        s.titleBackgroundColor           = Theme::Edge;
        s.activeTitleBackgroundColor     = Theme::Edge;
        s.titleColor                     = Theme::White;
        s.activeTitleColor               = Theme::White;
        s.titleFont                      = SmallFont;
        s.buttonColor                    = Theme::Text;
        s.activeButtonColor              = Theme::White;
        s.mouseOverBackgroundButtonColor = Theme::Accent;
        s.mouseOverButtonColor           = Theme::Panel;
        break;
      }

      case STYLE_DIALOG_LABEL:
      {
        auto & s = ((uiStaticLabel *) object)->labelStyle();
        s.backgroundColor = Theme::Panel;
        s.textColor       = Theme::Text;
        s.textFont        = MediumFont;   // message boxes measure the text with this font
        break;
      }

      case STYLE_DIALOG_PANEL:
      {
        auto panel = (uiPanel *) object;
        panel->windowStyle().borderSize  = 1;
        panel->windowStyle().borderColor = Theme::Edge;
        panel->panelStyle().backgroundColor = Theme::Panel;
        break;
      }

      case STYLE_EDIT:
      {
        auto edit = (uiTextEdit *) object;
        edit->windowStyle().borderSize         = 1;
        edit->windowStyle().focusedBorderSize  = 1;
        edit->windowStyle().borderColor        = Theme::Edge;
        edit->windowStyle().activeBorderColor  = Theme::Edge;
        edit->windowStyle().focusedBorderColor = Theme::Accent;
        auto & s = edit->textEditStyle();
        s.backgroundColor          = Theme::Background;
        s.mouseOverBackgroundColor = Theme::Background;
        s.focusedBackgroundColor   = Theme::Background;
        s.textColor                = Theme::Accent;
        s.textFont                 = SmallFont;
        break;
      }

      default:
        break;
    }
  }

} vic20Style;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Drawing helpers


enum class Align { Left, Center, Right };

// x is the left side, the center or the right side of the text, depending on "align"
static void drawText(Canvas * cv, fabgl::FontInfo const * font, RGB888 color, int x, int y, char const * text, Align align = Align::Left, bool bold = false)
{
  int width = cv->textExtent(font, text) + (bold ? strlen(text) : 0);
  if (align == Align::Center)
    x -= width / 2;
  else if (align == Align::Right)
    x -= width;
  cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(bold));
  cv->setPenColor(color);
  cv->drawText(font, x, y, text);
  cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false));
}

static void fillRect(Canvas * cv, RGB888 color, Rect const & rect)
{
  cv->setBrushColor(color);
  cv->fillRectangle(rect);
}

// a rectangle with cut corners
static void drawRoundRect(Canvas * cv, RGB888 fill, RGB888 border, RGB888 outside, Rect const & r)
{
  fillRect(cv, fill, r);
  cv->setPenColor(border);
  cv->drawRectangle(r);
  cv->setPenColor(outside);
  cv->setPixel(r.X1, r.Y1);
  cv->setPixel(r.X2, r.Y1);
  cv->setPixel(r.X1, r.Y2);
  cv->setPixel(r.X2, r.Y2);
}

// a section title: a small square and a bold text
static void drawHeading(Canvas * cv, int x, int y, char const * title, RGB888 color = Theme::Text)
{
  fillRect(cv, Theme::Accent, Rect(x, y + 4, x + 3, y + 7));
  drawText(cv, SmallFont, color, x + 9, y, title, Align::Left, true);
}

// a panel with rounded corners and an optional title
static void drawPanel(Canvas * cv, Rect const & r, char const * title)
{
  drawRoundRect(cv, Theme::Panel, Theme::Edge, Theme::Background, r);
  if (title)
    drawHeading(cv, r.X1 + 6, r.Y1 + 4, title);
}

// the five diagonal stripes of the VIC-20 badge, "height" pixels high
static void drawStripes(Canvas * cv, int x, int y, int height)
{
  int slant = height / 3;
  for (int i = 0; i < 5; ++i) {
    int   sx = x + i * 7;
    Point p[4] = { Point(sx + slant, y), Point(sx + slant + 4, y), Point(sx + 4, y + height - 1), Point(sx, y + height - 1) };
    cv->setBrushColor(Theme::Rainbow[i]);
    cv->fillPath(p, 4);
  }
}

// a key cap with the text inside (6x8 font), returns its width
static int drawKeyCap(Canvas * cv, int x, int y, char const * text)
{
  int w = strlen(text) * 6 + 7;
  drawRoundRect(cv, Theme::Background, Theme::Edge, Theme::Panel, Rect(x, y, x + w - 1, y + 11));
  drawText(cv, TinyFont, Theme::White, x + 4, y + 2, text);
  return w;
}

// key caps separated by '|' ("L-ALT|A S W Z"), with a "+" between them
static void drawKeys(Canvas * cv, int x, int y, char const * keys)
{
  char buf[32];
  strlcpy(buf, keys, sizeof(buf));
  char * key = buf;
  while (key) {
    char * next = strchr(key, '|');
    if (next)
      *next++ = 0;
    x += drawKeyCap(cv, x, y, key);
    if (next) {
      drawText(cv, SmallFont, Theme::Dim, x + 2, y, "+");
      x += 10;
    }
    key = next;
  }
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Program names
//
//    PROGNAME [EXP] [ADDRESS].EXT


enum class ProgramKind { Folder, ParentFolder, Program, Cartridge, Other };

static char const * RAMTAGS[8] = { "", "3K", "8K", "16K", "24K", "27K", "32K", "35K" };

static char const * RAMDESCS[8] = { "UNEXPANDED", "3K EXPANSION", "8K EXPANSION", "16K EXPANSION", "24K EXPANSION",
                                    "27K (24K + 3K)", "32K EXPANSION", "35K (32K + 3K)" };

static char const * JOYDESCS[3] = { "NONE", "CURSOR KEYS", "MOUSE" };


static bool hasExtension(char const * filename, char const * ext)
{
  char const * dot = strrchr(filename, '.');
  return dot && strcasecmp(dot + 1, ext) == 0;
}


// the name to show, without extension and tags. "ram" gets the RAMExpansionOption (0 = none), "address" the
// cartridge address tag ("20", "40", "60", "A0", empty if missing)
static void parseProgramName(char const * filename, char * name, int nameSize, int * ram, char * address)
{
  *ram = 0;
  address[0] = 0;
  strlcpy(name, filename, nameSize);
  char * dot = strrchr(name, '.');
  if (dot && dot != name)
    *dot = 0;
  bool isCRT = hasExtension(filename, "CRT");
  // tags from the end
  while (true) {
    char * space = strrchr(name, ' ');
    if (!space)
      break;
    char const * tag = space + 1;
    bool found = false;
    for (int i = 1; i < 8 && !found; ++i)
      if (strcasecmp(tag, RAMTAGS[i]) == 0) {
        *ram  = i;
        found = true;
      }
    if (!found && isCRT && (!strcmp(tag, "20") || !strcmp(tag, "40") || !strcmp(tag, "60") || !strcasecmp(tag, "A0"))) {
      strlcpy(address, tag, 3);
      address[0] = toupper(address[0]);
      found = true;
    }
    if (!found)
      break;
    *space = 0;
  }
}


static ProgramKind programKind(FileBrowser & dir, int index)
{
  auto item = dir.get(index);
  if (item->isDir)
    return strcmp(item->name, "..") == 0 ? ProgramKind::ParentFolder : ProgramKind::Folder;
  if (hasExtension(item->name, "PRG"))
    return ProgramKind::Program;
  if (hasExtension(item->name, "CRT"))
    return ProgramKind::Cartridge;
  return ProgramKind::Other;
}


// a 11x10 icon, top-left at x, y. "inverted" = on the accent color (focused selection)
static void drawProgramIcon(Canvas * cv, ProgramKind kind, int x, int y, bool inverted)
{
  switch (kind) {

    case ProgramKind::Folder:
    case ProgramKind::ParentFolder:
    {
      RGB888 color = inverted ? Theme::Panel : (kind == ProgramKind::Folder ? Theme::Folder : Theme::Dim);
      fillRect(cv, color, Rect(x, y + 1, x + 4, y + 2));
      fillRect(cv, color, Rect(x, y + 3, x + 10, y + 9));
      if (kind == ProgramKind::ParentFolder) {
        // up arrow
        RGB888 arrow = inverted ? Theme::Accent : Theme::Background;
        cv->setPenColor(arrow);
        cv->drawLine(x + 5, y + 4, x + 5, y + 8);
        cv->drawLine(x + 3, y + 6, x + 5, y + 4);
        cv->drawLine(x + 7, y + 6, x + 5, y + 4);
      }
      break;
    }

    case ProgramKind::Program:
    {
      // a cassette
      RGB888 color = inverted ? Theme::Panel : Theme::Accent;
      cv->setPenColor(color);
      cv->drawRectangle(x, y + 1, x + 10, y + 8);
      fillRect(cv, color, Rect(x + 2, y + 3, x + 3, y + 4));
      fillRect(cv, color, Rect(x + 7, y + 3, x + 8, y + 4));
      cv->drawLine(x + 3, y + 6, x + 7, y + 6);
      break;
    }

    case ProgramKind::Cartridge:
    {
      // a cartridge with its contacts
      RGB888 color = inverted ? Theme::Panel : Theme::Cartridge;
      fillRect(cv, color, Rect(x, y, x + 10, y + 7));
      fillRect(cv, inverted ? Theme::Accent : Theme::Background, Rect(x + 3, y + 2, x + 7, y + 4));
      cv->setPenColor(color);
      for (int i = 1; i < 10; i += 2)
        cv->drawLine(x + i, y + 8, x + i, y + 9);
      break;
    }

    default:
    {
      // a document
      RGB888 color = inverted ? Theme::Panel : Theme::Dim;
      cv->setPenColor(color);
      cv->drawRectangle(x + 1, y, x + 8, y + 9);
      cv->drawLine(x + 3, y + 3, x + 6, y + 3);
      cv->drawLine(x + 3, y + 5, x + 6, y + 5);
      break;
    }

  }
}


// a small tag at the right of a list item (right side at x2), returns its left side
static int drawBadge(Canvas * cv, int x2, int cy, char const * text, bool inverted, bool dim)
{
  int  w = strlen(text) * 6 + 5;
  Rect r(x2 - w + 1, cy - 5, x2, cy + 4);
  RGB888 back = inverted ? Theme::Panel : (dim ? Theme::Panel : Theme::DarkAccent);
  drawRoundRect(cv, back, back, inverted ? Theme::Accent : Theme::Background, r);
  drawText(cv, TinyFont, dim && !inverted ? Theme::Text : Theme::Accent, r.X1 + 3, r.Y1 + 1, text);
  return r.X1;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dialogs


// a fixed frame with the theme
struct DialogFrame : public uiFrame {

  DialogFrame(uiFrame * parent, char const * title, Size const & size)
    : uiFrame(parent, title, UIWINDOW_PARENTCENTER, size, false, STYLE_DIALOG)
  {
    frameProps().resizeable        = false;
    frameProps().moveable          = false;
    frameProps().hasCloseButton    = false;
    frameProps().hasMaximizeButton = false;
    frameProps().hasMinimizeButton = false;
  }

};



struct DownloadProgressFrame : public DialogFrame {

  uiLabel *       fileLabel;
  uiLabel *       countLabel;
  uiProgressBar * progress;
  uiButton *      button;

  DownloadProgressFrame(uiFrame * parent, char const * title = "DOWNLOADING")
    : DialogFrame(parent, title, Size(220, 104))
  {
    fileLabel  = new uiLabel(this, "Connecting...", Point(10, 24), Size(200, 14), true, STYLE_VALUE_LEFT);
    progress   = new uiProgressBar(this, Point(10, 42), Size(200, 14), true, STYLE_PROGRESS);
    countLabel = new uiLabel(this, "", Point(10, 60), Size(200, 14), true, STYLE_DIM_VALUE);
    countLabel->labelStyle().textAlign = uiHAlign::Left;
    button = new uiButton(this, "ABORT", Point(80, 78), Size(60, 18), uiButtonKind::Button, true, STYLE_BUTTON);
    button->onClick = [&]() { exitModal(1); };
  }

};



// two text fields (WiFi network and password, URL and file name)
struct TwoFieldsDialog : public DialogFrame {

  uiTextEdit * edit1;
  uiTextEdit * edit2;

  TwoFieldsDialog(uiFrame * parent, char const * title, char const * label1, char const * text1, int width1, char const * label2, char const * text2, int width2)
    : DialogFrame(parent, title, Size(240, 128))
  {
    onPaint = [this, label1, label2]() {
      auto cv = canvas();
      drawHeading(cv, 8, 21, label1);
      drawHeading(cv, 8, 61, label2);
    };
    edit1 = new uiTextEdit(this, text1, Point(8, 36), Size(width1, 20), true, STYLE_EDIT);
    edit2 = new uiTextEdit(this, text2, Point(8, 76), Size(width2, 20), true, STYLE_EDIT);
    auto okButton = new uiButton(this, "OK", Point(116, 102), Size(54, 18), uiButtonKind::Button, true, STYLE_BUTTON);
    okButton->onClick = [&]() { exitModal(1); };
    auto cancelButton = new uiButton(this, "CANCEL", Point(176, 102), Size(54, 18), uiButtonKind::Button, true, STYLE_BUTTON);
    cancelButton->onClick = [&]() { exitModal(0); };
    onKeyUp = [&](uiKeyEventInfo const & key) {
      if (key.VK == VirtualKey::VK_RETURN || key.VK == VirtualKey::VK_KP_ENTER)
        exitModal(1);
      else if (key.VK == VirtualKey::VK_ESCAPE)
        exitModal(0);
    };
    onShow = [&]() { app()->setFocusedWindow(edit1); };
  }

  // shows the dialog: true = OK, the texts are copied to text1 and text2
  static bool run(uiApp * app, char const * title, char const * label1, char * text1, int size1, int width1, char const * label2, char * text2, int size2, int width2, bool password2 = false, bool fileFromURL = false)
  {
    auto dlg = new TwoFieldsDialog(app->rootWindow(), title, label1, text1, width1, label2, text2, width2);
    dlg->edit2->textEditProps().passwordMode = password2;
    if (fileFromURL) {
      // the file name follows the last part of the URL
      dlg->edit1->onChange = [=]() {
        char const * slash = strrchr(dlg->edit1->text(), '/');
        dlg->edit2->setText(slash ? slash + 1 : "");
        dlg->edit2->repaint();
      };
    }
    bool ok = app->showModalWindow(dlg) == 1;
    if (ok) {
      strlcpy(text1, dlg->edit1->text(), size1);
      strlcpy(text2, dlg->edit2->text(), size2);
    }
    app->destroyWindow(dlg);
    return ok;
  }

};



struct HelpFrame : public DialogFrame {

  HelpFrame(uiFrame * parent)
    : DialogFrame(parent, "KEYBOARD HELP", Size(240, 332))
  {
    auto button = new uiButton(this, "OK", Point(90, 306), Size(60, 18), uiButtonKind::Button, true, STYLE_BUTTON);
    button->onClick = [&]() { exitModal(0); };
    onKeyUp = [&](uiKeyEventInfo const & key) {
      if (key.VK == VirtualKey::VK_ESCAPE || key.VK == VirtualKey::VK_F1)
        exitModal(0);
    };
    onShow = [this, button]() { app()->setFocusedWindow(button); };

    onPaint = [&]() {
      auto cv = canvas();
      constexpr int KX = 10;    // keys column
      constexpr int DX = 112;   // descriptions column
      constexpr int DY = 14;    // row height
      int y = 21;

      static char const * GENERAL[][2] = {
        { "F12",           "Run / back to menu" },
        { "F1",            "This help" },
        { "DEL",           "Delete file or folder" },
        { "L-ALT|A S W Z", "Move the screen" },
      };
      drawHeading(cv, 8, y, "GENERAL");
      y += 16;
      for (auto & row : GENERAL) {
        drawKeys(cv, KX, y, row[0]);
        drawText(cv, SmallFont, Theme::Text, DX, y, row[1]);
        y += DY;
      }

      static char const * VICKEYS[][2] = {
        { "ESC",       "RUN/STOP" },
        { "HOME",      "CLR/HOME" },
        { "BACKSPACE", "INST/DEL" },
        { "DELETE",    "RESTORE" },
        { "L-WIN",     "CBM (Commodore key)" },
        { "^",         "Up arrow" },
        { "_",         "Left arrow" },
        { "~",         "Pi" },
      };
      y += 4;
      drawHeading(cv, 8, y, "VIC-20 KEYBOARD");
      y += 16;
      for (auto & row : VICKEYS) {
        drawKeys(cv, KX, y, row[0]);
        drawText(cv, SmallFont, Theme::Text, DX, y, row[1]);
        y += DY;
      }

      static char const * JOYSTICK[][3] = {
        { "NONE",    "R-ALT + ARROWS", "R-ALT + MENU" },
        { "CURSORS", "ARROWS",         "MENU" },
        { "MOUSE",   "MOUSE",          "BUTTON" },
      };
      constexpr int JX[3] = { 10, 74, 166 };
      y += 4;
      drawHeading(cv, 8, y, "JOYSTICK");
      y += 16;
      drawText(cv, SmallFont, Theme::Dim, JX[0], y, "MODE");
      drawText(cv, SmallFont, Theme::Dim, JX[1], y, "MOVE");
      drawText(cv, SmallFont, Theme::Dim, JX[2], y, "FIRE");
      y += DY;
      for (auto & row : JOYSTICK) {
        for (int i = 0; i < 3; ++i)
          drawText(cv, SmallFont, i == 0 ? Theme::Accent : Theme::Text, JX[i], y, row[i]);
        y += DY;
      }
    };
  }

};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration screen (256x384)
//
//   +-----------------------------+
//   | VIC-20 ///// EMULATOR  HELP |  header
//   | [PROGRAMS                 ] |  program list, free space
//   | [MACHINE                  ] |  RAM expansion, joystick, keyboard layout
//   | RUN  LOAD  RESET  HARD RESET|
//   | [DOWNLOAD        WEB  URL ] |
//   |   www.fabglib.org, credits  |
//   +-----------------------------+


class Menu : public uiApp {

  // layout
  static constexpr int  HEADER_H      = 28;
  static Rect programsPanel()           { return Rect(4,  32, 251, 199); }
  static Rect machinePanel()            { return Rect(4, 203, 251, 301); }
  static constexpr int  ACTIONS_Y     = 305;
  static Rect downloadPanel()           { return Rect(4, 330, 251, 354); }
  static constexpr int  FOOTER_Y      = 358;

  uiFileBrowser *  fileBrowser;
  uiProgressBar *  storageBar;
  uiLabel *        freeSpaceLbl;
  uiTileSelector * ramTiles;
  uiLabel *        ramLabel;
  uiTileSelector * joyTiles;
  uiLabel *        joyLabel;
  uiStepper *      kbdStepper;
  int              kbdLayout;

  // Main machine
  Machine *       machine;


  void init() {
    machine = new Machine(&DisplayController);

    setStyle(&vic20Style);

    // message boxes with the theme
    auto & dlgStyles = appProps().dialogStyleClasses;
    dlgStyles.frame    = STYLE_DIALOG;
    dlgStyles.label    = STYLE_DIALOG_LABEL;
    dlgStyles.button   = STYLE_BUTTON;
    dlgStyles.textEdit = STYLE_EDIT;
    dlgStyles.panel    = STYLE_DIALOG_PANEL;

    rootWindow()->frameStyle().backgroundColor = Theme::Background;
    rootWindow()->onPaint = [&]() { paintBackground(); };

    // programs list
    Rect pp = programsPanel();
    fileBrowser = new uiFileBrowser(rootWindow(), Point(pp.X1 + 6, pp.Y1 + 20), Size(pp.width() - 12, 130), true, STYLE_PROGRAMS);
    fileBrowser->setDirectory(basepath);  // set absolute path
    fileBrowser->changeDirectory( fileBrowser->content().exists(DOWNDIR, false) ? DOWNDIR : EMBDIR ); // set relative path
    fileBrowser->onPaintItem = [&](int index, Rect const & r, bool selected) {
      paintProgramItem(index, r, selected);
    };
    fileBrowser->onChange = [&]() {
      setSelectedProgramConf();
    };

    // free space
    storageBar   = new uiProgressBar(rootWindow(), Point(pp.X1 + 6, pp.Y2 - 12), Size(120, 8), true, STYLE_STORAGE);
    freeSpaceLbl = new uiLabel(rootWindow(), "", Point(pp.X1 + 130, pp.Y2 - 15), Size(pp.X2 - 6 - pp.X1 - 130 + 1, 13), true, STYLE_VALUE);
    updateFreeSpaceLabel();

    // handles following keys:
    //  F1         ->  show help
    //  ESC or F12 ->  run the emulator
    rootWindow()->onKeyUp = [&](uiKeyEventInfo const & key) {
      switch (key.VK) {
        // F1
        case VirtualKey::VK_F1:
          showHelp();
          break;
        // ESC or F12
        case VirtualKey::VK_ESCAPE:
        case VirtualKey::VK_F12:
          runVIC20();
          break;
        // just to avoid compiler warning
        default:
          break;
      }
    };

    // handles following keys in filebrowser:
    //   RETURN -> load selected program and return to vic
    //   DELETE -> remove selected dir or file
    fileBrowser->onKeyUp = [&](uiKeyEventInfo const & key) {
      switch (key.VK) {
        // RETURN
        case VirtualKey::VK_RETURN:
          if (!fileBrowser->isDirectory() && loadSelectedProgram())
            runVIC20();
          break;
        // DELETE
        case VirtualKey::VK_DELETE:
          if (fileBrowser->filename() && strcmp(fileBrowser->filename(), "..") != 0) {
            char text[48];
            snprintf(text, sizeof(text), "Delete \"%.24s\"?", fileBrowser->filename());
            if (messageBox("DELETE", text, "Yes", "Cancel", nullptr, uiMessageBoxIcon::Question) == uiMessageBoxResult::Button1) {
              fileBrowser->content().remove( fileBrowser->filename() );
              fileBrowser->update();
              updateFreeSpaceLabel();
            }
          }
          break;
        // just to avoid compiler warning
        default:
          break;
      }
    };

    // handles double click on program list
    fileBrowser->onDblClick = [&]() {
      if (!fileBrowser->isDirectory() && loadSelectedProgram())
        runVIC20();
    };

    // RAM expansion
    Rect mp = machinePanel();
    ramLabel = new uiLabel(rootWindow(), "", Point(mp.X1 + 100, mp.Y1 + 19), Size(mp.X2 - 6 - mp.X1 - 100 + 1, 13), true, STYLE_VALUE);
    ramTiles = new uiTileSelector(rootWindow(), Point(mp.X1 + 6, mp.Y1 + 33), Size(mp.width() - 12, 21), true, STYLE_TILES);
    for (int i = 0; i < 8; ++i)
      ramTiles->items().append("");
    ramTiles->onPaintTile = [&](int index, Rect const & r, bool selected) {
      paintTileText(ramTiles, r, selected, index ? RAMTAGS[index] : "--");
    };
    ramTiles->onChange = [&]() {
      machine->setRAMExpansion((RAMExpansionOption) ramTiles->selectedItem());
      updateRAMLabel();
    };
    ramTiles->selectItem((int) machine->RAMExpansion());
    updateRAMLabel();

    // joystick emulation
    joyLabel = new uiLabel(rootWindow(), "", Point(mp.X1 + 66, mp.Y1 + 57), Size(80, 13), true, STYLE_VALUE_LEFT);
    joyTiles = new uiTileSelector(rootWindow(), Point(mp.X1 + 6, mp.Y1 + 71), Size(132, 22), true, STYLE_TILES);
    for (int i = 0; i < 3; ++i)
      joyTiles->items().append("");
    joyTiles->onPaintTile = [&](int index, Rect const & r, bool selected) {
      paintJoystickIcon(index, r, selected);
    };
    joyTiles->onChange = [&]() {
      setJoystick(joyTiles->selectedItem());
    };
    setJoystick(preferences.getInt("joyEmu", (int) machine->joyEmu()));

    // keyboard layout
    kbdLayout  = updateKbdLayout();
    kbdStepper = new uiStepper(rootWindow(), Point(mp.X1 + 146, mp.Y1 + 71), Size(mp.X2 - 6 - mp.X1 - 146 + 1, 22), true, STYLE_STEPPER);
    kbdStepper->setText(SupportedLayouts::shortNames()[kbdLayout]);
    kbdStepper->onStep = [&](int step) {
      int count = SupportedLayouts::count();
      kbdLayout = (kbdLayout + step + count) % count;
      preferences.putInt("kbdLayout", kbdLayout);
      updateKbdLayout();
      kbdStepper->setText(SupportedLayouts::shortNames()[kbdLayout]);
      kbdStepper->repaint();
    };

    // actions: Run, Load, Soft Reset and Hard Reset
    auto VIC20Button = new uiButton(rootWindow(), "RUN  F12", Point(4, ACTIONS_Y), Size(64, 22), uiButtonKind::Button, true, STYLE_BUTTON_RUN);
    VIC20Button->onClick = [&]() {
      runVIC20();
    };

    auto loadButton = new uiButton(rootWindow(), "LOAD", Point(72, ACTIONS_Y), Size(50, 22), uiButtonKind::Button, true, STYLE_BUTTON);
    loadButton->onClick = [&]() {
      if (loadSelectedProgram())
        runVIC20();
    };

    auto resetButton = new uiButton(rootWindow(), "RESET", Point(126, ACTIONS_Y), Size(52, 22), uiButtonKind::Button, true, STYLE_BUTTON);
    resetButton->onClick = [&]() {
      machine->reset();
      runVIC20();
    };

    // removes the cartridge and resets
    auto hresetButton = new uiButton(rootWindow(), "HARD RESET", Point(182, ACTIONS_Y), Size(70, 22), uiButtonKind::Button, true, STYLE_BUTTON_DANGER);
    hresetButton->onClick = [&]() {
      machine->removeCRT();
      machine->reset();
      runVIC20();
    };

    // Auto button: downloads the games listed and linked in weblist.h
    Rect dp = downloadPanel();
    auto downloadProgsBtn = new uiButton(rootWindow(), "AUTO", Point(dp.X2 - 126, dp.Y1 + 4), Size(62, 17), uiButtonKind::Button, true, STYLE_BUTTON);
    downloadProgsBtn->onClick = [&]() {
      downloadGames();
    };

    // Download from URL
    auto downloadURLBtn = new uiButton(rootWindow(), "URL...", Point(dp.X2 - 59, dp.Y1 + 4), Size(54, 17), uiButtonKind::Button, true, STYLE_BUTTON);
    downloadURLBtn->onClick = [&]() {
      downloadFromURL();
    };

    // help
    auto helpButton = new uiButton(rootWindow(), "HELP", Point(214, 6), Size(37, 17), uiButtonKind::Button, true, STYLE_BUTTON);
    helpButton->onClick = [&]() {
      showHelp();
    };

    // focus on programs list
    setFocusedWindow(fileBrowser);
  }

  void paintBackground()
  {
    auto cv = rootWindow()->canvas();

    fillRect(cv, Theme::Background, Rect(0, 0, 255, 383));

    // header: title, VIC-20 stripes, subtitle and a rainbow line
    drawText(cv, TitleFont, Theme::White, 8, 2, "VIC-20", Align::Left, true);
    int x = 8 + cv->textExtent(TitleFont, "VIC-20") + 6 + 8;
    drawStripes(cv, x, 5, 18);
    drawText(cv, SmallFont, Theme::Accent, x + 46, 9, "EMULATOR", Align::Left, true);
    for (int i = 0; i < 5; ++i)
      fillRect(cv, Theme::Rainbow[i], Rect(i * 256 / 5, HEADER_H - 1, (i + 1) * 256 / 5 - 1, HEADER_H));

    // programs
    Rect pp = programsPanel();
    drawPanel(cv, pp, "PROGRAMS");
    drawText(cv, SmallFont, Theme::Dim, pp.X2 - 6, pp.Y1 + 4, basepath && strcmp(basepath, SDCARD_MOUNT_PATH) == 0 ? "SD CARD" : "FLASH", Align::Right);

    // machine settings
    Rect mp = machinePanel();
    drawPanel(cv, mp, "MACHINE");
    drawText(cv, SmallFont, Theme::Dim, mp.X1 + 6, mp.Y1 + 19, "RAM EXPANSION");
    drawText(cv, SmallFont, Theme::Dim, mp.X1 + 6, mp.Y1 + 57, "JOYSTICK");
    drawText(cv, SmallFont, Theme::Dim, mp.X1 + 146, mp.Y1 + 57, "KEYBOARD");

    // download
    Rect dp = downloadPanel();
    drawPanel(cv, dp, nullptr);
    drawHeading(cv, dp.X1 + 6, dp.Y1 + 6, "DOWNLOAD");

    // footer
    drawText(cv, SmallFont, Theme::Dim,  128, FOOTER_Y,      "www.fabglib.org", Align::Center);
    drawText(cv, SmallFont, Theme::Edge, 128, FOOTER_Y + 12, "2019-2026 by Fabrizio Di Vittorio", Align::Center);
  }

  void paintProgramItem(int index, Rect const & r, bool selected)
  {
    auto cv      = fileBrowser->canvas();
    auto & dir   = fileBrowser->content();
    auto item    = dir.get(index);
    bool focused = selected && fileBrowser->hasFocus();   // the row has the accent color
    int  cy      = (r.Y1 + r.Y2) / 2;
    auto kind    = programKind(dir, index);

    drawProgramIcon(cv, kind, r.X1 + 4, cy - 5, focused);

    char name[64];
    int  ram = 0;
    char address[3] = "";
    if (kind == ProgramKind::Program || kind == ProgramKind::Cartridge)
      parseProgramName(item->name, name, sizeof(name), &ram, address);
    else
      strlcpy(name, item->name, sizeof(name));

    // tags on the right: RAM expansion and cartridge address
    int x2 = r.X2 - 3;
    if (address[0]) {
      char text[6];
      snprintf(text, sizeof(text), "%s00", address);
      x2 = drawBadge(cv, x2, cy, text, focused, true) - 3;
    }
    if (ram)
      x2 = drawBadge(cv, x2, cy, RAMTAGS[ram], focused, false) - 3;

    RGB888 textColor = focused ? Theme::Panel : (selected ? Theme::White : (kind == ProgramKind::Other ? Theme::Dim : Theme::Text));
    cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false));
    cv->setPenColor(textColor);
    cv->drawTextWithEllipsis(SmallFont, r.X1 + 20, cy - SmallFont->height / 2, name, x2 - 2);
  }

  // a text in a tile, the selected tile is filled with the accent color
  void paintTileText(uiTileSelector * tiles, Rect const & r, bool selected, char const * text)
  {
    auto cv = tiles->canvas();
    if (selected)
      fillRect(cv, Theme::Accent, r.shrink(2));
    drawText(cv, SmallFont, selected ? Theme::Panel : Theme::Text, (r.X1 + r.X2 + 1) / 2, (r.Y1 + r.Y2 + 1) / 2 - SmallFont->height / 2, text, Align::Center, selected);
  }

  // 0 = none (crossed circle), 1 = cursor keys, 2 = mouse
  void paintJoystickIcon(int index, Rect const & r, bool selected)
  {
    auto cv = joyTiles->canvas();
    if (selected)
      fillRect(cv, Theme::Accent, r.shrink(2));
    RGB888 color = selected ? Theme::Panel : Theme::Text;
    int cx = (r.X1 + r.X2) / 2, cy = (r.Y1 + r.Y2) / 2;
    cv->setPenColor(color);
    switch (index) {
      case 0:
        cv->drawEllipse(cx, cy, 13, 13);
        cv->drawLine(cx - 4, cy + 4, cx + 4, cy - 4);
        break;
      case 1:
        cv->drawRectangle(cx - 3, cy - 7, cx + 3, cy - 2);
        cv->drawRectangle(cx - 11, cy, cx - 5, cy + 5);
        cv->drawRectangle(cx - 3, cy, cx + 3, cy + 5);
        cv->drawRectangle(cx + 5, cy, cx + 11, cy + 5);
        break;
      case 2:
        cv->drawRectangle(cx - 5, cy - 7, cx + 5, cy + 7);
        cv->drawLine(cx - 5, cy - 2, cx + 5, cy - 2);
        cv->drawLine(cx, cy - 7, cx, cy - 2);
        fillRect(cv, color, Rect(cx - 4, cy - 6, cx - 1, cy - 3));
        // round corners
        cv->setPenColor(selected ? Theme::Accent : Theme::Background);
        cv->setPixel(cx - 5, cy - 7);
        cv->setPixel(cx + 5, cy - 7);
        cv->setPixel(cx - 5, cy + 7);
        cv->setPixel(cx + 5, cy + 7);
        break;
    }
  }

  void updateRAMLabel()
  {
    int index = ramTiles->selectedItem();
    ramLabel->setText(RAMDESCS[index >= 0 && index < 8 ? index : 0]);
  }

  void selectRAMExpansion(int index)
  {
    if (index != ramTiles->selectedItem()) {
      ramTiles->selectItem(index);
      machine->setRAMExpansion((RAMExpansionOption) index);
      updateRAMLabel();
    }
  }

  void setJoystick(int mode)
  {
    if (mode < JE_None || mode > JE_Mouse)
      mode = JE_None;
    machine->setJoyEmu((JoyEmu) mode);
    joyTiles->selectItem(mode);
    joyLabel->setText(JOYDESCS[mode]);
    if (preferences.getInt("joyEmu", -1) != mode)
      preferences.putInt("joyEmu", mode);
  }

  // downloads from the Internet all the games listed in weblist.h
  void downloadGames()
  {
    if (checkWiFi() && messageBox("DOWNLOAD GAMES", "From the Internet. Check your local laws", "OK", "Cancel", nullptr, uiMessageBoxIcon::Warning) == uiMessageBoxResult::Button1) {
      auto pframe = new DownloadProgressFrame(rootWindow(), "DOWNLOADING GAMES");
      auto modalStatus = initModalWindow(pframe);
      processModalWindowEvents(modalStatus, 100);
      prepareForDownload();
      char const * const * plist = WEB_LIST;
      int count = sizeof(WEB_LIST) / (2 * sizeof(char const *));
      int dcount = 0;
      for (int i = 0; i < count; ++i) {
        char const * filename = *plist++;
        char const * URL      = *plist++;
        pframe->fileLabel->setText(filename);
        pframe->countLabel->setTextFmt("Game %d of %d, %d downloaded", i + 1, count, dcount);
        if (!processModalWindowEvents(modalStatus, 100))
          break;
        if (downloadURL(URL, filename))
          ++dcount;
        pframe->progress->setPercentage((i + 1) * 100 / count);
      }
      pframe->fileLabel->setText("Completed");
      pframe->countLabel->setTextFmt("%d of %d games downloaded", dcount, count);
      // modify "abort" button to "OK"
      pframe->button->setText("OK");
      pframe->button->repaint();
      // wait for OK
      processModalWindowEvents(modalStatus, -1);
      endModalWindow(modalStatus);
      destroyWindow(pframe);
      fileBrowser->update();
      updateFreeSpaceLabel();
    }
    wifi.disconnect();
  }

  // downloads a single program
  void downloadFromURL()
  {
    if (checkWiFi()) {
      char * URL = new char[128];
      char * filename = new char[25];
      strcpy(URL, "http://");
      filename[0] = 0;
      if (TwoFieldsDialog::run(this, "DOWNLOAD FROM URL", "URL", URL, 128, 224, "FILE NAME", filename, 25, 150, false, true) && filename[0]) {
        if (downloadURL(URL, filename)) {
          messageBox("DOWNLOAD", "Download OK!", "OK", nullptr, nullptr, uiMessageBoxIcon::Info);
          fileBrowser->update();
          updateFreeSpaceLabel();
        } else
          messageBox("DOWNLOAD", "Download failed!", "OK", nullptr, nullptr, uiMessageBoxIcon::Error);
      }
      delete [] filename;
      delete [] URL;
      wifi.disconnect();
    }
  }

  // this is the main loop where 6502 instructions are executed until F12 has been pressed
  void runVIC20()
  {
    auto keyboard = PS2Controller.keyboard();

    enableKeyboardAndMouseEvents(false);
    keyboard->emptyVirtualKeyQueue();
    machine->VIC().enableAudio(true);

    // setup user directory
    machine->fileBrowser()->setDirectory(basepath);
    if (!machine->fileBrowser()->exists(USERDIR, false))
      machine->fileBrowser()->makeDirectory(USERDIR);
    machine->fileBrowser()->changeDirectory(USERDIR);

    auto cv = canvas();
    cv->setBrushColor(0, 0, 0);
    cv->clear();
    cv->waitCompletion();

    bool run = true;
    while (run) {

      #if DEBUGMACHINE
        int cycles = machine->run();
        if (restart) {
          scycles = cycles;
          restart = false;
        } else
          scycles += cycles;
      #else
        machine->run();
      #endif

      // read keyboard
      if (keyboard->virtualKeyAvailable()) {
        bool keyDown;
        VirtualKey vk = keyboard->getNextVirtualKey(&keyDown);
        switch (vk) {

          // F12 - stop running
          case VirtualKey::VK_F12:
            if (!keyDown)
              run = false;  // exit loop
            break;

          // other keys
          default:
            // send to emulated machine
            machine->setKeyboard(vk, keyDown);
            break;
        }
      }

    }
    machine->VIC().enableAudio(false);
    keyboard->emptyVirtualKeyQueue();
    enableKeyboardAndMouseEvents(true);
    rootWindow()->repaint();
  }

  // extract required RAM size from selected filename (a space and 3K, 8K...)
  void setSelectedProgramConf()
  {
    char const * fname = fileBrowser->filename();
    if (fname) {
      // select required memory expansion
      static char const * EXP[8] = { "", " 3K", " 8K", " 16K", " 24K", " 27K", " 32K", " 35K" };
      int r = 0;
      for (int i = 1; i < 8; ++i)
        if (strstr(fname, EXP[i])) {
          r = i;
          break;
        }
      selectRAMExpansion(r);
    }
  }

  // extract address from selected filename. "20"=0x2000, "40"=0x4000, "60"=0x6000 or "A0"=0xA000, "A0" or automatic is the default
  // addrPos if specified will contain where address has been found
  int getAddress(char * filename, char * * addrPos = nullptr)
  {
    static char const * ASTR[] = { " 20", " 40", " 60", " a0", " A0" };
    static const int    ADDR[] = { 0x2000, 0x4000, 0x6000, 0xa000, 0xa000 };
    for (int i = 0; i < sizeof(ADDR) / sizeof(int); ++i) {
      auto p = strstr(filename, ASTR[i]);
      if (p) {
        if (addrPos)
          *addrPos = p + 1;
        return ADDR[i];
      }
    }
    return -1;
  }

  // configuration given by filename:
  //    PROGNAME[EXP][ADDRESS].EXT
  //      PROGNAME : program name (not included extension)
  //      EXP      : expansion ("3K", "8K", "16K", "24K", "27K", "32K", "35K")
  //      ADDRESS  : hex address ("20"=0x2000, "40"=0x4000, "60"=0x6000 or "A0"=0xA000), "A0" or automatic is the default
  //      EXT      : "CRT" or "PRG"
  bool loadSelectedProgram()
  {
    bool backToVIC = false;
    char const * fname = fileBrowser->filename();
    if (fname) {
      FileBrowser & dir  = fileBrowser->content();

      bool isPRG = strstr(fname, ".PRG") || strstr(fname, ".prg");
      bool isCRT = strstr(fname, ".CRT") || strstr(fname, ".crt");

      if (isPRG || isCRT) {

        machine->removeCRT();

        int fullpathlen = dir.getFullPath(fname);
        char fullpath[fullpathlen];
        dir.getFullPath(fname, fullpath, fullpathlen);

        if (isPRG) {
          // this is a PRG, just reset, load and run
          machine->loadPRG(fullpath, true, true);
        } else if (isCRT) {
          // this a CRT, find other parts, load and reset
          char * addrPos = nullptr;
          int addr = getAddress(fullpath, &addrPos);
          if (addrPos) {
            // address specified, try to load all possible positions (2000, 4000, 6000 and a000)
            static const char * POS = { "246Aa" }; // 2=2000, 4=4000, 6=6000, A=a000
            for (int i = 0; i < sizeof(POS); ++i) {
              *addrPos = POS[i];  // actually replaces char inside "fullpath"
              machine->loadCRT(fullpath, true, getAddress(fullpath));
            }
          } else {
            // no address specified, just load it
            machine->loadCRT(fullpath, true, addr);
          }
        }
        backToVIC = true;
      }
    }
    return backToVIC;
  }

  // true if WiFi is has been connected
  bool checkWiFi() {
    if (wifi.state() == fabgl::WiFiConnectionState::Connected)
      return true;
    char SSID[32] = "";
    char psw[64]  = "";
    if (!preferences.getString("SSID", SSID, sizeof(SSID)) || !preferences.getString("WiFiPsw", psw, sizeof(psw))) {
      // ask user for SSID and password
      if (TwoFieldsDialog::run(this, "WIFI CONNECT", "NETWORK NAME", SSID, sizeof(SSID), 224, "PASSWORD", psw, sizeof(psw), 224, true)) {
        preferences.putString("SSID", SSID);
        preferences.putString("WiFiPsw", psw);
      } else
        return false;
    }
    bool connected = (wifi.connect(SSID, psw, 16000) == fabgl::WiFiConnectionState::Connected);
    if (!connected) {
      messageBox("NETWORK ERROR", "WiFi connection failed", "OK", nullptr, nullptr, uiMessageBoxIcon::Error);
      preferences.remove("WiFiPsw");
    }
    return connected;
  }

  // creates List folder and makes it current
  void prepareForDownload()
  {
    FileBrowser & dir = fileBrowser->content();
    dir.setDirectory(basepath);
    dir.makeDirectory(DOWNDIR);
    dir.changeDirectory(DOWNDIR);
  }

  // download specified filename from URL
  bool downloadURL(char const * URL, char const * filename)
  {
    //printf("Downloading %s from %s\n", filename, URL);
    FileBrowser & dir = fileBrowser->content();
    if (dir.exists(filename, false)) {
      return true;
    }
    bool success = false;
    fabgl::HTTPRequest http;
    int httpCode = http.open(URL);
    if (httpCode == 200) {

      int fullpathLen = dir.getFullPath(filename);
      char fullpath[fullpathLen];
      dir.getFullPath(filename, fullpath, fullpathLen);
      FILE * f = fopen(fullpath, "wb");

      if (f) {
        int len = http.contentLength();
        constexpr int BUFSIZE = 1024;
        uint8_t * buf = (uint8_t*) malloc(BUFSIZE);
        int dsize = 0;
        while (len > 0 || len == -1) {
          int c = http.read(buf, BUFSIZE);
          if (c <= 0)
            break;
          fwrite(buf, c, 1, f);
          dsize += c;
          if (len > 0)
            len -= c;
        }
        free(buf);
        fclose(f);
        success = (len == 0 || (len == -1 && dsize > 0));
        if (!success)
          remove(fullpath);   // don't leave a partial file, it would look already downloaded
      }
    }
    //printf("Download %s\n", success ? "OK" : "Failed");
    return success;
  }

  // show free space (SD card or SPIFFS)
  void updateFreeSpaceLabel() {
    int64_t total = 0, used = 0;
    FileBrowser::getFSInfo(fileBrowser->content().getCurrentDriveType(), 0, &total, &used);
    int64_t freeKB = (total - used) / 1024;
    if (freeKB >= 10240)
      freeSpaceLbl->setTextFmt("%lld MiB FREE", freeKB / 1024);
    else
      freeSpaceLbl->setTextFmt("%lld KiB FREE", freeKB);
    storageBar->setPercentage(total > 0 ? (int) (used * 100 / total) : 0);
  }

  int updateKbdLayout() {
    auto curLayoutIdx = preferences.getInt("kbdLayout", 3); // default US
    PS2Controller.keyboard()->setLayout(SupportedLayouts::layouts()[curLayoutIdx]);
    return curLayoutIdx;
  }

  void showHelp() {
    auto hframe = new HelpFrame(rootWindow());
    showModalWindow(hframe);
    destroyWindow(hframe);
  }


};



///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////




void setup()
{
  #if DEBUG
  vTaskDelay(pdMS_TO_TICKS(500)); printf("\n\n\nReset\n"); // for debug purposes
  #endif

  preferences.begin("VIC20", false);
  //preferences.clear();

  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::CreateVirtualKeysQueue);

  DisplayController.begin();
  DisplayController.setResolution(VGA_256x384_60Hz);

  // this improves user interface speed - check possible drawbacks before enabling definitively
  //DisplayController.enableBackgroundPrimitiveExecution(false);

  // adjust this to center screen in your monitor
  //DisplayController.moveScreen(20, -2);

  Canvas cv(&DisplayController);
  cv.selectFont(&fabgl::FONT_6x8);

  cv.clear();
  cv.drawText(25, 10, "Initializing...");
  cv.waitCompletion();

  if (FileBrowser::mountSDCard(FORMAT_ON_FAIL, SDCARD_MOUNT_PATH))
    basepath = SDCARD_MOUNT_PATH;
  else if (FileBrowser::mountSPIFFS(FORMAT_ON_FAIL, SPIFFS_MOUNT_PATH))
    basepath = SPIFFS_MOUNT_PATH;

  cv.drawText(25, 30, "Copying embedded programs...");
  cv.waitCompletion();
  copyEmbeddedPrograms();
}


void loop()
{
  #if DEBUGMACHINE
  TimerHandle_t xtimer = xTimerCreate("", pdMS_TO_TICKS(5000), pdTRUE, (void*)0, vTimerCallback1SecExpired);
  xTimerStart(xtimer, 0);
  #endif

  auto menu = new Menu;
  menu->run(&DisplayController);
}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
