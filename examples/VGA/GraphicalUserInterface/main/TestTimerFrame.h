#pragma once

#include <time.h>


// analog clock updated by a timer (time since boot: the board has no RTC)
struct TestTimerFrame : public uiFrame {

  uiPaintBox * paintBox;
  uiTimerHandle timer;

  TestTimerFrame(uiFrame * parent)
    : uiFrame(parent, "Clock", Point(470, 20), Size(160, 170), false, uiThemeStyle::Frame) {

    timer = nullptr;

    // the bottom strip shows the size grip of the frame
    paintBox = new uiPaintBox(this, clientPos(), Size(clientSize().width, clientSize().height - 12), true, uiThemeStyle::PaintBox);
    paintBox->paintBoxStyle().backgroundColor = ink(Ink::Window);
    paintBox->anchors().right = true;
    paintBox->anchors().bottom = true;
    paintBox->onPaint = [&](Rect const & r) { onPaintPaintBox(r); };

    this->onShow = [&]() {
      timer = app()->setTimer(this, 1000);
    };
    this->onHide = [&]() {
      if (timer)
        app()->killTimer(timer);
      timer = nullptr;
    };
    this->onTimer = [&](uiTimerHandle tHandle) { paintBox->repaint(); };
  }

  void onPaintPaintBox(Rect const & r) {
    int width = r.width(), height = r.height();

    time_t now = time(0);
    tm * localtm = localtime(&now);

    double radius = std::min(width, height) / 2.0 - 6;
    double pointsRadius  = radius * 0.86;
    double secondsRadius = radius * 0.80;
    double minutesRadius = radius * 0.72;
    double hoursRadius   = radius * 0.50;

    double s = localtm->tm_sec / 60.0 * (2.0 * M_PI) - (M_PI / 2.0);
    double m = (localtm->tm_min + localtm->tm_sec / 60.0) / 60.0 * (2.0 * M_PI) - (M_PI / 2.0);
    double h = (localtm->tm_hour + localtm->tm_min / 60.0) / 24.0 * (2.0 * M_PI) * 2.0 - (M_PI / 2.0);

    double cx = width / 2.0;
    double cy = height / 2.0;

    auto cv = canvas();

    // face
    cv->setBrushColor(ink(Ink::Card));
    cv->fillEllipse(cx, cy, radius * 2, radius * 2);
    cv->setPenColor(ink(Ink::Border));
    cv->drawEllipse(cx, cy, radius * 2, radius * 2);

    // ticks
    for (int a = 0; a < 360; a += 6) {
      double arad = a * M_PI / 180.0;
      cv->setPenColor(a % 30 == 0 ? ink(Ink::TextDim) : ink(Ink::Border));
      double r1 = a % 30 == 0 ? pointsRadius - 4 : pointsRadius;
      cv->drawLine(cx + cos(arad) * r1, cy + sin(arad) * r1, cx + cos(arad) * (radius - 3), cy + sin(arad) * (radius - 3));
    }

    // hands
    cv->setPenColor(ink(Ink::Text));
    cv->drawLine(cx, cy, cx + cos(h) * hoursRadius,   cy + sin(h) * hoursRadius);
    cv->drawLine(cx + 1, cy, cx + 1 + cos(h) * hoursRadius, cy + sin(h) * hoursRadius);
    cv->drawLine(cx, cy, cx + cos(m) * minutesRadius, cy + sin(m) * minutesRadius);
    cv->setPenColor(ink(Ink::Accent));
    cv->drawLine(cx, cy, cx + cos(s) * secondsRadius, cy + sin(s) * secondsRadius);
    cv->setBrushColor(ink(Ink::Accent));
    cv->fillEllipse(cx, cy, 6, 6);
  }

};
