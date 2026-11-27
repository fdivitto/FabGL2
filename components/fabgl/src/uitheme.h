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
 * @brief This file contains uiDarkTheme, a ready to use dark theme for the GUI (fabui.h)
 */


#include "fabui.h"


namespace fabgl {



/** \ingroup Enumerations
 * @brief Colors of uiDarkTheme. Each one is a slot of the 16 color palette (see uiDarkTheme::setupPalette())
 */
enum class uiThemeInk : uint8_t {
  Black,        /**< Black (slot 0: the mouse pointer uses it) */
  Desktop,      /**< Desktop background, tinted with the accent */
  Green,        /**< Green (leds) */
  Bloom1,       /**< Very dark shade of the accent (wallpapers) */
  AccentDark,   /**< Dark shade of the accent: active title bar, pressed buttons */
  Bloom2,       /**< Dark shade of the accent: graph area fill, wallpapers */
  Card,         /**< Raised surfaces: cards, popups, dialogs, drop down lists (black like Window: the border marks them) */
  Border,       /**< Thin lines and borders */
  TextDim,      /**< Secondary text */
  Red,          /**< Red (slot 9: error icon of the message boxes) */
  Hover,        /**< Background under the mouse, off segments */
  Amber,        /**< Amber (slot 11: warning icon of the message boxes) */
  Accent,       /**< The accent color (slot 12: info and question icons of the message boxes) */
  Window,       /**< Window background */
  Text,         /**< Text */
  White,        /**< White (slot 15: the mouse pointer uses it) */
};


/** \ingroup Enumerations
 * @brief Accent colors of uiDarkTheme
 */
enum class uiThemeAccent : uint8_t {
  Orange,       /**< Orange */
  Cyan,         /**< Cyan */
  Violet,       /**< Violet */
  Green,        /**< Green */
};


/**
 * @brief Style classes of uiDarkTheme
 *
 * Pass them as styleClassID (last parameter of the constructors) to select a variant. Controls created without
 * style class get the theme by type (Label, Button, Edit...). Classes of a derived theme must be lower than First.
 */
struct uiThemeStyle {
  enum : uint32_t {
    First = 1000,
    Frame = First,    /**< Application window (uiFrame) */
    Dialog,           /**< Dialog and message box (uiFrame), raised surface */
    DialogFooter,     /**< Panel with the buttons of a dialog (uiPanel) */
    Popup,            /**< Frame without title: menus, popups (uiFrame) */
    Taskbar,          /**< Bar without border at the screen side (uiFrame) */
    Card,             /**< Raised panel with a thin border, groups controls (uiPanel) */
    Label,            /**< Text, the background is the one of the container (uiLabel, uiStaticLabel) */
    LabelDim,         /**< Secondary text, small font */
    LabelTitle,       /**< Section title, accent color */
    LabelBig,         /**< Big title, accent color */
    LabelDesktop,     /**< White text on the desktop */
    LabelValue,       /**< Value, accent color, right aligned (uiLabel) */
    Button,           /**< Outlined button (uiButton) */
    ButtonAccent,     /**< Primary button, filled with the accent */
    ButtonFlat,       /**< Borderless button (taskbars, toolbars) */
    Edit,             /**< uiTextEdit */
    ListBox,          /**< uiListBox, uiFileBrowser, uiColorListBox... */
    ComboBox,         /**< uiComboBox, uiColorComboBox */
    SplitButton,      /**< uiSplitButton */
    CheckBox,         /**< uiCheckBox (check box and round radio button) */
    Slider,           /**< uiSlider */
    ProgressBar,      /**< uiProgressBar */
    PaintBox,         /**< uiPaintBox, black surface */
    Knob,             /**< uiKnob */
    ToggleSwitch,     /**< uiToggleSwitch */
    LevelBar,         /**< uiLevelBar */
    LedBar,           /**< uiLedBar */
    Stepper,          /**< uiStepper */
    TileSelector,     /**< uiTileSelector */
    Graph,            /**< uiGraph, black surface */
    PianoKeyboard,    /**< uiPianoKeyboard */
    Last,
  };
};


/**
 * @brief A ready to use dark flat theme: near black surfaces, thin gray lines and a colored accent
 *
 * It is made for 16 color displays (VGA16Controller, CVBS16Controller): setupPalette() reprograms the palette with the
 * theme colors (slots chosen so that the mouse pointer and the message box icons keep their colors). The colors are
 * on the grid of the VGA output (2 bits per channel: 0, 85, 170, 255). Without setupPalette() (for example to keep the
 * standard 16 colors) or on other displays the theme colors are approximated by the display.
 *
 * Usage:
 *
 *     static fabgl::uiDarkTheme theme;     // optionally: theme(fabgl::uiThemeAccent::Violet)
 *
 *     class MyApp : public uiApp {
 *       void init() {
 *         theme.apply(this);
 *         // controls created without style class get the theme by type
 *         new uiButton(rootWindow(), "OK", Point(10, 10), Size(80, 24));
 *         // style classes select the variants
 *         new uiStaticLabel(rootWindow(), "Title", Point(10, 40), true, fabgl::uiThemeStyle::LabelTitle);
 *       }
 *     } app;
 *
 *     DisplayController.setResolution(VGA_640x480_60Hz);
 *     theme.setupPalette(&DisplayController);   // before the application creates its windows
 *     app.run(&DisplayController);
 *
 * Use ink() to paint with the theme colors (onPaint handlers, uiPaintBox). Derive from it to add style classes
 * (lower than uiThemeStyle::First) and call uiDarkTheme::setStyle() for the others.
 *
 * @ingroup gui
 */
class uiDarkTheme : public uiStyle {

public:

  /**
   * @brief Creates the theme
   *
   * @param accent The accent color
   */
  uiDarkTheme(uiThemeAccent accent = uiThemeAccent::Orange);

  /**
   * @brief Determines the accent color
   *
   * @return The accent specified at construction
   */
  uiThemeAccent accent()                     { return m_accent; }

  /**
   * @brief Gets the actual RGB value of a theme color
   *
   * @param value Theme color
   *
   * @return The RGB color
   */
  RGB888 color(uiThemeInk value);

  /**
   * @brief Gets the color to use with the Canvas and the styles to obtain a theme color
   *
   * After setupPalette() this is the default color of the palette slot (the display maps it to the slot), otherwise
   * it is the actual color.
   *
   * @param value Theme color
   *
   * @return The color to pass to Canvas::setPenColor(), Canvas::setBrushColor() or to a style field
   */
  RGB888 ink(uiThemeInk value)               { return m_paletted ? m_ink[(int) value] : color(value); }

  /**
   * @brief Reprograms the palette of the display with the theme colors
   *
   * Call it after setResolution() and before the application creates its windows (styles store the colors).
   * With 16 colors every theme color has its slot. With 4 colors (VGA4Controller) the theme uses black, gray, the
   * accent and white: dim text is white, the accent shades and the status colors (red, amber, green) are the accent.
   *
   * @param displayController A 16 color (VGA16Controller, CVBS16Controller) or 4 color (VGA4Controller) paletted display
   */
  template <typename DisplayController>
  void setupPalette(DisplayController * displayController) {
    int colors = preparePalette(displayController->colorsCount());
    for (int i = 0; i < colors; ++i)
      displayController->setPaletteItem(i, paletteColor(i));
  }

  /**
   * @brief Applies the theme to an application
   *
   * Sets it as the application style (uiApp::setStyle()), themes the message boxes (uiAppProps::dialogStyleClasses)
   * and the desktop (root window background). Call it from uiApp::init(), before creating the windows.
   *
   * @param app The application
   */
  void apply(uiApp * app);

  /**
   * @brief Configures a UI element (called by the library)
   *
   * @param object The UI element
   * @param styleClassID One of uiThemeStyle, or 0 to style it by type
   */
  void setStyle(uiObject * object, uint32_t styleClassID);

  /**
   * @brief Gets the style class used for an element created without style class
   *
   * @param object The UI element
   *
   * @return A uiThemeStyle value, 0 if the type has no style
   */
  uint32_t defaultStyleClass(uiObject * object);

  static FontInfo const * smallFont()        { return &FONT_std_12; }   /**< Font of secondary texts */
  static FontInfo const * normalFont()       { return &FONT_std_14; }   /**< Font of texts and controls */
  static FontInfo const * titleFont()        { return &FONT_std_17; }   /**< Font of section titles */
  static FontInfo const * bigFont()          { return &FONT_std_24; }   /**< Font of big titles */


protected:

  void setBorder(uiWindow * window, int size, RGB888 color, RGB888 focusedColor, int cornerRadius = 0);
  void setScrollBars(uiScrollableControl * control);
  void setListBox(uiCustomListBox * listBox);
  void setEdit(uiTextEdit * edit);
  void setButton(uiButton * button, uint32_t styleClassID);
  void setFrame(uiFrame * frame, uint32_t styleClassID, bool keepBorderSize);
  void setLabel(uiObject * object, uint32_t styleClassID);
  void setComboBox(uiCustomComboBox * comboBox);

  int preparePalette(int colors);
  RGB888 paletteColor(int slot);


private:

  uiThemeAccent m_accent;
  bool          m_paletted;
  int           m_paletteSize;
  RGB888        m_ink[16];      // color to use for each uiThemeInk with the reprogrammed palette

};



} // end of namespace
