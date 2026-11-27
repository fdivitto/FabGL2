#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "Theme.h"
#include "TestModalDialog.h"
#include "TestPaintBoxFrame.h"
#include "TestTimerFrame.h"
#include "TestControlsFrame.h"
#include "TestNewControlsFrame.h"


class MyApp : public uiApp {

  static constexpr int TaskbarHeight = 40;

  uiFrame * taskbar;
  uiFrame * startMenu;
  uiButton * startButton;
  uiLabel * freeMemLabel1, * freeMemLabel2;

  TestPaintBoxFrame * paintBoxFrame;
  TestTimerFrame * testTimerFrame;
  TestControlsFrame * testControlsFrame;
  TestNewControlsFrame * testNewControlsFrame;

  fabgl::Stack<uiFrame*> dynamicFrames;

  void init() {

    // the dark theme of the library: message boxes and desktop included. Controls created without style class
    // get the theme by type, the style classes (uiThemeStyle) select the variants
    theme.apply(this);

    rootWindow()->onPaint = [&]() { paintDesktop(); };

    // application windows (hidden)
    testControlsFrame    = new TestControlsFrame(rootWindow());
    testNewControlsFrame = new TestNewControlsFrame(rootWindow());
    paintBoxFrame        = new TestPaintBoxFrame(rootWindow());
    testTimerFrame       = new TestTimerFrame(rootWindow());

    createTaskbar();
    createStartMenu();

    // show the free memory every 2s
    setTimer(this, 2000);
    onTimer = [&](uiTimerHandle tHandle) { showFreeMemory(); };
    showFreeMemory();

    showWindow(startMenu, true);
    setActiveWindow(startMenu);
  }

  // "bloom" wallpaper: concentric ellipses with dark blues
  void paintDesktop() {
    auto cv = rootWindow()->canvas();
    int cx = 420, cy = 250;
    cv->setBrushColor(ink(Ink::Bloom1));
    cv->fillEllipse(cx, cy, 560, 400);
    cv->setBrushColor(ink(Ink::Bloom2));
    cv->fillEllipse(cx + 20, cy + 10, 380, 260);
    cv->setBrushColor(ink(Ink::AccentDark));
    cv->fillEllipse(cx + 40, cy + 20, 160, 110);
    cv->setPenColor(ink(Ink::White));
    cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false).Italic(false).Underline(false).Invert(0));
    cv->drawText(BigFont, 20, 16, "FabGL");
    cv->drawText(NormalFont, 22, 44, "Graphical User Interface");
  }

  void createTaskbar() {
    int y = rootWindow()->clientSize().height - TaskbarHeight;
    taskbar = new uiFrame(rootWindow(), "", Point(0, y), Size(rootWindow()->clientSize().width, TaskbarHeight), true, uiThemeStyle::Taskbar);
    taskbar->onPaint = [&]() {
      // separator line on top
      auto cv = taskbar->canvas();
      cv->setPenColor(ink(Ink::Border));
      cv->drawLine(0, 0, taskbar->size().width - 1, 0);
    };

    startButton = new uiButton(taskbar, "Start", Point(8, 7), Size(72, 26), uiButtonKind::Button, true, uiThemeStyle::ButtonAccent);
    startButton->onClick = [&]() {
      showWindow(startMenu, !startMenu->state().visible);
      if (startMenu->state().visible)
        setActiveWindow(startMenu);
    };

    // pinned windows
    auto pin = [&](char const * text, int x, int width, uiFrame * frame) {
      auto button = new uiButton(taskbar, text, Point(x, 7), Size(width, 26), uiButtonKind::Button, true, uiThemeStyle::ButtonFlat);
      button->onClick = [this, frame]() { openFrame(frame); };
    };
    pin("Controls",     96,  78, testControlsFrame);
    pin("New Controls", 178, 98, testNewControlsFrame);
    pin("Paint Box",    280, 78, paintBoxFrame);
    pin("Clock",        362, 56, testTimerFrame);

    // free memory (right side)
    int w = 150;
    int x = taskbar->size().width - w - 10;
    freeMemLabel1 = new uiLabel(taskbar, "", Point(x, 5),  Size(w, 15), true, uiThemeStyle::LabelDim);
    freeMemLabel2 = new uiLabel(taskbar, "", Point(x, 20), Size(w, 15), true, uiThemeStyle::LabelDim);
    freeMemLabel1->labelStyle().textAlign = uiHAlign::Right;
    freeMemLabel2->labelStyle().textAlign = uiHAlign::Right;
  }

  void createStartMenu() {
    constexpr int Cols    = 3;
    constexpr int Rows    = 3;
    constexpr int TileW   = 126;
    constexpr int TileH   = 40;
    constexpr int Gap     = 6;
    constexpr int Margin  = 14;
    constexpr int TilesY  = 80;
    constexpr int W       = Margin * 2 + Cols * TileW + (Cols - 1) * Gap;
    constexpr int H       = TilesY + Rows * TileH + (Rows - 1) * Gap + Margin + 26;

    int y = rootWindow()->clientSize().height - TaskbarHeight - H - 8;
    startMenu = new uiFrame(rootWindow(), "", Point(8, y), Size(W, H), false, uiThemeStyle::Popup);
    startMenu->frameProps().moveable   = false;
    startMenu->frameProps().resizeable = false;

    new uiStaticLabel(startMenu, "FabGL GUI", Point(Margin, 12), true, uiThemeStyle::LabelTitle);
    new uiStaticLabel(startMenu, "Overlapping windows, mouse, keyboard and all the controls", Point(Margin, 34), true, uiThemeStyle::LabelDim);
    new uiStaticLabel(startMenu, "Tests", Point(Margin, 58), true, uiThemeStyle::Label);

    struct Tile { char const * text; void (MyApp::*action)(); };
    static const Tile Tiles[Rows * Cols] = {
      { "Controls",      &MyApp::onTestControls },
      { "New Controls",  &MyApp::onTestNewControls },
      { "Paint Box",     &MyApp::onTestPaintBox },
      { "Modal Dialog",  &MyApp::onTestModalDialog },
      { "Message Boxes", &MyApp::onTestMessageBoxes },
      { "Input Box",     &MyApp::onTestInputBox },
      { "Clock",         &MyApp::onTestTimer },
      { "Create Frame",  &MyApp::onCreateFrame },
      { "Destroy Frame", &MyApp::onDestroyFrame },
    };
    for (int i = 0; i < Rows * Cols; ++i) {
      int x = Margin + (i % Cols) * (TileW + Gap);
      int y = TilesY + (i / Cols) * (TileH + Gap);
      auto button = new uiButton(startMenu, Tiles[i].text, Point(x, y), Size(TileW, TileH), uiButtonKind::Button, true, uiThemeStyle::Button);
      auto action = Tiles[i].action;
      button->onClick = [this, action]() {
        showWindow(startMenu, false);
        (this->*action)();
      };
    }

    // footer: author
    int fy = H - 22;
    startMenu->onPaint = [this, fy]() {
      auto cv = startMenu->canvas();
      cv->setPenColor(ink(Ink::Border));
      cv->drawLine(1, fy - 6, startMenu->size().width - 2, fy - 6);
    };
    new uiStaticLabel(startMenu, "www.fabgl.com - by Fabrizio Di Vittorio", Point(Margin, fy), true, uiThemeStyle::LabelDim);
  }

  void openFrame(uiFrame * frame) {
    showWindow(frame, true);
    frame->bringOnTop();
    setActiveWindow(frame);
  }

  void showFreeMemory() {
    freeMemLabel1->setTextFmt("Free 8 bit: %d KiB", heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    freeMemLabel2->setTextFmt("Free 32 bit: %d KiB", heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024);
    freeMemLabel1->repaint();
    freeMemLabel2->repaint();
  }

  void onTestControls()    { openFrame(testControlsFrame); }
  void onTestNewControls() { openFrame(testNewControlsFrame); }
  void onTestPaintBox()    { openFrame(paintBoxFrame); }
  void onTestTimer()       { openFrame(testTimerFrame); }

  void onTestMessageBoxes() {
    messageBox("This is the title", "This is the main text", "Button1", "Button2", "Button3", uiMessageBoxIcon::Info);
    messageBox("This is the title", "This is the main text", "Yes", "No", nullptr, uiMessageBoxIcon::Question);
    messageBox("This is the title", "This is the main text", "OK",  nullptr, nullptr, uiMessageBoxIcon::Info);
    messageBox("This is the title", "This is the main text", "OK",  nullptr, nullptr, uiMessageBoxIcon::Error);
    messageBox("This is the title", "Little text", "OK",  nullptr, nullptr, uiMessageBoxIcon::Warning);
    messageBox("This is the title", "No icon", "OK",  nullptr, nullptr, uiMessageBoxIcon::None);
    messageBox("", "No title", "OK",  nullptr, nullptr);
  }

  void onCreateFrame() {
    static const Ink Backgrounds[] = { Ink::Window, Ink::Card, Ink::Hover, Ink::Black };
    uiFrame * newFrame = new uiFrame(rootWindow(), "", Point(110 + randomInt(330), 20 + randomInt(280)), Size(200, 90), true, uiThemeStyle::Frame);
    newFrame->setTitleFmt("Frame #%d", dynamicFrames.count());
    newFrame->frameStyle().backgroundColor = ink(Backgrounds[randomInt(4)]);
    auto label = new uiStaticLabel(newFrame, "FabGL - www.fabgl.com", Point(12, 36), true, uiThemeStyle::LabelTitle);
    label->anchors().left = false;
    label->anchors().top  = false;
    dynamicFrames.push(newFrame);
  }

  void onDestroyFrame() {
    if (!dynamicFrames.isEmpty())
      destroyWindow(dynamicFrames.pop());
  }

  void onTestModalDialog() {
    // show TestModalDialog as modal window
    auto testModalDialog = new TestModalDialog(rootWindow());
    showModalWindow(testModalDialog);
    destroyWindow(testModalDialog);
  }

  void onTestInputBox() {
    InputBox ib(this);
    ib.setAutoOK(5);
    //// simple message box
    ib.message("InputBox", "This is a message box using InputBox object");
    //// progress bar
    ib.progressBox("Example of Progress Bar", "Abort", true, 200, [&](fabgl::ProgressForm * form) {
      for (int i = 0; i <= 100; ++i) {
        if (!form->update(i, "Index is %d/100", i))
          break;
        vTaskDelay(pdMS_TO_TICKS(40));
      }
      vTaskDelay(pdMS_TO_TICKS(400));
    });
    //// menu
    int s = ib.menu("Example of simple Menu", "Click on one item", "Item number zero;Item number one;Item number two;Item number three");
    ib.messageFmt("", nullptr, "OK", "You have selected item %d", s);
    //// options selection box with OK button (items from separated strings) and autoOK of 5 seconds
    s = ib.select("Example of Menu with timeout", "Click on one item", "Item number zero;Item number one;Item number two;Item number three", ';', "Cancel", "OK");
    ib.messageFmt("", nullptr, "OK", "You have selected item %d", s);
  }

};
