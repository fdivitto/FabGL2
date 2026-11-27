Graphical user interface {#guide_gui}
========================

[TOC]

FabGL offers two levels of user interface:
- fabgl::InputBox: ready-made modal dialogs (messages, text input, lists, menus, progress, file selection),
  usable with a few lines of code and no event handling;
- fabgl::uiApp: a complete windowing system with overlapping frames, controls, keyboard focus, mouse and timers.


# Quick dialogs with InputBox {#gui_inputbox}

```cpp
fabgl::PS2Controller PS2Controller;
fabgl::InputBox      ib;

extern "C" void app_main()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  ib.begin(VGA_640x480_60Hz, 500, 400, 4);        // creates its own 4-color VGA controller

  char name[32] = "";
  if (ib.textInput("Setup", "Your name", name, sizeof(name) - 1) == InputResult::Enter) {
    int choice = ib.select("Setup", "Favorite color", "Red;Green;Blue");
    ib.messageFmt("Setup", nullptr, "OK", "Hello %s, you chose %d", name, choice);
  }

  ib.end();
}
```

InputBox can also use an existing display controller (`ib.begin(&DisplayController)`) or run inside a uiApp.


# Structure of a uiApp application {#gui_structure}

An application derives from fabgl::uiApp, creates its windows and controls in init() and is started with run():

```cpp
fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;

class MyApp : public uiApp {

  uiLabel * label;

  void init() override {
    rootWindow()->frameStyle().backgroundColor = RGB888(0, 0, 64);

    auto frame = new uiFrame(rootWindow(), "My window", Point(50, 50), Size(300, 150));

    label = new uiLabel(frame, "Press the button", Point(10, 30));

    auto button = new uiButton(frame, "Click me", Point(10, 60), Size(80, 22));
    button->onClick = [&]() {
      label->setText("Clicked!");
      label->update();
    };
  }

} app;

extern "C" void app_main()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);

  app.run(&DisplayController);     // returns when quit() is called
}
```

Key points:
- **Windows and controls are created with `new` and owned by their parent.** They are destroyed automatically
  when the parent is destroyed or when the application ends: never `delete` them yourself (use
  uiApp::destroyWindow() to remove one at run time).
- Every window has a parent. The root window (rootWindow()) covers the whole screen; frames (fabgl::uiFrame)
  are movable, resizable windows with a title bar; controls (buttons, labels, ...) live inside frames.
- Positions are relative to the parent window.
- run() executes the event loop in the calling task. runAsync() runs it in a new task.


# Events {#gui_events}

Events are delivered through *delegates*: public members to which you assign a function or a lambda.

| Delegate | Available on | Called when |
|---|---|---|
| onClick | uiButton, uiCheckBox, uiLabel, uiListBox | the control is clicked (or activated with the keyboard) |
| onChange | uiTextEdit, uiCheckBox, uiSlider, uiComboBox, uiListBox, uiButton (switch kind), uiKnob, uiToggleSwitch, uiLevelBar, uiTileSelector | the value changes |
| onSelect | uiSimpleMenu, uiSplitButton | an item is chosen |
| onStep | uiStepper | an arrow button is pressed (-1 or +1) |
| onNoteOn, onNoteOff | uiPianoKeyboard | a key is pressed or released (MIDI note number) |
| onKeyDown, onKeyUp | uiFrame and derived windows | a key is pressed or released while the frame is active |
| onPaint | uiFrame (no parameters), uiPaintBox, uiGraph (receive an area) | the window must be repainted |
| onShow, onHide, onResize | uiFrame | the frame is shown, hidden or resized |
| onTimer | uiFrame, uiApp | a timer created with setTimer() expires |

```cpp
auto edit = new uiTextEdit(frame, "", Point(10, 30), Size(150, 20));
edit->onChange = [=]() { printf("text: %s\n", edit->text()); };

frame->onKeyUp = [&](uiKeyEventInfo const & key) {
  if (key.VK == fabgl::VK_ESCAPE)
    quit(0);
};
```

Event handlers run in the GUI task, so they can safely create, modify and destroy windows. From other tasks,
send an event with uiApp::postEvent() instead of touching windows directly.


# Updating the screen {#gui_repaint}

Changing a property (text, color, items...) does not always repaint the control: call `repaint()` on the
control (or `update()` on labels, which also recalculates their size). Repainting is queued and performed by the
GUI task.

To draw custom content, use a fabgl::uiPaintBox or the onPaint delegate of a frame, and draw with the frame
canvas:

```cpp
frame->onPaint = [=]() {
  auto cv = frame->canvas();
  cv->setPenColor(Color::BrightRed);
  cv->drawLine(0, 0, 100, 100);
};
```

List boxes (fabgl::uiListBox, fabgl::uiFileBrowser...) paint their items with the onPaintItem delegate when it is
assigned: icons, columns, tags. The item background is already filled with the selection colors:

```cpp
fileBrowser->onPaintItem = [=](int index, Rect const & r, bool selected) {
  auto cv   = fileBrowser->canvas();
  auto item = fileBrowser->content().get(index);
  cv->drawText(&fabgl::FONT_std_12, r.X1 + 14, r.Y1 + 2, item->name);   // leaves room for an icon
};
```


# Timers and modal windows {#gui_timers}

```cpp
auto timer = setTimer(frame, 1000);                 // fires frame->onTimer every second
frame->onTimer = [&](uiTimerHandle t) { /* ... */ };
killTimer(timer);
```

A modal window blocks the rest of the interface until it is closed:

```cpp
auto dlg = new uiFrame(rootWindow(), "Question", UIWINDOW_PARENTCENTER, Size(200, 100), false);
auto ok  = new uiButton(dlg, "OK", Point(70, 60), Size(60, 20));
ok->onClick = [=]() { dlg->exitModal(1); };
int result = showModalWindow(dlg);                  // returns the value passed to exitModal()
destroyWindow(dlg);
```

For simple questions use uiApp::messageBox(), uiApp::inputBox() and uiApp::fileDialog().


# Styles {#gui_styles}

Every window and control has style structures (frameStyle(), windowStyle(), buttonStyle(), labelStyle(), ...)
that can be changed directly. To style many controls consistently, derive from fabgl::uiStyle, assign a style
class ID to each control (last constructor parameter) and install the style with uiApp::setStyle():

```cpp
enum { STYLE_NONE, STYLE_BUTTON };

struct MyStyle : uiStyle {
  void setStyle(uiObject * object, uint32_t styleClassID) override {
    if (styleClassID == STYLE_BUTTON)
      ((uiButton *) object)->buttonStyle().backgroundColor = RGB888(64, 64, 128);
  }
} myStyle;

// in init():
setStyle(&myStyle);
new uiButton(frame, "Styled", Point(10, 10), Size(80, 20), uiButtonKind::Button, true, STYLE_BUTTON);
```

The windows created by uiApp::messageBox() and uiApp::inputBox() get the style classes listed in
fabgl::uiDialogStyleClasses (uiAppProps::dialogStyleClasses), so they can match the application theme:

```cpp
appProps().dialogStyleClasses.frame  = STYLE_DIALOG;
appProps().dialogStyleClasses.label  = STYLE_DIALOG_LABEL;
appProps().dialogStyleClasses.button = STYLE_BUTTON;
```

The @ref example_VIC20 example themes its whole interface this way (custom painted program list, tile selectors,
dialogs).


# Synth and instrument controls {#gui_synth_controls}

These controls are custom painted and fully styleable. They were designed for audio applications, but are
generic:

| Control | Description |
|---|---|
| fabgl::uiKnob | rotary knob with a ring of dots: drag up/down, mouse wheel, keyboard; double click restores the default value. The text below the knob is set with setValueText() / setValueTextFmt() inside onChange |
| fabgl::uiToggleSwitch | on/off switch with an optional "ON" / "OFF" text |
| fabgl::uiLevelBar | horizontal segmented bar (led meter look), read-only or editable with the mouse and the keyboard |
| fabgl::uiLedBar | row of leds, each with a state (off or one of three colors) |
| fabgl::uiStepper | a text between "previous" and "next" arrow buttons |
| fabgl::uiTileSelector | row of selectable tiles painted by the application (onPaintTile), with optional captions |
| fabgl::uiGraph | plots a sequence of values, optionally filling the area below the curve; onPaintBackground and onPaint add markers and labels |
| fabgl::uiPianoKeyboard | on-screen piano keyboard played with the mouse (with glissando) or by the application (press() / release()) |

```cpp
auto cutoff = new uiKnob(frame, "CUTOFF", Point(10, 30), Size(60, 76));
cutoff->onChange = [=]() {
  synth.filter().setCutoff(ResonantLowPassFilter::cutoffFromPosition(cutoff->position()));
  cutoff->setValueTextFmt("%d Hz", synth.filter().cutoff());
};
cutoff->setup(0, 100, 70);                 // range and default position (calls onChange)

auto piano = new uiPianoKeyboard(frame, Point(10, 120), Size(400, 120));
piano->onNoteOn  = [&](int note) { synth.noteOn(note); };
piano->onNoteOff = [&](int note) { synth.noteOff(note); };
```

The @ref example_Synth example uses all of them, themed with a fabgl::uiStyle.

