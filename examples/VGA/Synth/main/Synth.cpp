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
 * FabGL Synth: a polyphonic synthesizer played with the PS/2 keyboard (or the mouse).
 *
 * Keyboard layout (like most music trackers):
 *
 *      W   E       T   Y   U       O   P       ]/+
 *    A   S   D   F   G   H   J   K   L   ;/ò  '/à
 *    C   D   E   F   G   A   B   C   D   E    F
 *
 *    Z / X          : octave down / up
 *    1 .. 5         : select waveform
 *    PGUP / PGDN    : previous / next preset
 *
 * The sound is produced by fabgl::PolySynth: each note is played by a voice (an oscillator followed by a
 * resonant low-pass filter, shaped by an ADSR envelope; a second ADSR envelope moves the filter cutoff; an LFO
 * modulates cutoff and amplitude). The interface uses the FabGL synth controls (uiKnob, uiToggleSwitch,
 * uiLevelBar, uiLedBar, uiStepper, uiTileSelector, uiGraph, uiPianoKeyboard), themed by a uiStyle and a custom
 * 16-color palette.
 *
 * 64 presets (presets.h: classic synthesizer sounds) can be edited and saved to the NVS flash partition.
 */


#include <math.h>
#include <ctype.h>

#include "fabgl.h"
#include "fabui.h"

#include "presets.h"


using fabgl::uiStyle;
using fabgl::uiObject;
using fabgl::uiWindow;


fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;
SoundGenerator         soundGenerator;
PolySynth              synth;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Theme
//
// The display has 16 colors (VGA16Controller) and every palette slot is reprogrammed with a theme color.
// Canvas colors are RGB888 values that the driver maps to a palette slot through a table built from the
// default palette (setPaletteItem() doesn't rebuild it). So a theme color is selected with the default
// color of its slot: see ink().


enum class Ink {
  Background,   // slot 0 stays black: the mouse pointer uses it
  Panel,
  Edge,
  Fill,
  Shadow,
  Text,
  Dim,
  Amber,
  DimAmber,
  Cyan,
  Grid,
  Led,
  DimLed,
  Accent,
  KeyShade,
  White,        // slot 15 stays white: the mouse pointer uses it
};


static const RGB888 InkColors[16] = {
  RGB888(  0,   0,   0),   // Background
  RGB888(  0,   0,  85),   // Panel
  RGB888(  0,  85, 170),   // Edge
  RGB888(  0,   0, 170),   // Fill
  RGB888( 85,  85,  85),   // Shadow
  RGB888(170, 170, 255),   // Text
  RGB888( 85,  85, 170),   // Dim
  RGB888(255, 170,   0),   // Amber
  RGB888(170,  85,   0),   // DimAmber
  RGB888(  0, 255, 255),   // Cyan
  RGB888(  0,  85,  85),   // Grid
  RGB888(  0, 255,  85),   // Led
  RGB888(  0,  85,   0),   // DimLed
  RGB888(255,   0, 170),   // Accent
  RGB888(170, 170, 170),   // KeyShade
  RGB888(255, 255, 255),   // White
};


static RGB888 ink(Ink value)
{
  return RGB888((Color) value);
}


static void setupPalette()
{
  for (int i = 0; i < 16; ++i)
    DisplayController.setPaletteItem(i, InkColors[i]);
}


static fabgl::FontInfo const * const SmallFont  = &fabgl::FONT_std_12;
static fabgl::FontInfo const * const MediumFont = &fabgl::FONT_std_17;
static fabgl::FontInfo const * const BigFont    = &fabgl::FONT_std_24;
static fabgl::FontInfo const * const TinyFont   = &fabgl::FONT_6x8;


// style classes (last parameter of the control constructors)
enum StyleClass {
  STYLE_NONE,
  STYLE_KNOB,
  STYLE_SWITCH,
  STYLE_LEVELBAR,
  STYLE_LEDBAR,
  STYLE_STEPPER,
  STYLE_TILES,
  STYLE_GRAPH,
  STYLE_PIANO,
  STYLE_WAVE_NAME,      // label on a panel
  STYLE_PRESET,         // preset selector (stepper with the preset number and name)
  STYLE_BUTTON,
  STYLE_DIALOG,         // save dialog frame
  STYLE_DIALOG_LABEL,
  STYLE_EDIT,           // preset name edit box (the only focusable control)
};


struct SynthStyle : uiStyle {

  void setStyle(uiObject * object, uint32_t styleClassID)
  {
    // keyboard events go to the main frame (it plays the notes), except in the edit box of the save dialog
    ((uiWindow *) object)->windowProps().focusable = styleClassID == STYLE_EDIT;

    switch (styleClassID) {

      case STYLE_KNOB:
      {
        auto & s = ((uiKnob *) object)->knobStyle();
        s.backgroundColor     = ink(Ink::Panel);
        s.labelFont           = SmallFont;
        s.labelColor          = ink(Ink::Text);
        s.valueFont           = SmallFont;
        s.valueColor          = ink(Ink::Amber);
        s.activeValueColor    = ink(Ink::White);
        s.ringColor           = ink(Ink::Amber);
        s.ringBackgroundColor = ink(Ink::Shadow);
        s.bodyColor           = ink(Ink::Shadow);
        s.capColor            = ink(Ink::Background);
        s.pointerColor        = ink(Ink::White);
        break;
      }

      case STYLE_SWITCH:
      {
        auto & s = ((uiToggleSwitch *) object)->toggleSwitchStyle();
        s.backgroundColor = ink(Ink::Panel);
        s.onColor         = ink(Ink::Amber);
        s.offColor        = ink(Ink::Shadow);
        s.thumbColor      = ink(Ink::White);
        s.textFont        = SmallFont;
        s.onTextColor     = ink(Ink::Amber);
        s.offTextColor    = ink(Ink::Dim);
        break;
      }

      case STYLE_LEVELBAR:
      {
        auto & s = ((uiLevelBar *) object)->levelBarStyle();
        s.backgroundColor = ink(Ink::Panel);
        s.lowColor        = ink(Ink::Led);
        s.lowOffColor     = ink(Ink::DimLed);
        s.highColor       = ink(Ink::Amber);
        s.highOffColor    = ink(Ink::DimAmber);
        s.textFont        = MediumFont;
        s.textColor       = ink(Ink::Amber);
        break;
      }

      case STYLE_LEDBAR:
      {
        auto & s = ((uiLedBar *) object)->ledBarStyle();
        s.backgroundColor = ink(Ink::Background);
        s.ledColors[0]    = ink(Ink::DimLed);     // idle
        s.ledColors[1]    = ink(Ink::Led);        // playing
        s.ledColors[2]    = ink(Ink::DimAmber);   // releasing
        break;
      }

      case STYLE_STEPPER:
      case STYLE_PRESET:
      {
        auto & s = ((uiStepper *) object)->stepperStyle();
        s.backgroundColor                = ink(Ink::Panel);
        s.buttonBackgroundColor          = ink(Ink::Background);
        s.mouseDownButtonBackgroundColor = ink(Ink::Amber);
        s.buttonBorderColor              = ink(Ink::Edge);
        s.arrowColor                     = ink(Ink::Text);
        s.mouseDownArrowColor            = ink(Ink::Background);
        s.textFont                       = styleClassID == STYLE_PRESET ? MediumFont : BigFont;
        s.textColor                      = ink(Ink::Amber);
        break;
      }

      case STYLE_BUTTON:
      {
        auto button = (uiButton *) object;
        button->windowStyle().borderColor        = ink(Ink::Edge);
        button->windowStyle().activeBorderColor  = ink(Ink::Edge);
        button->windowStyle().focusedBorderColor = ink(Ink::Edge);
        button->windowStyle().borderSize         = 1;
        auto & s = button->buttonStyle();
        s.backgroundColor          = ink(Ink::Background);
        s.mouseOverBackgroundColor = ink(Ink::Panel);
        s.mouseDownBackgroundColor = ink(Ink::Amber);
        s.downBackgroundColor      = ink(Ink::Amber);
        s.textColor                = ink(Ink::Text);
        s.mouseOverTextColor       = ink(Ink::Amber);
        s.downTextColor            = ink(Ink::Background);
        s.textFont                 = SmallFont;
        break;
      }

      case STYLE_DIALOG:
      {
        auto frame = (uiFrame *) object;
        frame->windowStyle().borderColor        = ink(Ink::Amber);
        frame->windowStyle().activeBorderColor  = ink(Ink::Amber);
        frame->windowStyle().focusedBorderColor = ink(Ink::Amber);
        frame->windowStyle().borderSize         = 1;
        auto & s = frame->frameStyle();
        s.backgroundColor            = ink(Ink::Panel);
        s.titleBackgroundColor       = ink(Ink::Background);
        s.activeTitleBackgroundColor = ink(Ink::Background);
        s.titleColor                 = ink(Ink::Amber);
        s.activeTitleColor           = ink(Ink::Amber);
        s.titleFont                  = SmallFont;
        break;
      }

      case STYLE_DIALOG_LABEL:
      {
        auto & s = ((uiStaticLabel *) object)->labelStyle();
        s.backgroundColor = ink(Ink::Panel);
        s.textColor       = ink(Ink::Text);
        s.textFont        = SmallFont;
        break;
      }

      case STYLE_EDIT:
      {
        auto edit = (uiTextEdit *) object;
        edit->windowStyle().borderColor        = ink(Ink::Edge);
        edit->windowStyle().activeBorderColor  = ink(Ink::Edge);
        edit->windowStyle().focusedBorderColor = ink(Ink::Amber);
        edit->windowStyle().borderSize         = 1;
        auto & s = edit->textEditStyle();
        s.backgroundColor          = ink(Ink::Background);
        s.mouseOverBackgroundColor = ink(Ink::Background);
        s.focusedBackgroundColor   = ink(Ink::Background);
        s.textColor                = ink(Ink::Amber);
        s.textFont                 = MediumFont;
        break;
      }

      case STYLE_TILES:
      {
        auto & s = ((uiTileSelector *) object)->tileSelectorStyle();
        s.backgroundColor         = ink(Ink::Panel);
        s.tileBackgroundColor     = ink(Ink::Background);
        s.tileBorderColor         = ink(Ink::Edge);
        s.selectedTileBorderColor = ink(Ink::Amber);
        s.textFont                = SmallFont;
        s.textColor               = ink(Ink::Dim);
        s.selectedTextColor       = ink(Ink::Amber);
        break;
      }

      case STYLE_GRAPH:
      {
        auto graph = (uiGraph *) object;
        graph->windowStyle().borderColor = ink(Ink::Edge);
        auto & s = graph->graphStyle();
        s.backgroundColor = ink(Ink::Background);
        s.gridColor       = ink(Ink::Grid);
        s.lineColor       = ink(Ink::Cyan);
        s.fillColor       = ink(Ink::Fill);
        break;
      }

      case STYLE_PIANO:
      {
        auto & s = ((uiPianoKeyboard *) object)->pianoKeyboardStyle();
        s.backgroundColor      = ink(Ink::Background);
        s.topLineColor         = ink(Ink::DimAmber);
        s.whiteKeyColor        = ink(Ink::White);
        s.whiteKeyFrontColor   = ink(Ink::KeyShade);
        s.blackKeyColor        = ink(Ink::Background);
        s.blackKeyBevelColor   = ink(Ink::Shadow);
        s.pressedKeyColor      = ink(Ink::Amber);
        s.pressedKeyFrontColor = ink(Ink::DimAmber);
        s.textFont             = SmallFont;
        s.labelColor           = ink(Ink::Dim);
        s.blackLabelColor      = ink(Ink::KeyShade);
        s.noteNameColor        = ink(Ink::KeyShade);
        s.cNoteNameColor       = ink(Ink::Fill);
        s.pressedTextColor     = ink(Ink::Background);
        break;
      }

      case STYLE_WAVE_NAME:
      {
        auto & s = ((uiLabel *) object)->labelStyle();
        s.backgroundColor = ink(Ink::Panel);
        s.textFont        = MediumFont;
        s.textColor       = ink(Ink::Amber);
        s.textAlign       = uiHAlign::Center;
        break;
      }

      default:
        break;
    }
  }

} synthStyle;



////////////////////////////////////////////////////////////////////////////////////////////////////
// Drawing helpers (main frame background, graph overlays)


enum class Align { Left, Center, Right };

// x is the left side, the center or the right side of the text, depending on "align"
static void drawText(Canvas * cv, fabgl::FontInfo const * font, Ink color, int x, int y, char const * text, Align align = Align::Left, bool bold = false)
{
  int width = cv->textExtent(font, text) + (bold ? strlen(text) : 0);
  if (align == Align::Center)
    x -= width / 2;
  else if (align == Align::Right)
    x -= width;
  cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(bold));
  cv->setPenColor(ink(color));
  cv->drawText(font, x, y, text);
}

static void fillRect(Canvas * cv, Ink color, Rect const & rect)
{
  cv->setBrushColor(ink(color));
  cv->fillRectangle(rect);
}

static void drawLine(Canvas * cv, Ink color, int x1, int y1, int x2, int y2)
{
  cv->setPenColor(ink(color));
  cv->drawLine(x1, y1, x2, y2);
}

// a panel with rounded corners, a title and an optional note on the right
static void drawPanel(Canvas * cv, Rect const & r, char const * title, char const * note = nullptr)
{
  fillRect(cv, Ink::Panel, r);
  cv->setPenColor(ink(Ink::Edge));
  cv->drawRectangle(r);
  cv->setPenColor(ink(Ink::Background));
  cv->setPixel(r.X1, r.Y1);
  cv->setPixel(r.X2, r.Y1);
  cv->setPixel(r.X1, r.Y2);
  cv->setPixel(r.X2, r.Y2);
  if (title) {
    fillRect(cv, Ink::Amber, Rect(r.X1 + 8, r.Y1 + 9, r.X1 + 11, r.Y1 + 12));
    drawText(cv, SmallFont, Ink::Text, r.X1 + 17, r.Y1 + 5, title, Align::Left, true);
  }
  if (note)
    drawText(cv, SmallFont, Ink::Dim, r.X2 - 8, r.Y1 + 5, note, Align::Right);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// PS/2 keyboard mapping


// virtual key -> keyboard index (semitones from the leftmost C), -1 = not a note key
static int keyToIndex(VirtualKey vk)
{
  using namespace fabgl;
  switch (vk) {
    case VK_a: case VK_A:                   return 0;   // C
    case VK_w: case VK_W:                   return 1;   // C#
    case VK_s: case VK_S:                   return 2;   // D
    case VK_e: case VK_E:                   return 3;   // D#
    case VK_d: case VK_D:                   return 4;   // E
    case VK_f: case VK_F:                   return 5;   // F
    case VK_t: case VK_T:                   return 6;   // F#
    case VK_g: case VK_G:                   return 7;   // G
    case VK_y: case VK_Y:                   return 8;   // G#
    case VK_h: case VK_H:                   return 9;   // A
    case VK_u: case VK_U:                   return 10;  // A#
    case VK_j: case VK_J:                   return 11;  // B
    case VK_k: case VK_K:                   return 12;  // C
    case VK_o: case VK_O:                   return 13;  // C#
    case VK_l: case VK_L:                   return 14;  // D
    case VK_p: case VK_P:                   return 15;  // D#
    case VK_SEMICOLON: case VK_COLON:       return 16;  // E  (US layout)
    case VK_GRAVE_o:                        return 16;  // E  (IT layout)
    case VK_QUOTE: case VK_QUOTEDBL:        return 17;  // F  (US layout)
    case VK_GRAVE_a:                        return 17;  // F  (IT layout)
    case VK_RIGHTBRACKET:                   return 18;  // F# (US layout)
    case VK_PLUS:                           return 18;  // F# (IT layout)
    default:                                return -1;
  }
}


static constexpr int PIANO_KEYS = 25;   // two octaves plus the final C

// labels drawn on the on-screen keys
static char const * KeyLabels[PIANO_KEYS] = {
  "A", "W", "S", "E", "D", "F", "T", "G", "Y", "H", "U", "J",
  "K", "O", "L", "P", ";", "'", "]",
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Main frame
//
//   +--------------------------------------------------------------+
//   | FabGL SYNTH  8-VOICE POLYPHONIC SYNTHESIZER    VOICES ######## |  header
//   | [OSCILLATOR/LFO  ] [ENVELOPE        ] [LOW-PASS FILTER     ] |  panels: graph + controls
//   | [PRESET  SAVE/RESET  |  OCTAVE  |  MASTER VOLUME            ] |  strip
//   | [ piano keyboard                                            ] |
//   +--------------------------------------------------------------+


class SynthFrame : public uiFrame {

public:

  SynthFrame(uiWindow * parent)
    : uiFrame(parent, "", Point(0, 0), parent->clientSize())
  {
    frameProps().resizeable        = false;
    frameProps().moveable          = false;
    frameProps().hasCloseButton    = false;
    frameProps().hasMaximizeButton = false;
    frameProps().hasMinimizeButton = false;
    frameStyle().backgroundColor   = ink(Ink::Background);
    windowStyle().borderSize       = 0;

    onPaint = [&]() { paintBackground(); };

    // voices activity
    int ledsW = synth.voicesCount() * 16 - 4;
    m_voiceLeds = new uiLedBar(this, Point(624 - ledsW, 19), Size(ledsW, 8), synth.voicesCount(), true, STYLE_LEDBAR);
    onTimer = [&](uiTimerHandle) {
      updateVoiceLeds();
      updatePresetModified();
    };


    createOscillatorPanel(panelRect(0));
    createEnvelopePanel(panelRect(1));
    createFilterPanel(panelRect(2));
    createStrip(stripRect());

    // on screen keyboard
    m_piano = new uiPianoKeyboard(this, Point(12, 310), Size(616, 162), PIANO_KEYS, true, STYLE_PIANO);
    m_piano->setKeyLabels(KeyLabels);
    m_piano->onNoteOn  = [&](int note) { synth.noteOn(note); };
    m_piano->onNoteOff = [&](int note) { synth.noteOff(note); };
    updateOctave();

    // PS/2 keyboard
    onKeyDown = [&](uiKeyEventInfo const & key) {
      int index = keyToIndex(key.VK);
      if (index >= 0) {
        m_piano->press(index, uiPianoKeyboard::KeyboardSource);
        return;
      }
      switch (key.VK) {
        case fabgl::VK_z: case fabgl::VK_Z:
          changeOctave(-1);
          break;
        case fabgl::VK_x: case fabgl::VK_X:
          changeOctave(+1);
          break;
        case fabgl::VK_1: case fabgl::VK_2: case fabgl::VK_3: case fabgl::VK_4: case fabgl::VK_5:
          selectWaveform(key.VK - fabgl::VK_1);
          break;
        case fabgl::VK_PAGEUP:
          selectPreset(m_presetIndex - 1);
          break;
        case fabgl::VK_PAGEDOWN:
          selectPreset(m_presetIndex + 1);
          break;
        default:
          break;
      }
    };

    onKeyUp = [&](uiKeyEventInfo const & key) {
      int index = keyToIndex(key.VK);
      if (index >= 0)
        m_piano->release(index, uiPianoKeyboard::KeyboardSource);
    };
  }

private:

  // layout
  static constexpr int PANEL_Y = 50;
  static constexpr int PANEL_W = 204;
  static constexpr int PANEL_H = 182;
  static constexpr int GRAPH_H = 64;
  static constexpr int PanelCols[3] = { 8, 218, 428 };
  static constexpr int StripCols[3] = { 18, 330, 484 };

  static Rect panelRect(int index)          { return Rect(PanelCols[index], PANEL_Y, PanelCols[index] + PANEL_W - 1, PANEL_Y + PANEL_H - 1); }
  static Rect stripRect()                   { return Rect(8, PANEL_Y + PANEL_H + 6, 631, PANEL_Y + PANEL_H + 6 + 63); }
  static int  controlsY()                   { return PANEL_Y + 24 + GRAPH_H + 8; }

  uiGraph * addGraph(Rect const & panel)
  {
    return new uiGraph(this, Point(panel.X1 + 8, panel.Y1 + 24), Size(PANEL_W - 16, GRAPH_H), true, STYLE_GRAPH);
  }

  // a knob in the controls area of a panel. "apply" is called when the value changes
  template <typename Apply>
  uiKnob * addKnob(int x, int width, char const * label, int min, int max, int value, Apply apply)
  {
    auto knob = new uiKnob(this, label, Point(x, controlsY()), Size(width, 76), true, STYLE_KNOB);
    knob->onChange = [=]() { apply(knob); };
    knob->setup(min, max, value);
    return knob;
  }


  //// oscillator ////

  void createOscillatorPanel(Rect const & panel)
  {
    m_waveGraph = addGraph(panel);
    m_waveGraph->setRange(-127, 127);

    m_waveTiles = new uiTileSelector(this, Point(panel.X1 + 8, controlsY()), Size(PANEL_W - 16, 46), true, STYLE_TILES);
    for (int i = 0; i < 5; ++i)
      m_waveTiles->items().appendFmt("%d", i + 1);
    m_waveTiles->onPaintTile = [&](int index, Rect const & r, bool selected) { paintWaveIcon(index, r, selected); };
    m_waveTiles->onChange    = [&]() { selectWaveform(m_waveTiles->selectedItem()); };

    m_waveName = new uiLabel(this, "", Point(panel.X1 + 8, controlsY() + 50), Size(PANEL_W - 16, 22), true, STYLE_WAVE_NAME);

    selectWaveform((int) synth.waveform());

    createLFOPage(panel);

    // the panel shows the oscillator (OSC) or the LFO
    m_oscSelector = new uiTileSelector(this, Point(panel.X2 - 8 - 92, panel.Y1 + 4), Size(92, 16), true, STYLE_TILES);
    m_oscSelector->items().append("");
    m_oscSelector->items().append("");
    m_oscSelector->onPaintTile = [&](int index, Rect const & r, bool selected) {
      drawText(m_oscSelector->canvas(), SmallFont, selected ? Ink::Amber : Ink::Dim, (r.X1 + r.X2) / 2, r.Y1 + 2, index ? "LFO" : "OSC", Align::Center, true);
    };
    m_oscSelector->onChange = [&]() { selectOscillatorPage(m_oscSelector->selectedItem()); };
    selectOscillatorPage(0);
  }

  // 0 = oscillator, 1 = LFO
  void selectOscillatorPage(int index)
  {
    bool lfo = index == 1;
    m_oscSelector->selectItem(index);
    uiWindow * oscPage[] = { m_waveGraph, m_waveTiles, m_waveName };
    uiWindow * lfoPage[] = { m_lfoTiles, m_lfoGraph, m_lfoKnobs[0], m_lfoKnobs[1], m_lfoKnobs[2] };
    for (auto w : oscPage)
      app()->showWindow(w, !lfo);
    for (auto w : lfoPage)
      app()->showWindow(w, lfo);
  }


  //// LFO (second page of the oscillator panel) ////

  void createLFOPage(Rect const & panel)
  {
    static constexpr int TILES_H = 24;

    // LFO waveform tiles and shape, in the graph area
    m_lfoTiles = new uiTileSelector(this, Point(panel.X1 + 8, panel.Y1 + 24), Size(PANEL_W - 16, TILES_H), true, STYLE_TILES);
    for (int i = 0; i < 5; ++i)
      m_lfoTiles->items().append("");
    m_lfoTiles->onPaintTile = [&](int index, Rect const & r, bool selected) { paintLFOIcon(index, r, selected); };
    m_lfoTiles->onChange    = [&]() { selectLFOWaveform(m_lfoTiles->selectedItem()); };

    m_lfoGraph = new uiGraph(this, Point(panel.X1 + 8, panel.Y1 + 24 + TILES_H + 4), Size(PANEL_W - 16, GRAPH_H - TILES_H - 4), true, STYLE_GRAPH);
    m_lfoGraph->setRange(-127, 127);
    m_lfoGraph->onPaint = [&](Rect const & plot) {
      drawText(m_lfoGraph->canvas(), TinyFont, Ink::Dim, plot.X2 - 2, plot.Y1 + 2, SynthLFO::waveformName(synth.lfo().waveform()), Align::Right);
    };

    // rate (logarithmic scale), cutoff and amplitude modulation depths
    int kw = (PANEL_W - 16) / 3, kx = panel.X1 + 8;
    m_lfoKnobs[0] = addKnob(kx, kw, "RATE", 0, 100, 60, [&](uiKnob * k) {
      synth.lfo().setRate(SynthLFO::rateFromPosition(k->position()));
      int r10 = (int) lroundf(synth.lfo().rate() * 10);
      k->setValueTextFmt("%d.%d Hz", r10 / 10, r10 % 10);
    });
    m_lfoKnobs[1] = addKnob(kx + kw, kw, "CUTOFF", 0, 100, 0, [&](uiKnob * k) {
      synth.lfo().setCutoffDepth(k->position());
      k->setValueTextFmt("%d %%", k->position());
    });
    m_lfoKnobs[2] = addKnob(kx + kw * 2, kw, "AMP", 0, 100, 0, [&](uiKnob * k) {
      synth.lfo().setAmpDepth(k->position());
      k->setValueTextFmt("%d %%", k->position());
    });

    selectLFOWaveform((int) synth.lfo().waveform());
  }

  void paintLFOIcon(int index, Rect const & r, bool selected)
  {
    auto cv  = m_lfoTiles->canvas();
    int  x1  = r.X1 + 5, x2 = r.X2 - 5, cy = (r.Y1 + r.Y2) / 2, amp = 7;
    int  px  = x1, py = cy - SynthLFO::waveformShape((LFOWaveform) index, 0) * amp / 127;
    cv->setPenColor(ink(selected ? Ink::Amber : Ink::Text));
    for (int x = x1 + 1; x <= x2; ++x) {
      int y = cy - SynthLFO::waveformShape((LFOWaveform) index, (x - x1) * 255 / (x2 - x1)) * amp / 127;
      cv->drawLine(px, py, x, y);
      px = x;
      py = y;
    }
  }

  void selectLFOWaveform(int index)
  {
    if (index < 0 || index > (int) LFOWaveform::Random)
      return;
    if (index == (int) synth.lfo().waveform() && index == m_lfoTiles->selectedItem())
      return;   // nothing changes: no repaint
    auto waveform = (LFOWaveform) index;
    synth.lfo().setWaveform(waveform);
    m_lfoTiles->selectItem(index);
    // two cycles
    int count = m_lfoGraph->plotRect().width();
    int16_t values[count];
    for (int i = 0; i < count; ++i)
      values[i] = SynthLFO::waveformShape(waveform, i * 512 / count);
    m_lfoGraph->setValues(values, count);
  }

  // one cycle of the waveform inside a tile
  void paintWaveIcon(int index, Rect const & r, bool selected)
  {
    auto cv  = m_waveTiles->canvas();
    int  x1  = r.X1 + 5, x2 = r.X2 - 5, cy = (r.Y1 + r.Y2) / 2, amp = 7;
    int  px  = x1, py = cy - SynthVoice::waveformShape((SynthWaveform) index, 0) * amp / 127;
    cv->setPenColor(ink(selected ? Ink::Amber : Ink::Text));
    for (int x = x1 + 1; x <= x2; ++x) {
      int y = cy - SynthVoice::waveformShape((SynthWaveform) index, (x - x1) * 255 / (x2 - x1)) * amp / 127;
      cv->drawLine(px, py, x, y);
      px = x;
      py = y;
    }
  }

  void selectWaveform(int index)
  {
    if (index < 0 || index > (int) SynthWaveform::Sawtooth)
      return;
    if (index == (int) synth.waveform() && index == m_waveTiles->selectedItem())
      return;   // nothing changes: no repaint
    auto waveform = (SynthWaveform) index;
    synth.setWaveform(waveform);
    m_waveTiles->selectItem(index);
    m_waveName->setText(SynthVoice::waveformName(waveform));
    // two cycles
    int count = m_waveGraph->plotRect().width();
    int16_t values[count];
    for (int i = 0; i < count; ++i)
      values[i] = SynthVoice::waveformShape(waveform, i * 512 / count);
    m_waveGraph->setValues(values, count);
  }


  //// envelope ////

  void createEnvelopePanel(Rect const & panel)
  {
    m_envGraph = addGraph(panel);
    m_envGraph->setRange(0, 100);
    m_envGraph->graphProps().fillArea   = true;
    m_envGraph->graphProps().hGridLines = 0;
    m_envGraph->graphProps().vGridLines = 0;
    m_envGraph->onPaintBackground = [&](Rect const & plot) { paintEnvelopeBackground(plot); };
    m_envGraph->onPaint           = [&](Rect const & plot) { paintEnvelopeOverlay(plot); };

    // the knobs edit the selected envelope: amplitude (AMP) or filter cutoff (FILTER)
    m_envSelector = new uiTileSelector(this, Point(panel.X2 - 8 - 92, panel.Y1 + 4), Size(92, 16), true, STYLE_TILES);
    m_envSelector->items().append("");
    m_envSelector->items().append("");
    m_envSelector->onPaintTile = [&](int index, Rect const & r, bool selected) {
      drawText(m_envSelector->canvas(), SmallFont, selected ? Ink::Amber : Ink::Dim, (r.X1 + r.X2) / 2, r.Y1 + 2, index ? "FILTER" : "AMP", Align::Center, true);
    };
    m_envSelector->onChange = [&]() { selectEnvelope(m_envSelector->selectedItem()); };

    int kw = (PANEL_W - 16) / 4, kx = panel.X1 + 8;
    m_envKnobs[0] = addKnob(kx + kw * 0, kw, "ATTACK",  0, 1000,  10, [&](uiKnob * k) { currentEnvelope().setAttack(k->position());  k->setValueTextFmt("%d ms", k->position()); updateEnvelopeGraph(); });
    m_envKnobs[1] = addKnob(kx + kw * 1, kw, "DECAY",   0, 1000, 200, [&](uiKnob * k) { currentEnvelope().setDecay(k->position());   k->setValueTextFmt("%d ms", k->position()); updateEnvelopeGraph(); });
    m_envKnobs[2] = addKnob(kx + kw * 2, kw, "SUSTAIN", 0, 100,   70, [&](uiKnob * k) { currentEnvelope().setSustain(k->position()); k->setValueTextFmt("%d %%", k->position()); updateEnvelopeGraph(); });
    m_envKnobs[3] = addKnob(kx + kw * 3, kw, "RELEASE", 0, 2000, 300, [&](uiKnob * k) { currentEnvelope().setRelease(k->position()); k->setValueTextFmt("%d ms", k->position()); updateEnvelopeGraph(); });

    selectEnvelope(0);
  }

  ADSREnvelope & currentEnvelope() { return m_editFilterEnvelope ? synth.filterEnvelope() : synth.envelope(); }

  // 0 = amplitude envelope, 1 = filter envelope
  void selectEnvelope(int index)
  {
    m_editFilterEnvelope = index == 1;
    m_envSelector->selectItem(index);
    auto & env = currentEnvelope();
    int values[4] = { env.attack(), env.decay(), env.sustain(), env.release() };
    for (int i = 0; i < 4; ++i) {
      m_envKnobs[i]->setPosition(values[i]);
      m_envKnobs[i]->onChange();     // value text, also when the position doesn't change
    }
    updateEnvelopeGraph();
  }

  // envelope shape on a fixed time scale: the shape changes as the times change
  void updateEnvelopeGraph()
  {
    if (m_batchUpdate) {
      m_envGraphDirty = true;
      return;
    }
    static constexpr int SUSTAIN_W = 30;     // width of the sustain segment
    static constexpr int MAX_MS    = 4000;   // max attack + decay + release

    auto & env = currentEnvelope();
    int w      = m_envGraph->plotRect().width();
    int avail  = w - 1 - SUSTAIN_W;
    int a      = env.attack()  * avail / MAX_MS;
    int d      = env.decay()   * avail / MAX_MS;
    int r      = env.release() * avail / MAX_MS;

    // vertices: x (0..w-1) and level (0..100)
    m_envX[0] = 0;          m_envY[0] = 0;
    m_envX[1] = a;          m_envY[1] = 100;
    m_envX[2] = a + d;      m_envY[2] = env.sustain();
    m_envX[3] = w - 1 - r;  m_envY[3] = env.sustain();
    m_envX[4] = w - 1;      m_envY[4] = 0;

    int16_t values[w];
    for (int x = 0, seg = 0; x < w; ++x) {
      while (seg < 3 && x > m_envX[seg + 1])
        ++seg;
      int x1 = m_envX[seg], x2 = m_envX[seg + 1];
      values[x] = x2 > x1 ? m_envY[seg] + (m_envY[seg + 1] - m_envY[seg]) * (x - x1) / (x2 - x1) : m_envY[seg + 1];
    }
    m_envGraph->setValues(values, w);
  }

  void paintEnvelopeBackground(Rect const & plot)
  {
    auto cv = m_envGraph->canvas();
    for (int i = 1; i < 4; ++i)
      drawLine(cv, Ink::Grid, plot.X1 + m_envX[i], plot.Y1, plot.X1 + m_envX[i], plot.Y2);
  }

  void paintEnvelopeOverlay(Rect const & plot)
  {
    static char const * names[4] = { "A", "D", "S", "R" };
    auto cv = m_envGraph->canvas();
    for (int i = 1; i < 4; ++i) {
      int x = plot.X1 + m_envX[i], y = m_envGraph->valueToY(m_envY[i]);
      fillRect(cv, Ink::Amber, Rect(x - 1, y - 1, x + 1, y + 1));
    }
    for (int i = 0; i < 4; ++i)
      if (m_envX[i + 1] - m_envX[i] >= 8)
        drawText(cv, TinyFont, Ink::Dim, plot.X1 + (m_envX[i] + m_envX[i + 1]) / 2 + 1, plot.Y2 - 9, names[i], Align::Center);
  }


  //// filter ////

  static constexpr float MIN_FREQ = 20.0f;
  static constexpr float MAX_FREQ = 8000.0f;
  static constexpr int   MAX_DB10 = 240;     // dB * 10
  static constexpr int   MIN_DB10 = -360;

  void createFilterPanel(Rect const & panel)
  {
    m_filterGraph = addGraph(panel);
    m_filterGraph->setRange(MIN_DB10, MAX_DB10);
    m_filterGraph->graphProps().fillArea   = true;
    m_filterGraph->graphProps().hGridLines = 0;
    m_filterGraph->graphProps().vGridLines = 0;
    m_filterGraph->onPaintBackground = [&](Rect const & plot) { paintFilterBackground(plot); };
    m_filterGraph->onPaint           = [&](Rect const & plot) { paintFilterOverlay(plot); };

    // knobs 0..100 (cutoff on a logarithmic scale)
    int kw = (PANEL_W - 16) / 3, kx = panel.X1 + 8;
    m_cutoffKnob = addKnob(kx, kw, "CUTOFF", 0, 100, 70, [&](uiKnob * k) {
      synth.filter().setCutoff(ResonantLowPassFilter::cutoffFromPosition(k->position()));
      k->setValueTextFmt("%d Hz", synth.filter().cutoff());
      updateFilterGraph();
    });
    m_resonanceKnob = addKnob(kx + kw, kw, "RESONANCE", 0, 100, 50, [&](uiKnob * k) {
      synth.filter().setResonance(ResonantLowPassFilter::resonanceFromPosition(k->position()));
      k->setValueTextFmt("%d %%", k->position());
      updateFilterGraph();
    });
    // how much the filter envelope moves the cutoff (100% = the whole range, about 7 octaves)
    m_envAmountKnob = addKnob(kx + kw * 2, kw, "ENV AMT", -100, 100, 40, [&](uiKnob * k) {
      synth.filter().setEnvelopeAmount(k->position());
      k->setValueTextFmt("%+d %%", k->position());
    });

    m_filterSwitch = new uiToggleSwitch(this, Point(panel.X2 - 8 - 64, panel.Y1 + 4), Size(64, 16), synth.filter().enabled(), true, STYLE_SWITCH);
    m_filterSwitch->onChange = [&]() { enableFilter(m_filterSwitch->checked()); };
  }

  void enableFilter(bool value)
  {
    if (value == synth.filter().enabled() && value == m_filterSwitch->checked())
      return;   // nothing changes: no repaint
    synth.filter().setEnabled(value);
    m_filterSwitch->setChecked(value);
    updateFilterGraph();
  }

  int freqToX(Rect const & plot, float freq) { return plot.X1 + (int) lroundf(logf(freq / MIN_FREQ) / logf(MAX_FREQ / MIN_FREQ) * (plot.width() - 1)); }

  // response in dB * 10 for each column of the plot area
  void updateFilterGraph()
  {
    if (m_batchUpdate) {
      m_filterGraphDirty = true;
      return;
    }
    auto & filter = synth.filter();
    if (!filter.enabled()) {
      m_filterGraph->setValues(nullptr, 0);
      return;
    }
    int w = m_filterGraph->plotRect().width();
    int16_t values[w];
    for (int x = 0; x < w; ++x) {
      float freq = MIN_FREQ * powf(MAX_FREQ / MIN_FREQ, (float) x / (w - 1));
      values[x] = fabgl::iclamp((int) lroundf(filter.responseDB(freq) * 10), MIN_DB10, MAX_DB10);
    }
    m_filterGraph->setValues(values, w);
  }

  // grid (100 Hz, 1 KHz, 0 dB) and cutoff marker
  void paintFilterBackground(Rect const & plot)
  {
    auto cv  = m_filterGraph->canvas();
    int x100 = freqToX(plot, 100), x1k = freqToX(plot, 1000), y0 = m_filterGraph->valueToY(0);
    drawLine(cv, Ink::Grid, x100, plot.Y1, x100, plot.Y2);
    drawLine(cv, Ink::Grid, x1k, plot.Y1, x1k, plot.Y2);
    drawLine(cv, Ink::Grid, plot.X1, y0, plot.X2, y0);
    if (synth.filter().enabled()) {
      int xc = freqToX(plot, synth.filter().cutoff());
      drawLine(cv, Ink::DimAmber, xc, plot.Y1, xc, plot.Y2);
    }
  }

  void paintFilterOverlay(Rect const & plot)
  {
    auto cv  = m_filterGraph->canvas();
    int x100 = freqToX(plot, 100), x1k = freqToX(plot, 1000);
    drawText(cv, TinyFont, Ink::Dim, x100 + 3, plot.Y2 - 8, "100");
    drawText(cv, TinyFont, Ink::Dim, x1k + 3, plot.Y2 - 8, "1K");
    if (!synth.filter().enabled()) {
      int y0 = m_filterGraph->valueToY(0);
      drawLine(cv, Ink::Shadow, plot.X1, y0, plot.X2, y0);
      drawText(cv, SmallFont, Ink::Dim, (plot.X1 + plot.X2) / 2, plot.Y1 + 4, "BYPASS", Align::Center, true);
    }
  }


  //// strip: presets, octave, master volume ////

  void createStrip(Rect const & strip)
  {
    int y = strip.Y1 + 22;

    m_presetStepper = new uiStepper(this, Point(StripCols[0], y), Size(232, 36), true, STYLE_PRESET);
    m_presetStepper->onStep = [&](int delta) { selectPreset(m_presetIndex + delta); };

    auto save = new uiButton(this, "SAVE", Point(StripCols[0] + 238, y), Size(56, 17), uiButtonKind::Button, true, STYLE_BUTTON);
    save->onClick = [&]() { savePresetDialog(); };

    auto reset = new uiButton(this, "RESET", Point(StripCols[0] + 238, y + 19), Size(56, 17), uiButtonKind::Button, true, STYLE_BUTTON);
    reset->onClick = [&]() { applyPreset(FactoryPresets[m_presetIndex]); };

    m_octave = new uiStepper(this, Point(StripCols[1], y), Size(134, 36), true, STYLE_STEPPER);
    m_octave->onStep = [&](int delta) { changeOctave(delta); };

    auto volume = new uiLevelBar(this, Point(StripCols[2], y + 8), Size(strip.X2 - 10 - StripCols[2], 20), true, STYLE_LEVELBAR);
    volume->setRange(0, 127);
    volume->onChange = [=]() { soundGenerator.setVolume(volume->position()); };
    volume->setPosition(100);
  }


  //// presets ////

  static constexpr char const * PREFS_NAMESPACE = "synth";
  static constexpr int          PREFS_VERSION   = 1;     // change it when the Preset layout changes

public:

  // factory presets, replaced by the ones saved to NVS. Starts with preset 01
  void loadPresets()
  {
    memcpy(m_presets, FactoryPresets, sizeof(m_presets));
    fabgl::Preferences prefs;
    if (prefs.begin(PREFS_NAMESPACE, true)) {
      if (prefs.getInt("version", 0) == PREFS_VERSION) {
        for (int i = 0; i < PRESETS_COUNT; ++i) {
          char key[8];
          snprintf(key, sizeof(key), "p%02d", i);
          Preset preset;
          if (prefs.getBytes(key, &preset, sizeof(Preset)) == sizeof(Preset)) {
            preset.name[PRESET_NAME_SIZE - 1] = 0;
            m_presets[i] = preset;
          }
        }
      }
      prefs.end();
    }
    m_presetIndex = -1;
    selectPreset(0);
  }

private:

  // loads a preset in the controls (the current changes, if any, are lost)
  void selectPreset(int index)
  {
    m_presetIndex = (index + PRESETS_COUNT) % PRESETS_COUNT;
    applyPreset(m_presets[m_presetIndex]);
  }

  // Sets the controls (and so the synth) to the preset sound.
  // Only the controls whose value changes are updated, and the graphs are recalculated once at the end: many
  // repaints would take several frames, showing the controls erased and redrawn (flickering).
  void applyPreset(Preset const & p)
  {
    synth.allNotesOff();
    m_batchUpdate = true;

    selectWaveform(p.wave);

    auto & env = synth.envelope();
    env.setAttack(p.attack);
    env.setDecay(p.decay);
    env.setSustain(p.sustain);
    env.setRelease(p.release);
    auto & fenv = synth.filterEnvelope();
    fenv.setAttack(p.fAttack);
    fenv.setDecay(p.fDecay);
    fenv.setSustain(p.fSustain);
    fenv.setRelease(p.fRelease);
    auto & shown = currentEnvelope();
    setKnob(m_envKnobs[0], shown.attack());
    setKnob(m_envKnobs[1], shown.decay());
    setKnob(m_envKnobs[2], shown.sustain());
    setKnob(m_envKnobs[3], shown.release());
    updateEnvelopeGraph();

    enableFilter(p.filterOn);
    setKnob(m_cutoffKnob, p.cutoff);
    setKnob(m_resonanceKnob, p.resonance);
    setKnob(m_envAmountKnob, p.envAmount);

    selectLFOWaveform(p.lfoWave);
    setKnob(m_lfoKnobs[0], p.lfoRate);
    setKnob(m_lfoKnobs[1], p.lfoCutoff);
    setKnob(m_lfoKnobs[2], p.lfoAmp);

    m_batchUpdate = false;
    if (m_envGraphDirty)
      updateEnvelopeGraph();
    if (m_filterGraphDirty)
      updateFilterGraph();
    m_envGraphDirty = m_filterGraphDirty = false;

    m_presetModified = -1;   // updatePresetModified() updates the text
    updatePresetModified();
  }

  // Sets a knob (its onChange applies the value). Knobs and synth parameters are always in sync: a knob already
  // at the value needs nothing (no repaint)
  static void setKnob(uiKnob * knob, int value)
  {
    if (knob->position() != value)
      knob->setPosition(value);
  }

  // the current sound, from the controls
  void capturePreset(Preset & p)
  {
    memset(&p, 0, sizeof(Preset));
    strlcpy(p.name, m_presets[m_presetIndex].name, PRESET_NAME_SIZE);
    p.wave      = (uint8_t) synth.waveform();
    auto & env  = synth.envelope();
    p.attack    = env.attack();
    p.decay     = env.decay();
    p.sustain   = env.sustain();
    p.release   = env.release();
    auto & fenv = synth.filterEnvelope();
    p.fAttack   = fenv.attack();
    p.fDecay    = fenv.decay();
    p.fSustain  = fenv.sustain();
    p.fRelease  = fenv.release();
    p.filterOn  = synth.filter().enabled();
    p.cutoff    = m_cutoffKnob->position();
    p.resonance = m_resonanceKnob->position();
    p.envAmount = m_envAmountKnob->position();
    p.lfoWave   = (uint8_t) synth.lfo().waveform();
    p.lfoRate   = m_lfoKnobs[0]->position();
    p.lfoCutoff = m_lfoKnobs[1]->position();
    p.lfoAmp    = m_lfoKnobs[2]->position();
  }

  // shows "NN NAME", with "*" when the sound differs from the stored preset
  void updatePresetModified()
  {
    Preset current;
    capturePreset(current);
    int modified = current.sameSound(m_presets[m_presetIndex]) ? 0 : 1;
    if (modified != m_presetModified) {
      m_presetModified = modified;
      char text[32];
      snprintf(text, sizeof(text), "%02d %s%s", m_presetIndex + 1, m_presets[m_presetIndex].name, modified ? " *" : "");
      m_presetStepper->setText(text);
    }
  }

  void savePresetDialog()
  {
    static constexpr int W = 300, H = 160;
    auto dialog = new uiFrame(app()->rootWindow(), "SAVE PRESET", Point((640 - W) / 2, 120), Size(W, H), false, STYLE_DIALOG);
    dialog->frameProps().resizeable        = false;
    dialog->frameProps().hasMaximizeButton = false;
    dialog->frameProps().hasMinimizeButton = false;

    // destination
    new uiStaticLabel(dialog, "DESTINATION", Point(14, 28), true, STYLE_DIALOG_LABEL);
    int slot = m_presetIndex;
    auto slotStepper = new uiStepper(dialog, Point(14, 44), Size(W - 28, 30), true, STYLE_PRESET);
    auto updateSlot = [=, this](int value) {
      char text[32];
      snprintf(text, sizeof(text), "%02d %s", value + 1, m_presets[value].name);
      slotStepper->setText(text);
    };
    updateSlot(slot);
    slotStepper->onStep = [&, updateSlot](int delta) {
      slot = (slot + delta + PRESETS_COUNT) % PRESETS_COUNT;
      updateSlot(slot);
    };

    // name
    new uiStaticLabel(dialog, "NAME", Point(14, 82), true, STYLE_DIALOG_LABEL);
    auto edit = new uiTextEdit(dialog, m_presets[m_presetIndex].name, Point(14, 98), Size(W - 28, 24), true, STYLE_EDIT);

    auto ok = new uiButton(dialog, "SAVE", Point(W - 14 - 140 - 8, H - 30), Size(70, 20), uiButtonKind::Button, true, STYLE_BUTTON);
    ok->onClick = [=]() { dialog->exitModal(1); };
    auto cancel = new uiButton(dialog, "CANCEL", Point(W - 14 - 70, H - 30), Size(70, 20), uiButtonKind::Button, true, STYLE_BUTTON);
    cancel->onClick = [=]() { dialog->exitModal(0); };

    app()->setFocusedWindow(edit);
    if (app()->showModalWindow(dialog) == 1) {
      // name: upper case, at most 16 characters, "NONAME" when empty
      char name[PRESET_NAME_SIZE] = { };
      int len = 0;
      for (auto c = edit->text(); *c && len < PRESET_NAME_SIZE - 1; ++c)
        name[len++] = toupper(*c);
      while (len > 0 && name[len - 1] == ' ')
        name[--len] = 0;
      savePreset(slot, len ? name : "NONAME");
    }
    app()->destroyWindow(dialog);
    app()->setFocusedWindow(this);
  }

  // saves the current sound to a preset slot (NVS) and selects it
  void savePreset(int slot, char const * name)
  {
    Preset preset;
    capturePreset(preset);
    strlcpy(preset.name, name, PRESET_NAME_SIZE);
    m_presets[slot] = preset;
    fabgl::Preferences prefs;
    if (prefs.begin(PREFS_NAMESPACE)) {
      char key[8];
      snprintf(key, sizeof(key), "p%02d", slot);
      prefs.putBytes(key, &preset, sizeof(Preset));
      prefs.putInt("version", PREFS_VERSION);
      prefs.end();
    }
    selectPreset(slot);
  }


  //// main frame ////

  void paintBackground()
  {
    auto cv = canvas();

    // header: logo, tagline, voices
    int x = 16;
    drawText(cv, BigFont, Ink::Text, x, 10, "FabGL", Align::Left, true);
    x += cv->textExtent(BigFont, "FabGL") + 14;
    drawText(cv, BigFont, Ink::Amber, x, 10, "SYNTH", Align::Left, true);
    int logoEnd = x + cv->textExtent(BigFont, "SYNTH") + 5;
    drawText(cv, SmallFont, Ink::Dim, logoEnd + 14, 19, "8-VOICE POLYPHONIC SYNTHESIZER");
    drawText(cv, SmallFont, Ink::Dim, m_voiceLeds->pos().X - 8, 17, "VOICES", Align::Right, true);
    drawLine(cv, Ink::Edge, 8, 42, 631, 42);
    fillRect(cv, Ink::Amber, Rect(16, 41, logoEnd, 43));

    // panels
    drawPanel(cv, panelRect(0), "OSCILLATOR");
    drawPanel(cv, panelRect(1), "ENVELOPE");
    drawPanel(cv, panelRect(2), "LOW-PASS");

    // strip
    Rect r = stripRect();
    drawPanel(cv, r, nullptr);
    drawText(cv, SmallFont, Ink::Text, StripCols[0], r.Y1 + 5, "PRESET", Align::Left, true);
    drawText(cv, SmallFont, Ink::Dim,  StripCols[0] + 232, r.Y1 + 5, "PGUP / PGDN", Align::Right);
    drawText(cv, SmallFont, Ink::Text, StripCols[1], r.Y1 + 5, "OCTAVE", Align::Left, true);
    drawText(cv, SmallFont, Ink::Dim,  StripCols[1] + 134, r.Y1 + 5, "Z / X", Align::Right);
    drawText(cv, SmallFont, Ink::Text, StripCols[2], r.Y1 + 5, "MASTER VOLUME", Align::Left, true);
    for (int i = 1; i < 3; ++i)
      drawLine(cv, Ink::Edge, StripCols[i] - 10, r.Y1 + 8, StripCols[i] - 10, r.Y2 - 8);
  }

  void updateVoiceLeds()
  {
    for (int i = 0; i < synth.voicesCount(); ++i) {
      auto state = synth.voice(i).state();
      m_voiceLeds->setLed(i, state == SynthVoiceState::Idle ? 0 : (state == SynthVoiceState::Release ? 2 : 1));
    }
  }

  void changeOctave(int delta)
  {
    int base = m_piano->baseNote() + delta * 12;
    // leftmost C from C1 (MIDI 24) to C6 (MIDI 84): the rightmost key reaches C8
    if (base >= 24 && base <= 84) {
      m_piano->setBaseNote(base);
      updateOctave();
    }
  }

  void updateOctave()
  {
    char str[8];
    m_octave->setText(fabgl::midiNoteName(m_piano->baseNote(), str, sizeof(str)));
  }

  uiPianoKeyboard * m_piano;
  uiTileSelector *  m_waveTiles;
  uiLabel *         m_waveName;
  uiGraph *         m_waveGraph;
  uiTileSelector *  m_oscSelector;
  uiTileSelector *  m_lfoTiles;
  uiGraph *         m_lfoGraph;
  uiKnob *          m_lfoKnobs[3];
  uiGraph *         m_envGraph;
  uiTileSelector *  m_envSelector;
  uiKnob *          m_envKnobs[4];
  bool              m_editFilterEnvelope = false;
  uiGraph *         m_filterGraph;
  uiLedBar *        m_voiceLeds;
  uiStepper *       m_octave;
  uiStepper *       m_presetStepper;
  uiKnob *          m_cutoffKnob;
  uiKnob *          m_resonanceKnob;
  uiKnob *          m_envAmountKnob;
  uiToggleSwitch *  m_filterSwitch;
  Preset            m_presets[PRESETS_COUNT];
  int               m_presetIndex    = 0;
  int               m_presetModified = -1;   // -1 = unknown
  bool              m_batchUpdate    = false;
  bool              m_envGraphDirty  = false;
  bool              m_filterGraphDirty = false;
  int               m_envX[5] = { };   // envelope vertices (plot area coordinates)
  int               m_envY[5] = { };   // envelope levels (0..100)
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// Application


class SynthApp : public uiApp {

  void init()
  {
    synth.begin(&soundGenerator);

    setStyle(&synthStyle);

    auto frame = new SynthFrame(rootWindow());
    frame->loadPresets();
    showWindow(frame, true);
    setActiveWindow(frame);
    setFocusedWindow(frame);

    setTimer(frame, 40);   // voices activity leds
  }

} app;


extern "C" void app_main()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);
  setupPalette();

  // adjust this to center screen in your monitor
  //DisplayController.moveScreen(20, -2);

  while (true)
    app.run(&DisplayController);
}
