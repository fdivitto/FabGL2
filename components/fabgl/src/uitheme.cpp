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


#include "uitheme.h"


namespace fabgl {



// The ESP32 VGA output has 2 bits per channel (64 colors): each component is one of 0, 85, 170, 255
// (RGB888toPackedRGB222() keeps the 2 high bits), so the theme colors are defined on that grid.

// neutral colors (the accent dependent slots are taken from Accents)
static const RGB888 ThemeColors[16] = {
  RGB888(  0,   0,   0),   // Black
  RGB888(  0,   0,   0),   // Desktop     (accent)
  RGB888(  0, 170,  85),   // Green
  RGB888(  0,   0,   0),   // Bloom1      (accent)
  RGB888(  0,   0,   0),   // AccentDark  (accent)
  RGB888(  0,   0,   0),   // Bloom2      (accent)
  RGB888(  0,   0,   0),   // Card
  RGB888( 85,  85,  85),   // Border
  RGB888(170, 170, 170),   // TextDim
  RGB888(255,   0,   0),   // Red
  RGB888( 85,  85,  85),   // Hover
  RGB888(255, 255,   0),   // Amber
  RGB888(  0,   0,   0),   // Accent      (accent)
  RGB888(  0,   0,   0),   // Window
  RGB888(255, 255, 255),   // Text
  RGB888(255, 255, 255),   // White
};


struct AccentShades {
  RGB888 accent, accentDark, bloom2, bloom1, desktop;
};


// indexed by uiThemeAccent
static const AccentShades Accents[] = {
  // accent               accentDark            bloom2               bloom1             desktop
  { RGB888(255, 170,   0), RGB888(170, 85,   0), RGB888(85,  0,  0), RGB888(0, 0, 0), RGB888(0, 0, 0) },  // Orange
  { RGB888(  0, 170, 255), RGB888(  0, 85, 170), RGB888( 0, 85, 85), RGB888(0, 0, 0), RGB888(0, 0, 0) },  // Cyan
  { RGB888(170,  85, 255), RGB888( 85,  0, 170), RGB888(85,  0, 85), RGB888(0, 0, 0), RGB888(0, 0, 0) },  // Violet
  { RGB888( 85, 255,  85), RGB888(  0, 170, 85), RGB888( 0, 85,  0), RGB888(0, 0, 0), RGB888(0, 0, 0) },  // Green
};


uiDarkTheme::uiDarkTheme(uiThemeAccent accent)
  : m_accent(accent),
    m_paletted(false),
    m_paletteSize(0)
{
  applyToUnstyled = true;
}


// 4 color palette: slot of each uiThemeInk (0 = black, 1 = gray, 2 = accent, 3 = white)
static const uint8_t Ink4Slot[16] = {
  0,  // Black
  0,  // Desktop
  2,  // Green
  0,  // Bloom1
  2,  // AccentDark
  1,  // Bloom2
  0,  // Card
  1,  // Border
  3,  // TextDim
  2,  // Red
  1,  // Hover
  2,  // Amber
  2,  // Accent
  0,  // Window
  3,  // Text
  3,  // White
};


// default palette of VGA4Controller: the display maps a color to the slot that had the nearest default color
static const RGB888 Default4Palette[4] = { RGB888(0, 0, 0), RGB888(0, 0, 255), RGB888(0, 255, 0), RGB888(255, 255, 255) };


// prepares the colors returned by ink() for a palette of "colors" entries, returns the number of slots to program
int uiDarkTheme::preparePalette(int colors)
{
  m_paletteSize = colors >= 16 ? 16 : 4;
  for (int i = 0; i < 16; ++i)
    m_ink[i] = m_paletteSize == 16 ? RGB888((Color) i) : Default4Palette[Ink4Slot[i]];
  m_paletted = true;
  return m_paletteSize;
}


RGB888 uiDarkTheme::paletteColor(int slot)
{
  if (m_paletteSize == 16)
    return color((uiThemeInk) slot);
  switch (slot) {
    case 1:  return color(uiThemeInk::Border);
    case 2:  return color(uiThemeInk::Accent);
    case 3:  return color(uiThemeInk::White);
    default: return color(uiThemeInk::Black);
  }
}


RGB888 uiDarkTheme::color(uiThemeInk value)
{
  auto const & a = Accents[(int) m_accent];
  switch (value) {
    case uiThemeInk::Accent:     return a.accent;
    case uiThemeInk::AccentDark: return a.accentDark;
    case uiThemeInk::Bloom2:     return a.bloom2;
    case uiThemeInk::Bloom1:     return a.bloom1;
    case uiThemeInk::Desktop:    return a.desktop;
    default:                     return ThemeColors[(int) value];
  }
}


void uiDarkTheme::apply(uiApp * app)
{
  app->setStyle(this);

  auto & dlg = app->appProps().dialogStyleClasses;
  dlg.frame    = uiThemeStyle::Dialog;
  dlg.label    = uiThemeStyle::Label;
  dlg.button   = uiThemeStyle::Button;
  dlg.textEdit = uiThemeStyle::Edit;
  dlg.panel    = uiThemeStyle::DialogFooter;

  // the root window exists before the style is set
  app->rootWindow()->frameStyle().backgroundColor = ink(uiThemeInk::Desktop);
}


uint32_t uiDarkTheme::defaultStyleClass(uiObject * object)
{
  auto const & t = object->objectType();
  if (t.uiFrame) {
    auto title = ((uiFrame *) object)->title();
    return title && title[0] ? uiThemeStyle::Frame : uiThemeStyle::Popup;
  }
  if (t.uiPanel)
    return uiThemeStyle::Card;
  if (t.uiLabel || t.uiStaticLabel)
    return uiThemeStyle::Label;
  if (t.uiSplitButton)
    return uiThemeStyle::SplitButton;
  if (t.uiCustomComboBox)
    return uiThemeStyle::ComboBox;
  if (t.uiButton)
    return uiThemeStyle::Button;
  if (t.uiTextEdit)
    return uiThemeStyle::Edit;
  if (t.uiCustomListBox)
    return uiThemeStyle::ListBox;
  if (t.uiCheckBox)
    return uiThemeStyle::CheckBox;
  if (t.uiSlider)
    return uiThemeStyle::Slider;
  if (t.uiProgressBar)
    return uiThemeStyle::ProgressBar;
  if (t.uiPaintBox)
    return uiThemeStyle::PaintBox;
  if (t.uiKnob)
    return uiThemeStyle::Knob;
  if (t.uiToggleSwitch)
    return uiThemeStyle::ToggleSwitch;
  if (t.uiLevelBar)
    return uiThemeStyle::LevelBar;
  if (t.uiLedBar)
    return uiThemeStyle::LedBar;
  if (t.uiStepper)
    return uiThemeStyle::Stepper;
  if (t.uiTileSelector)
    return uiThemeStyle::TileSelector;
  if (t.uiGraph)
    return uiThemeStyle::Graph;
  if (t.uiPianoKeyboard)
    return uiThemeStyle::PianoKeyboard;
  return 0;
}


void uiDarkTheme::setBorder(uiWindow * window, int size, RGB888 color, RGB888 focusedColor, int cornerRadius)
{
  auto & s = window->windowStyle();
  s.borderSize         = size;
  s.focusedBorderSize  = size;
  s.borderColor        = color;
  s.activeBorderColor  = color;
  s.focusedBorderColor = focusedColor;
  s.cornerRadius       = cornerRadius;
}


void uiDarkTheme::setScrollBars(uiScrollableControl * control)
{
  auto & s = control->scrollableControlStyle();
  s.scrollBarBackgroundColor          = ink(uiThemeInk::Card);
  s.scrollBarForegroundColor          = ink(uiThemeInk::Border);
  s.mouseOverScrollBarForegroundColor = ink(uiThemeInk::TextDim);
  s.scrollBarSize                     = 10;
}


// controls are "outlined": background of the container, thin gray border, accent border when focused
void uiDarkTheme::setListBox(uiCustomListBox * listBox)
{
  RGB888 background = listBox->parentBackgroundColor();
  setBorder(listBox, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
  setScrollBars(listBox);
  auto & s = listBox->listBoxStyle();
  s.backgroundColor                = background;
  s.focusedBackgroundColor         = background;
  s.selectedBackgroundColor        = ink(uiThemeInk::Hover);
  s.focusedSelectedBackgroundColor = ink(uiThemeInk::Accent);
  s.itemHeight                     = 20;
  s.textFont                       = normalFont();
  s.textColor                      = ink(uiThemeInk::Text);
  s.selectedTextColor              = ink(uiThemeInk::White);
}


void uiDarkTheme::setEdit(uiTextEdit * edit)
{
  RGB888 background = edit->parentBackgroundColor();
  setBorder(edit, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
  auto & s = edit->textEditStyle();
  s.backgroundColor          = background;
  s.mouseOverBackgroundColor = background;
  s.focusedBackgroundColor   = background;
  s.textColor                = ink(uiThemeInk::Text);
  s.textFont                 = normalFont();
}


void uiDarkTheme::setButton(uiButton * button, uint32_t styleClassID)
{
  auto & s = button->buttonStyle();
  s.textFont = normalFont();
  switch (styleClassID) {
    case uiThemeStyle::ButtonAccent:
      setBorder(button, 1, ink(uiThemeInk::Accent), ink(uiThemeInk::White));
      s.backgroundColor          = ink(uiThemeInk::Accent);
      s.mouseOverBackgroundColor = ink(uiThemeInk::AccentDark);
      s.mouseDownBackgroundColor = ink(uiThemeInk::AccentDark);
      s.downBackgroundColor      = ink(uiThemeInk::AccentDark);
      s.textColor                = ink(uiThemeInk::White);
      s.mouseOverTextColor       = ink(uiThemeInk::White);
      s.downTextColor            = ink(uiThemeInk::White);
      break;
    case uiThemeStyle::ButtonFlat:
    {
      RGB888 background = button->parentBackgroundColor();
      setBorder(button, 1, background, ink(uiThemeInk::Accent));
      s.backgroundColor          = background;
      s.mouseOverBackgroundColor = ink(uiThemeInk::Hover);
      s.mouseDownBackgroundColor = ink(uiThemeInk::AccentDark);
      s.downBackgroundColor      = ink(uiThemeInk::AccentDark);
      s.textColor                = ink(uiThemeInk::Text);
      s.mouseOverTextColor       = ink(uiThemeInk::White);
      s.downTextColor            = ink(uiThemeInk::White);
      break;
    }
    default:
      // outlined. When down (switch buttons) it takes the accent color
      setBorder(button, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
      s.backgroundColor          = button->parentBackgroundColor();
      s.mouseOverBackgroundColor = ink(uiThemeInk::Hover);
      s.mouseDownBackgroundColor = ink(uiThemeInk::AccentDark);
      s.downBackgroundColor      = ink(uiThemeInk::Accent);
      s.textColor                = ink(uiThemeInk::Text);
      s.mouseOverTextColor       = ink(uiThemeInk::White);
      s.downTextColor            = ink(uiThemeInk::White);
      break;
  }
}


// windows with title: 3 pixels border (accent when active, it is also the resize area), accent title bar with a line
// below, size grip. Popups: 1 pixel border. keepBorderSize: frames created without style class keep their border
void uiDarkTheme::setFrame(uiFrame * frame, uint32_t styleClassID, bool keepBorderSize)
{
  bool hasTitle = styleClassID == uiThemeStyle::Frame || styleClassID == uiThemeStyle::Dialog;
  int  bSize    = keepBorderSize ? frame->windowStyle().borderSize : (hasTitle ? 3 : 1);
  setBorder(frame, bSize, ink(uiThemeInk::Border), ink(uiThemeInk::Border));
  if (hasTitle) {
    frame->windowStyle().borderColor       = ink(uiThemeInk::Hover);
    frame->windowStyle().activeBorderColor = ink(uiThemeInk::Accent);
  }
  frame->frameProps().hasSizeGrip = hasTitle;
  auto & s = frame->frameStyle();
  s.backgroundColor                = styleClassID == uiThemeStyle::Frame ? ink(uiThemeInk::Window) : ink(uiThemeInk::Card);
  s.titleBackgroundColor           = ink(uiThemeInk::Card);
  s.activeTitleBackgroundColor     = ink(uiThemeInk::AccentDark);
  s.titleSeparatorSize             = 1;
  s.titleSeparatorColor            = ink(uiThemeInk::Border);
  s.activeTitleSeparatorColor      = ink(uiThemeInk::Accent);
  s.sizeGripColor                  = ink(uiThemeInk::TextDim);
  s.titleColor                     = ink(uiThemeInk::TextDim);
  s.activeTitleColor               = ink(uiThemeInk::White);
  s.titleFont                      = normalFont();
  s.buttonColor                    = ink(uiThemeInk::TextDim);
  s.activeButtonColor              = ink(uiThemeInk::White);
  s.mouseOverBackgroundButtonColor = ink(uiThemeInk::Accent);
  s.mouseOverButtonColor           = ink(uiThemeInk::White);
}


void uiDarkTheme::setLabel(uiObject * object, uint32_t styleClassID)
{
  FontInfo const * font = styleClassID == uiThemeStyle::LabelDim ? smallFont() :
                          styleClassID == uiThemeStyle::LabelTitle ? titleFont() :
                          styleClassID == uiThemeStyle::LabelBig ? bigFont() : normalFont();
  bool accented = styleClassID == uiThemeStyle::LabelValue || styleClassID == uiThemeStyle::LabelTitle || styleClassID == uiThemeStyle::LabelBig;
  RGB888 color  = styleClassID == uiThemeStyle::LabelDim ? ink(uiThemeInk::TextDim) :
                  styleClassID == uiThemeStyle::LabelDesktop ? ink(uiThemeInk::White) :
                  accented ? ink(uiThemeInk::Accent) : ink(uiThemeInk::Text);
  RGB888 background = styleClassID == uiThemeStyle::LabelDesktop ? ink(uiThemeInk::Desktop) : ((uiWindow *) object)->parentBackgroundColor();
  if (object->objectType().uiStaticLabel) {
    auto & s = ((uiStaticLabel *) object)->labelStyle();
    s.textFont        = font;
    s.textColor       = color;
    s.backgroundColor = background;
  } else if (object->objectType().uiLabel) {
    auto & s = ((uiLabel *) object)->labelStyle();
    s.textFont        = font;
    s.textColor       = color;
    s.backgroundColor = background;
    s.textAlign       = styleClassID == uiThemeStyle::LabelValue ? uiHAlign::Right : uiHAlign::Left;
  }
}


void uiDarkTheme::setComboBox(uiCustomComboBox * comboBox)
{
  auto & s = comboBox->comboBoxStyle();
  s.buttonBackgroundColor = comboBox->parentBackgroundColor();
  s.buttonColor           = ink(uiThemeInk::TextDim);
  auto listBox = comboBox->listBoxControl();
  setListBox(listBox);
  // the drop down list is over other windows: raised surface
  listBox->listBoxStyle().backgroundColor        = ink(uiThemeInk::Card);
  listBox->listBoxStyle().focusedBackgroundColor = ink(uiThemeInk::Card);
  auto edit = comboBox->editControl();
  if (edit->objectType().uiTextEdit)
    setEdit((uiTextEdit *) edit);
  else if (edit->objectType().uiButton)
    setButton((uiButton *) edit, uiThemeStyle::Button);
  setBorder(comboBox, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Border));
}


void uiDarkTheme::setStyle(uiObject * object, uint32_t styleClassID)
{
  bool byType = styleClassID == 0;
  if (byType)
    styleClassID = defaultStyleClass(object);

  auto const & type = object->objectType();

  switch (styleClassID) {

    case uiThemeStyle::Frame:
    case uiThemeStyle::Dialog:
    case uiThemeStyle::Popup:
      if (type.uiFrame)
        setFrame((uiFrame *) object, styleClassID, byType);
      break;

    case uiThemeStyle::Taskbar:
      if (type.uiFrame) {
        auto frame = (uiFrame *) object;
        setFrame(frame, uiThemeStyle::Popup, false);
        frame->frameStyle().backgroundColor = ink(uiThemeInk::Black);
        frame->windowStyle().borderSize     = 0;
        frame->frameProps().moveable        = false;
        frame->frameProps().resizeable      = false;
      }
      break;

    case uiThemeStyle::DialogFooter:
    case uiThemeStyle::Card:
      if (type.uiPanel) {
        auto panel = (uiPanel *) object;
        setBorder(panel, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Border));
        panel->panelStyle().backgroundColor = styleClassID == uiThemeStyle::Card ? ink(uiThemeInk::Card) : ink(uiThemeInk::Window);
      }
      break;

    case uiThemeStyle::Label:
    case uiThemeStyle::LabelDim:
    case uiThemeStyle::LabelTitle:
    case uiThemeStyle::LabelBig:
    case uiThemeStyle::LabelDesktop:
    case uiThemeStyle::LabelValue:
      setLabel(object, styleClassID);
      break;

    case uiThemeStyle::Button:
    case uiThemeStyle::ButtonAccent:
    case uiThemeStyle::ButtonFlat:
      if (type.uiButton)
        setButton((uiButton *) object, styleClassID);
      break;

    case uiThemeStyle::Edit:
      if (type.uiTextEdit)
        setEdit((uiTextEdit *) object);
      break;

    case uiThemeStyle::ListBox:
      if (type.uiCustomListBox)
        setListBox((uiCustomListBox *) object);
      break;

    case uiThemeStyle::ComboBox:
    case uiThemeStyle::SplitButton:
      // the last constructor (uiComboBox, uiSplitButton...) has created the edit control and the list
      if (type.uiComboBox || type.uiSplitButton || type.uiColorComboBox)
        setComboBox((uiCustomComboBox *) object);
      break;

    case uiThemeStyle::CheckBox:
      if (type.uiCheckBox) {
        // outlined box with an accent mark, round radio buttons (corner radius = half size) with an accent dot
        auto checkBox = (uiCheckBox *) object;
        bool radio = checkBox->kind() == uiCheckBoxKind::RadioButton;
        int  size  = imin(checkBox->size().width, checkBox->size().height);
        RGB888 background = checkBox->parentBackgroundColor();
        setBorder(checkBox, 1, ink(uiThemeInk::TextDim), ink(uiThemeInk::Accent), radio ? size / 2 : 0);
        auto & s = checkBox->checkBoxStyle();
        s.boldMark                 = true;
        s.backgroundColor          = background;
        s.checkedBackgroundColor   = background;
        s.mouseOverBackgroundColor = ink(uiThemeInk::Hover);
        s.foregroundColor          = ink(uiThemeInk::Accent);
        s.mouseOverForegroundColor = ink(uiThemeInk::Accent);
      }
      break;

    case uiThemeStyle::Slider:
      if (type.uiSlider) {
        auto slider = (uiSlider *) object;
        setBorder(slider, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        slider->windowStyle().focusedBorderSize = 1;
        auto & s = slider->sliderStyle();
        s.backgroundColor    = slider->parentBackgroundColor();
        s.slideColor         = ink(uiThemeInk::Border);
        s.rangeColor         = ink(uiThemeInk::Accent);
        s.gripColor          = ink(uiThemeInk::TextDim);
        s.mouseOverGripColor = ink(uiThemeInk::Text);
        s.ticksColor         = ink(uiThemeInk::Border);
      }
      break;

    case uiThemeStyle::ProgressBar:
      if (type.uiProgressBar) {
        auto bar = (uiProgressBar *) object;
        setBorder(bar, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Border));
        auto & s = bar->progressBarStyle();
        s.backgroundColor = ink(uiThemeInk::Border);
        s.foregroundColor = ink(uiThemeInk::Accent);
        s.textFont        = smallFont();
        s.textColor       = ink(uiThemeInk::White);
      }
      break;

    case uiThemeStyle::PaintBox:
      if (type.uiPaintBox) {
        auto paintBox = (uiPaintBox *) object;
        setBorder(paintBox, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Border));
        setScrollBars(paintBox);
        paintBox->paintBoxStyle().backgroundColor = ink(uiThemeInk::Black);
      }
      break;

    case uiThemeStyle::Knob:
      if (type.uiKnob) {
        auto knob = (uiKnob *) object;
        setBorder(knob, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        knob->windowStyle().focusedBorderSize = 1;
        auto & s = knob->knobStyle();
        s.backgroundColor     = knob->parentBackgroundColor();
        s.labelFont           = smallFont();
        s.labelColor          = ink(uiThemeInk::TextDim);
        s.valueFont           = smallFont();
        s.valueColor          = ink(uiThemeInk::Text);
        s.activeValueColor    = ink(uiThemeInk::Accent);
        s.ringColor           = ink(uiThemeInk::Accent);
        s.ringBackgroundColor = ink(uiThemeInk::Hover);
        s.bodyColor           = ink(uiThemeInk::Border);
        s.capColor            = ink(uiThemeInk::Window);
        s.pointerColor        = ink(uiThemeInk::Accent);
      }
      break;

    case uiThemeStyle::ToggleSwitch:
      if (type.uiToggleSwitch) {
        auto sw = (uiToggleSwitch *) object;
        setBorder(sw, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        sw->windowStyle().focusedBorderSize = 1;
        auto & s = sw->toggleSwitchStyle();
        s.backgroundColor = sw->parentBackgroundColor();
        s.onColor         = ink(uiThemeInk::Accent);
        s.offColor        = ink(uiThemeInk::Border);
        s.thumbColor      = ink(uiThemeInk::Text);
        s.textFont        = normalFont();
        s.onTextColor     = ink(uiThemeInk::Text);
        s.offTextColor    = ink(uiThemeInk::TextDim);
      }
      break;

    case uiThemeStyle::LevelBar:
      if (type.uiLevelBar) {
        auto bar = (uiLevelBar *) object;
        setBorder(bar, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        bar->windowStyle().focusedBorderSize = 1;
        bar->levelBarProps().segmentGap = 1;
        auto & s = bar->levelBarStyle();
        s.backgroundColor = bar->parentBackgroundColor();
        s.lowColor        = ink(uiThemeInk::Accent);
        s.lowOffColor     = ink(uiThemeInk::Hover);
        s.highColor       = ink(uiThemeInk::Red);
        s.highOffColor    = ink(uiThemeInk::Hover);
        s.textFont        = normalFont();
        s.textColor       = ink(uiThemeInk::Text);
      }
      break;

    case uiThemeStyle::LedBar:
      if (type.uiLedBar) {
        auto & s = ((uiLedBar *) object)->ledBarStyle();
        s.backgroundColor = ((uiWindow *) object)->parentBackgroundColor();
        s.ledColors[0]    = ink(uiThemeInk::Hover);
        s.ledColors[1]    = ink(uiThemeInk::Green);
        s.ledColors[2]    = ink(uiThemeInk::Amber);
        s.ledColors[3]    = ink(uiThemeInk::Red);
      }
      break;

    case uiThemeStyle::Stepper:
      if (type.uiStepper) {
        auto stepper = (uiStepper *) object;
        RGB888 background = stepper->parentBackgroundColor();
        setBorder(stepper, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        auto & s = stepper->stepperStyle();
        s.backgroundColor                = background;
        s.buttonBackgroundColor          = background;
        s.mouseDownButtonBackgroundColor = ink(uiThemeInk::Accent);
        s.buttonBorderColor              = ink(uiThemeInk::Border);
        s.arrowColor                     = ink(uiThemeInk::TextDim);
        s.mouseDownArrowColor            = ink(uiThemeInk::White);
        s.textFont                       = normalFont();
        s.textColor                      = ink(uiThemeInk::Text);
      }
      break;

    case uiThemeStyle::TileSelector:
      if (type.uiTileSelector) {
        auto tiles = (uiTileSelector *) object;
        setBorder(tiles, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        tiles->windowStyle().focusedBorderSize = 1;
        auto & s = tiles->tileSelectorStyle();
        s.backgroundColor         = tiles->parentBackgroundColor();
        s.tileBackgroundColor     = ink(uiThemeInk::Window);
        s.tileBorderColor         = ink(uiThemeInk::Border);
        s.selectedTileBorderColor = ink(uiThemeInk::Accent);
        s.textFont                = smallFont();
        s.textColor               = ink(uiThemeInk::TextDim);
        s.selectedTextColor       = ink(uiThemeInk::Accent);
      }
      break;

    case uiThemeStyle::Graph:
      if (type.uiGraph) {
        auto graph = (uiGraph *) object;
        setBorder(graph, 1, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        auto & s = graph->graphStyle();
        s.backgroundColor = ink(uiThemeInk::Black);
        s.gridColor       = ink(uiThemeInk::Hover);
        s.lineColor       = ink(uiThemeInk::Accent);
        s.fillColor       = ink(uiThemeInk::Bloom2);
      }
      break;

    case uiThemeStyle::PianoKeyboard:
      if (type.uiPianoKeyboard) {
        auto piano = (uiPianoKeyboard *) object;
        setBorder(piano, 0, ink(uiThemeInk::Border), ink(uiThemeInk::Accent));
        piano->windowStyle().focusedBorderSize = 1;
        auto & s = piano->pianoKeyboardStyle();
        s.backgroundColor      = ink(uiThemeInk::Black);
        s.topLineColor         = ink(uiThemeInk::Accent);
        s.whiteKeyColor        = ink(uiThemeInk::Text);
        s.whiteKeyFrontColor   = ink(uiThemeInk::TextDim);
        s.blackKeyColor        = ink(uiThemeInk::Black);
        s.blackKeyBevelColor   = ink(uiThemeInk::Hover);
        s.pressedKeyColor      = ink(uiThemeInk::Accent);
        s.pressedKeyFrontColor = ink(uiThemeInk::AccentDark);
        s.textFont             = smallFont();
        s.labelColor           = ink(uiThemeInk::TextDim);
        s.blackLabelColor      = ink(uiThemeInk::TextDim);
        s.noteNameColor        = ink(uiThemeInk::TextDim);
        s.cNoteNameColor       = ink(uiThemeInk::AccentDark);
        s.pressedTextColor     = ink(uiThemeInk::White);
      }
      break;

    default:
      break;
  }
}



} // end of namespace
