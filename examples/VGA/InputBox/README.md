# InputBox

Guided tour of **`fabgl::InputBox`**: ready-made modal dialogs (messages, menus, text input, selections, progress
bars, file and folder browsers) that need just one function call each, without writing a GUI.

The demo shows the dialogs one after the other (640x350 window on a 640x480 screen), then restarts:

1. **Welcome message**, closed automatically after 10 seconds (`setAutoOK()`): the countdown is shown in the button
2. **Progress bar** with an *Abort* button
3. **Simple menu** built from a `;`-separated string, and a second menu built from a `StringList`
4. **Items selection** with extra buttons: *Add* (asks a value with a text input), *Remove* (asks a
   confirmation) and *Options*, a button with a drop-down of sub-items (*Edit*, *Restore*, *Advanced*)
5. **Quiz**: a single-choice selection with a *Submit* button
6. **Text input**: asks your name and greets you
7. **Folder browser** and **file selector** on the SD card (`/SD`) or, if no SD card is present, on the internal
   SPIFFS partition (`/Flash`, formatted on first use)
8. **WiFi configuration wizard**: scans the networks (progress box), lets you pick one from a menu, asks the
   password (masked text input) and connects, showing the result
9. **Restart** message

## Controls

Every dialog can be used with the mouse or the keyboard:

| Key | Action |
|---|---|
| `Enter` | Default button (OK / Submit) |
| `Esc` | Cancel |
| `Up` / `Down` | Move the selection in menus and lists |
| `Tab` | Move the focus to the next control |

## What it demonstrates

- `InputBox::message()`, `messageFmt()`, `menu()`, `select()`, `textInput()`, `progressBox()`,
  `folderBrowser()`, `fileSelector()`, `setupButton()` (extra and drop-down buttons), `setAutoOK()`
- `fabgl::StringList`, `fabgl::WiFiScanner`, `fabgl::WiFiConnection`, `FileBrowser::mountSDCard()` /
  `mountSPIFFS()`
- Custom painting of the background (`InputBox::onPaint`)

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 and/or PS/2 mouse on port 1
- Optional SD card

## Build

The example ships a `partitions.csv` with a SPIFFS partition.
See [Building the examples](../../../README.md#building-the-examples) in the main README.
