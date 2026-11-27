#pragma once

/*
  The look comes from the dark theme of the library (fabgl::uiDarkTheme, uitheme.h): it reprograms the 16 color
  palette (theme.setupPalette() in app_main) and styles every control. Style classes (fabgl::uiThemeStyle) select
  the variants: accent buttons, titles, cards... The accent can be Orange, Cyan, Violet or Green.
*/


static fabgl::uiDarkTheme theme(fabgl::uiThemeAccent::Orange);   // <- accent color


using Ink          = fabgl::uiThemeInk;
using uiThemeStyle = fabgl::uiThemeStyle;


// color to paint with (onPaint handlers)
static RGB888 ink(Ink value)
{
  return theme.ink(value);
}


static fabgl::FontInfo const * const SmallFont  = fabgl::uiDarkTheme::smallFont();
static fabgl::FontInfo const * const NormalFont = fabgl::uiDarkTheme::normalFont();
static fabgl::FontInfo const * const BigFont    = fabgl::uiDarkTheme::bigFont();
