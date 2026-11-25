# HardwareTest

Interactive **board test** with a graphical user interface, useful to check a newly assembled board
(for example the TTGO VGA32 or the Olimex ESP32-SBC-FabGL) piece by piece.

The window ("Hardware test", 640x480 VESA mode, 16 colors) contains one row per test:

| Test | How to use it | Expected result |
|---|---|---|
| **Colors** | Look at the 16 color boxes | 8 normal and 8 bright colors, all distinct. A missing color usually means a broken resistor or VGA pin |
| **Keyboard** | Click the text box and type | Typed characters appear |
| **Sound** | Press *Start* (it becomes *Stop*) | Random sine tones every 250 ms on the audio output (GPIO25) |
| **SD card** | Press *Start* | Mounts the SD card, writes and reads back a 16 KB temporary file: *Success!*, or *Mount Failed!* / *Write Failed!* / *Read Failed!* |
| **GPIO** | Choose an input GPIO (*In*) and an output GPIO (*Out*), click the output *State* button | The input *State* button shows the input level (0/1) in real time; the output GPIO follows its *State* button. Connect *Out* to *In* with a wire to test both |
| **Ext GPIO** | Shown only when an MCP23S17 expander is detected. Click A0...B2 to toggle the outputs, press *Play* for a LED animation | B3...B7 are inputs: they turn bright red when high; *INT* flashes yellow when the expander raises its interrupt line |
| **WiFi** | Press *Start* | Scans the WiFi networks: *Ok, Found N Networks* |

The GPIO lists contain only the pins not used by VGA, PS/2 and audio (inputs: 0, 1, 2, 3, 12, 13, 14, 16, 17, 18,
34, 35, 36, 39; outputs: 0, 1, 2, 3, 12, 13, 14, 16, 17, 18). GPIO 1 and 3 are the serial console pins.

## What it demonstrates

- A complete `uiApp` with frame, labels, color boxes, text edit, buttons, switch buttons, combo boxes and timers
- `fabgl::SoundGenerator`, `fabgl::FileBrowser` (SD card mount), `fabgl::WiFiScanner`, `fabgl::MCP23S17`
- Reducing `DisplayController.queueSize` to leave enough RAM for both WiFi and the SD card file system

## Hardware

- VGA output (default pins), audio on GPIO25
- PS/2 keyboard on port 0 and PS/2 mouse on port 1
- Optional: SD card (TTGO VGA32 slot), MCP23S17 GPIO expander (MISO = 35, or 2 on TTGO VGA32; MOSI = 12;
  CLK = 14; INT = 36)

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
