# VIC20

Emulator of the **Commodore VIC-20** home computer: MOS 6502 CPU, VIC 6561 video and sound chip (PAL), two VIA
6522, memory expansions up to 35 KB, cartridges, joystick and an emulated disk drive. It includes 22 programs (games
and demos) and can download many more from the Internet. Watch it on
[YouTube](https://www.youtube.com/watch?v=ZW427HVWYys).

## Getting started

At boot the emulator copies the included programs into the `VIC20Store` directory (on the SD card if present,
otherwise on the internal SPIFFS partition) and shows the **configuration screen**:

- on the left, the **program list** (the `VIC20Store` directory)
- on the right, the buttons and the settings
- at the bottom, the free space and the download buttons

Select a program and press `Enter` (or double-click it, or press **Load**): the emulator sets the needed RAM
expansion, loads the program and runs it. Press `F12` to go back to the configuration screen at any time, and `F12`
(or `Esc`) again to return to the VIC-20.

## Configuration screen

| Control | Function |
|---|---|
| **Run [F12]** | Runs (or resumes) the VIC-20 |
| **Load** | Loads and runs the selected program |
| **Soft Reset** | Resets the VIC-20 (the cartridge stays inserted) |
| **Hard Reset** | Removes the cartridge and resets the VIC-20 |
| **HELP [F1]** | Shows the keyboard shortcuts |
| **Keyboard Layout** | Layout of the PS/2 keyboard (US, UK, DE, IT, ...) |
| **RAM Expansion** | Unexpanded, 3K, 8K, 16K, 24K, 27K (24K+3K), 32K, 35K (32K+3K). Set automatically from the file name when a program is loaded |
| **Joystick** | *None*, *Cursor Keys* or *Mouse* (see below) |
| **Download From: Web** | Downloads all the programs listed in `main/progs/weblist.h` (about 90 games and cartridges from zimmers.net). Asks for the WiFi network the first time |
| **Download From: URL** | Downloads a single program: type the URL and then the file name to give it (see the naming rules below) |

In the program list, `Del` deletes the selected file or directory (asks a confirmation).

## Program files and naming rules

Programs are `.PRG` files (loaded in RAM and started with `RUN`) or `.CRT` files (cartridge images, inserted and
started with a reset). The **file name tells the emulator how to load the program**:

```
PROGNAME [EXP] [ADDRESS].EXT
```

| Part | Meaning |
|---|---|
| `PROGNAME` | Program name |
| `EXP` | RAM expansion needed: `3K`, `8K`, `16K`, `24K`, `27K`, `32K`, `35K`. Omit it for unexpanded programs |
| `ADDRESS` | Cartridges only: load address `20` (2000h), `40` (4000h), `60` (6000h) or `A0` (A000h, default) |
| `EXT` | `PRG` or `CRT` |

Examples: `Popeye 32K.PRG`, `Quikman 8K.PRG`, `Aggressor.CRT`.

Cartridges made of several ROM files: give all the parts the same name with a different address, for example
`Battlezone 60.CRT` and `Battlezone A0.CRT`; loading one of them loads all the parts.

You can also copy `.PRG` and `.CRT` files to the `VIC20Store` directory of the SD card from a PC.

Included programs: Arukanoido, Astro Panic, Blue Star, Dragonwing, Frogger07, Froggie, Kikstart, Kweepout,
Nibbler, Omega Fury, Pooyan, Popeye, Pulse, Quikman, Spikes, Splatform, Tank Battalion, Tammerfors, Tetris+, and
the demos Bah Bah, Birthday, Digit.

## Emulated disk drive

The VIC-20 sees the `VIC20Store` directory as disk drive **8**:

```
LOAD "$",8          (directory)
LIST
LOAD "NAME",8
SAVE "NAME",8
```

## Keys while the VIC-20 runs

| PC key | VIC-20 key / action |
|---|---|
| `F12` | Back to the configuration screen |
| `Esc` | RUN/STOP |
| `Home` | CLR/HOME |
| `Backspace` | INST/DEL |
| `Delete` | RESTORE |
| Left `Win` | CBM (Commodore key) |
| `F1` ... `F8` | F1 ... F8 |
| `^` | Up arrow |
| `_` | Left arrow |
| `~` | Pi |
| `Left Alt` + `A` / `S` / `W` / `Z` | Move the screen left / right / up / down (to center it on the monitor) |

### Joystick

| Joystick mode | Move | Fire |
|---|---|---|
| None | `Right Alt` + cursor keys | `Right Alt` + `Menu` |
| Cursor Keys | Cursor keys | `Menu` (the key next to the right `Ctrl`) |
| Mouse | Move the mouse | Any mouse button |

## What it demonstrates

- The `fabgl::MOS6502` CPU core, with emulated VIC 6561 (video and audio), VIA 6522 and IEC serial bus drive
- Rendering an emulated video chip on a `fabgl::VGAController` frame buffer
- A GUI (`uiApp`, `uiFileBrowser`) to manage programs, settings saved with `fabgl::Preferences`
- HTTP and HTTPS downloads with `fabgl::HTTPRequest` (see the TLS memory settings in `sdkconfig.defaults`)

## Hardware

- VGA output (default pins)
- Audio output on GPIO25
- PS/2 keyboard on port 0, optional PS/2 mouse on port 1
- Optional SD card (TTGO VGA32 slot)

## Build

The example ships a `partitions.csv` (3 MB application + SPIFFS).
See [Building the examples](../../../README.md#building-the-examples) in the main README.
