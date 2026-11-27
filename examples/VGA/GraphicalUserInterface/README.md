# GraphicalUserInterface

Tour of the **FabGL GUI** (`fabui.h`): overlapping windows, mouse and keyboard input, all the standard controls
and the newer ones, at 640x480 with 16 colors, with a modern dark flat theme (Metro like: near black surfaces,
thin gray lines, orange accent). The accent color can be changed in `main/Theme.h` (orange, cyan, violet, green).

The screen looks like a desktop: a wallpaper, a **taskbar** at the bottom and a **Start menu** with one tile per
test. The taskbar has the *Start* button (opens and closes the Start menu), buttons to open the main windows
directly, and the free memory on the right.

| Start menu tile | What it shows |
|---|---|
| **Controls** | The standard controls: text edit, normal / accent / switch / split buttons, list box, normal and editable combo boxes, check box, radio buttons, horizontal and vertical sliders |
| **New Controls** | The newer controls, connected to each other: tile selector and knobs that change the waveform drawn by a graph, toggle switches, a stepper that changes the octave of a piano keyboard, a volume level bar that drives the activity leds, a progress bar |
| **Paint Box** | A custom-painted chart with a horizontal scroll bar (`uiPaintBox`) |
| **Modal Dialog** | A modal window with a form (text edits in a card, buttons in the footer): the rest of the UI is blocked until *Close* is pressed |
| **Message Boxes** | A sequence of message boxes with 1-3 buttons and the different icons (info, question, error, warning, none) |
| **Input Box** | The `InputBox` dialogs (message, progress bar, menus) opened from inside a GUI application (they keep the default look) |
| **Clock** | An analog clock window updated by a timer (time since boot: the board has no RTC) |
| **Create Frame** | Creates a new window at a random position (move, resize, minimize, maximize and close it) |
| **Destroy Frame** | Destroys the last created window |

## Controls

- **Mouse**: click, drag window title bars to move them, drag window borders to resize, use the buttons in the
  title bar to minimize, maximize and close. Knobs: drag up/down or use the wheel. Piano: click the keys
- **Keyboard**: `Tab` / `Shift+Tab` move the focus, `Space` / `Enter` press the focused button, the arrow keys
  move in lists, combo boxes, sliders, knobs, steppers, tile selectors and level bars

## What it demonstrates

- The dark theme of the library (`fabgl::uiDarkTheme`, `main/Theme.h` selects the accent): it reprograms the 16
  color palette and styles every window and control, message boxes included. Style classes (`fabgl::uiThemeStyle`)
  select the variants: accent and flat buttons, titles, cards, dialog footer, popup, taskbar. Custom painting uses
  the theme colors through `theme.ink()`
- Styling options used by the theme: round radio buttons (`uiWindowStyle::cornerRadius`), a line below the title bar
  (`uiFrameStyle::titleSeparatorSize`), the size grip of resizable windows (`uiFrameProps::hasSizeGrip`), thicker
  check marks (`uiCheckBoxStyle::boldMark`)
- `uiApp`, `uiFrame`, modal windows (`showModalWindow()`), `messageBox()`, timers (`setTimer()` / `onTimer`),
  windows without title used as taskbar and popup menu, painting on the root window (wallpaper)
- Controls: `uiLabel`, `uiStaticLabel`, `uiTextEdit`, `uiButton`, `uiSplitButton`, `uiListBox`, `uiComboBox`,
  `uiCheckBox` (check box and radio button), `uiSlider`, `uiPanel`, `uiPaintBox`, `uiProgressBar`, `uiKnob`,
  `uiToggleSwitch`, `uiStepper`, `uiTileSelector` (tiles painted with `onPaintTile`), `uiGraph`, `uiLevelBar`,
  `uiLedBar`, `uiPianoKeyboard`
- Anchors (controls that follow the window size), event handlers as lambdas
- `uiApp::runAsync()` to run the application in its own task

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 and PS/2 mouse on port 1

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
