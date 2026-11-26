Examples {#guide_examples}
========

The `examples/` folder contains complete ESP-IDF projects that show how to use every part of the library: from
small demos of a single feature to complete applications such as emulators of retro computers, terminals and games.
Each example has its own page with the purpose of the demo, what it demonstrates, the required hardware and how to
use it (commands, keys, disk images...). The page of each example ends with its source code.


# Building the examples {#examples_build}

Every folder under `examples/` that contains an `sdkconfig.defaults` file is a standalone ESP-IDF project
(ESP-IDF 6.1 or newer):

```
. $IDF_PATH/export.sh
cd examples/VGA/SpaceInvaders
idf.py build flash monitor
```

To build an example copied outside the FabGL repository, set the `FABGL_PATH` environment variable to the root of
the repository.

To build or clean several examples at once:

```
tools/build_examples.sh            # all examples
tools/build_examples.sh VGA/       # VGA examples only
tools/clean_examples.sh            # remove the build output of all examples
```

Some examples ship a `partitions.csv` file (larger application partition, SPIFFS partition for files): it is used
automatically by `idf.py`.

Default pins (as on the TTGO VGA32 board), used by all the examples:

| Signal | GPIO |
|---|---|
| VGA Red (MSB, LSB) | 22, 21 |
| VGA Green (MSB, LSB) | 19, 18 |
| VGA Blue (MSB, LSB) | 5, 4 |
| VGA HSync, VSync | 23, 15 |
| PS/2 port 0 (keyboard) CLK, DATA | 33, 32 |
| PS/2 port 1 (mouse) CLK, DATA | 26, 27 |
| Audio output (DAC) | 25 (23 with composite video, sigma-delta) |
| Composite video output | 25 |

The "default pins" mentioned in the example pages are the ones of this table.


# VGA examples {#examples_vga}

| Example | Description |
|---|---|
| @subpage example_Altair8800 "Altair8800" | Altair 8800 emulator (i8080/Z80) running CP/M 1.4, 2.2, 3, Altair BASIC and DOS from the included disk images |
| @subpage example_AnsiTerminal "AnsiTerminal" | Serial VT/ANSI terminal with on-screen configuration (emulations, fonts, resolutions, serial parameters) |
| @subpage example_Audio "Audio" | Multi-channel sound mixer GUI: waveform generators and sampled sounds |
| @subpage example_ClassicRacer "ClassicRacer" | Two-player retro racing game (keyboard, mouse or joystick) |
| @subpage example_CollisionDetection "CollisionDetection" | Sprites and pixel-precise collision detection with `Scene` |
| @subpage example_DirectVGA "DirectVGA" | Minimal direct scanline rendering with `VGADirectController` |
| @subpage example_DirectVGA_ElMuro "DirectVGA_ElMuro" | Brick-breaker game rendered line by line with `VGADirectController` |
| @subpage example_DoubleBuffer "DoubleBuffer" | Double-buffered animation benchmark (balls, polygons, sprites, FPS) |
| @subpage example_FileBrowser "FileBrowser" | File manager for SPIFFS and SD card, with WiFi setup and HTTP/HTTPS downloads |
| @subpage example_GraphicalUserInterface "GraphicalUserInterface" | Tour of the GUI: windows, controls, modal dialogs, message boxes, timers |
| @subpage example_HardwareTest "HardwareTest" | Board test: colors, keyboard, sound, SD card, GPIOs, MCP23S17, WiFi |
| @subpage example_InputBox "InputBox" | Ready-made dialogs: messages, menus, text input, progress bars, file selector, WiFi wizard |
| @subpage example_LoopbackTerminal "LoopbackTerminal" | Terminal connected to itself, to try the escape sequences from the keyboard |
| @subpage example_ModelineStudio "ModelineStudio" | Find and tune VGA modelines from the serial console |
| @subpage example_MouseOnScreen "MouseOnScreen" | Mouse pointer, cursor shapes and drawing with the mouse |
| @subpage example_MultitaskingCPM "MultitaskingCPM" | Up to 12 concurrent CP/M 3 compatible Z80 sessions, with WiFi, telnet and ping |
| @subpage example_NetworkTerminal "NetworkTerminal" | WiFi command shell with telnet client, ping and network scan |
| @subpage example_PCEmulator "PCEmulator" | IBM PC/XT emulator (8086, CGA/Hercules) running DOS, Windows 3.0, ELKS, GEM... (requires PSRAM and SD card) |
| @subpage example_SimpleTerminalOut "SimpleTerminalOut" | Terminal output showcase: colors, styles, graphics, animations |
| @subpage example_SimpleTextTerminalOut "SimpleTextTerminalOut" | The same showcase with the text-only VGA controller |
| @subpage example_Songs "Songs" | Songs written as text strings, played with a software ADSR sound chip |
| @subpage example_SoundChipSimulator "SoundChipSimulator" | Catalog of retro sound effects (ADSR envelope + frequency sweep) |
| @subpage example_SpaceInvaders "SpaceInvaders" | Arcade shoot-'em-up game with sprites and sampled sounds |
| @subpage example_Sprites "Sprites" | The simplest sprite example, with the supported bitmap formats |
| @subpage example_Synth "Synth" | 8-voice polyphonic synthesizer with ADSR, resonant ladder filter (with envelope), LFO and 64 presets, played from the PS/2 keyboard |
| @subpage example_TerminalCharSet "TerminalCharSet" | Shows the terminal character set with codes |
| @subpage example_VIC20 "VIC20" | Commodore VIC-20 emulator with included games, cartridges and Internet downloads |

# Composite video examples {#examples_composite}

| Example | Description |
|---|---|
| @subpage example_BaseExample "BaseExample" | PAL/NTSC composite video test screen with mode selection |

# Other examples {#examples_others}

| Example | Description |
|---|---|
| @subpage example_KeyboardScanCodes "KeyboardScanCodes" | Shows the raw PS/2 keyboard scancodes |
| @subpage example_KeyboardVirtualKeys "KeyboardVirtualKeys" | Shows the virtual keys and ASCII codes generated by the keyboard, for each layout |
| @subpage example_MouseStudio "MouseStudio" | PS/2 mouse diagnostics on the serial console |
