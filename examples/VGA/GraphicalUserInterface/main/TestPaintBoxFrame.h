#pragma once

/* Actually this tests:

  - uiPaintBox widget
  - horizontal scroll bar
  - anchors

*/

#include "esp_random.h"



struct TestPaintBoxFrame : public uiFrame {

  uiPaintBox * paintBox;

  static constexpr int count = 1000;
  int8_t * values = nullptr;

  TestPaintBoxFrame(uiFrame * parent)
    : uiFrame(parent, "Paint Box", Point(150, 40), Size(340, 220), false, uiThemeStyle::Frame) {

    values = new int8_t[count];
    for (int i = 0; i < count; ++i)
      values[i] = randomInt(-50, 50);

    // the bottom strip shows the size grip of the frame
    paintBox = new uiPaintBox(this, clientPos(), Size(clientSize().width, clientSize().height - 12), true, uiThemeStyle::PaintBox);
    paintBox->anchors().right = true;
    paintBox->anchors().bottom = true;
    paintBox->setScrollBar(uiOrientation::Horizontal, 0, paintBox->clientSize().width, count);
    paintBox->onPaint = [&](Rect const & r) { onPaintPaintBox(r); };
    paintBox->onChangeHScrollBar = [&]() { paintBox->repaint(); };
  }

  void onPaintPaintBox(Rect const & r) {
    int w = r.width(), h = r.height();
    int midY = h / 2;

    auto cv = canvas();

    // paint occurs even on resize, so we need to make sure it has the right "width" (w)
    paintBox->setScrollBar(uiOrientation::Horizontal, paintBox->HScrollBarPos(), w, count, true);

    cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false).Italic(false).Underline(false).Invert(0));
    cv->selectFont(SmallFont);
    for (int i = paintBox->HScrollBarPos(), x = 1; i < paintBox->HScrollBarPos() + paintBox->HScrollBarVisible(); ++i, ++x) {
      cv->setPenColor(values[i] < 0 ? ink(Ink::AccentDark) : ink(Ink::Accent));
      cv->drawLine(x, midY, x, midY + values[i]);
      if (i % 50 == 0) {
        cv->setPenColor(ink(Ink::Border));
        cv->drawLine(x, midY - 15, x, midY + 15);
        cv->setPenColor(ink(Ink::TextDim));
        cv->drawTextFmt(x + 2, h - 18, "%d", i);
      }
    }
    cv->setPenColor(ink(Ink::Border));
    cv->drawLine(0, midY, w - 1, midY);
  }

};
