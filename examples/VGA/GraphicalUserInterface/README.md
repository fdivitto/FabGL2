# GraphicalUserInterface

Tour of the **FabGL GUI** (`fabui.h`): overlapping windows, mouse and keyboard input, and all the standard
controls, at 640x480 with 16 colors.

The *tests* panel on the left contains one button per test, plus the free memory:

| Button | What it shows |
|---|---|
| **Test Controls** | A window with all the controls: label, text edit, normal / switch / split buttons, list box, normal and editable combo boxes, check box, radio buttons, horizontal and vertical sliders |
| **Create Frame** | Creates a new window with a random color and position (move, resize, minimize, maximize and close it) |
| **Destroy Frame** | Destroys the last created window |
| **Test Modal Dialog** | A modal window with a form (text edits in a panel): the rest of the UI is blocked until *Close* is pressed |
| **Test MessageBox** | A sequence of message boxes with 1-3 buttons and the different icons (info, question, error, warning, none) |
| **Test PaintBox** | A custom-painted chart with a horizontal scroll bar (`uiPaintBox`) |
| **Test Timer** | An analog clock window updated by a timer |
| **Test InputBox** | The `InputBox` dialogs (message, progress bar, menus) opened from inside a GUI application |

## Controls

- **Mouse**: click, drag window title bars to move them, drag window borders to resize, use the buttons in the
  title bar to minimize, maximize and close
- **Keyboard**: `Tab` / `Shift+Tab` move the focus, `Space` / `Enter` press the focused button, the arrow keys
  move in lists, combo boxes and sliders

## What it demonstrates

- `uiApp`, `uiFrame`, modal windows (`showModalWindow()`), `messageBox()`, timers (`setTimer()` / `onTimer`)
- Controls: `uiLabel`, `uiStaticLabel`, `uiTextEdit`, `uiButton`, `uiSplitButton`, `uiListBox`, `uiComboBox`,
  `uiCheckBox` (check box and radio button), `uiSlider`, `uiPanel`, `uiPaintBox`
- Anchors (controls that follow the window size), event handlers as lambdas
- `uiApp::runAsync()` to run the application in its own task

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 and PS/2 mouse on port 1

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
