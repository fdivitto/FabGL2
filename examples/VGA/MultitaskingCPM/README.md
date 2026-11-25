# MultitaskingCPM

A **multitasking, multi-session CP/M 3** compatible system. Up to 12 independent Z80 machines run at the same time,
each one with its own screen, and you switch between them with the function keys. Every session runs a CP/M 3
compatible BDOS and CCP reimplemented in C++, with files stored as normal files on the SD card or on the internal
flash (SPIFFS). Watch it on [YouTube](https://www.youtube.com/watch?v=3UevsxMQZ5w).

## Getting started

At the first boot the system creates the drives and copies the included programs (this takes a while), then shows
the `A>` prompt. Type `HELP` for the list of the built-in commands.

```
A>HELP
A>LS BIN
A>CD DEMOS
A>MBASIC SOUND
```

At every boot each new session runs `PROFILE.SUB`, which sets the search path (`A:BIN;A:SDK`) and shows `INFO`.

## Sessions

| Key / command | Action |
|---|---|
| `F1` ... `F12` | Switch to session 1...12; a new session is created the first time its key is pressed |
| `TERM 0-11` | Switch to the specified session |
| `TERM AUX` | Start a new session connected to the serial port (TX = GPIO2, RX = GPIO34, 115200 8N1, XON/XOFF) |
| `TERM USB` | Start a new session connected to the USB serial port (use any terminal program on the PC) |
| `EXIT` | Close the current session (`EXIT n` closes session n) |

Each session needs about 20 KB of RAM: the number of sessions you can open depends on the free memory.

## Built-in commands

| Command | Description |
|---|---|
| `DIR` | Directory view |
| `LS` | Colored directory view |
| `CD` | Display or change the current directory (subdirectories are supported, e.g. `CD SDK`) |
| `ERA` (`ERASE`, `DELETE`, `DEL`, `RM`) | Remove one or more files |
| `REN` (`RENAME`) | Rename one or more files |
| `TYPE` (`CAT`) | Display a text file |
| `PATH` | Get or set the directories searched for programs |
| `MKDIR` (`MD`) / `RMDIR` | Create / remove a directory |
| `COPY` (`CP`) | Copy files |
| `INFO` / `DINFO` | System info / debug info (sessions, memory) |
| `EMU n` | Terminal emulation of the session (`EMU` alone lists them: ANSI, ADM 3A, ADM 31, Hazeltine 1500, Osborne, Kaypro, VT52, ANSI Legacy) |
| `KEYB xx` | Keyboard layout: `US`, `UK`, `DE`, `IT`, `ES`, `FR`, `BE`, `NO` |
| `WIFISCAN` | Scan the WiFi networks |
| `WIFI ssid password` | Connect to a WiFi network. Example: `WIFI mynet mypass` |
| `PING host` | Ping a host. Example: `PING www.fabgl.com` |
| `TELNET host` | Telnet to a host. Example: `TELNET towel.blinkenlights.nl` |
| `FORMAT` | Erase the SPIFFS or SD card and restore the programs at the next boot |
| `REBOOT` | Restart the system |
| `HELP` (`?`) | Show the help |

`Ctrl+C` stops the running program (and `PING` / `TELNET`).

## Drives and included programs

| Drive | Location |
|---|---|
| A: | `driveA` directory on the SD card, or on SPIFFS if no SD card is present |
| B: | `driveB` directory (empty) |

Programs copied to drive A: at the first boot:

| Directory | Programs |
|---|---|
| `\` | `PROFILE.SUB` (startup script of each session) |
| `BIN` | CP/M 3 utilities: DATE, GET, PUT, SETDEF, SUBMIT, SHOW, PIP, ERASE, ED, DIR, DEVICE, TYPE, SAVE, RENAME, DUMP |
| `SDK` | MBASIC, MAC, RMAC, LINK, LIB, HEXCOM, GENCOM, ZSID, VDE (text editor) |
| `GAMES` | CATCHUM, GORILLA |
| `DEMOS` | BASIC programs that use the FabGL terminal sequences: BLINK, GPIOREAD, ADC, ADCVOLTS, SOUND, MOUSE, GRAPH, SPRITE (`CD DEMOS`, then `MBASIC SOUND`); `SAY.COM` text-to-speech with the sample texts `SPEECH1.TXT`...`SPEECH4.TXT` |

`SAY` speaks English text, phonemes in brackets, or a text file:

```
A>SAY Hello world
A>CD DEMOS
A>SAY SPEECH1.TXT
```

More programs (Turbo Pascal 3, SBASIC, LADDER, ZEXALL/ZEXDOC, GREP, DIFF, ...) are available in
`main/src/programs.h` but excluded when WiFi is enabled, to save flash space: comment out `#define HAS_WIFI` in
`main/src/defs.h` to include them (then run `FORMAT` so they are copied again).

To add your own programs, convert them with `xxd -i` and add them to the `programs[]` table in
`main/src/programs.h` (see the instructions at the top of that file), or copy them to the SD card directly from a
PC: the `driveA` and `driveB` directories are normal FAT directories.

## What it demonstrates

- The `fabgl::Z80` CPU core, running several virtual machines in separate FreeRTOS tasks
- A CP/M 3 compatible BDOS, BIOS and CCP implemented in C++ on top of the ESP-IDF file system
  (`fabgl::FileBrowser`, SD card or SPIFFS)
- Several `fabgl::Terminal` instances sharing one display, each with its own emulation, and terminals connected to
  the UART (`fabgl::SerialPort`)
- WiFi, ping and telnet from inside an emulated machine
- Reducing the default queue and stack sizes of the library to save RAM

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0, optional PS/2 mouse on port 1 (used by `MOUSE.BAS`)
- Optional SD card (TTGO VGA32 slot: MISO = GPIO16 (2 on PICO-D4), MOSI = GPIO17 (12 on PICO-D4), CLK = GPIO14,
  CS = GPIO13)
- Optional audio on GPIO25 (`SOUND.BAS`, `SAY.COM`)

## Build

The example ships a `partitions.csv` (2 MB application + SPIFFS).
See [Building the examples](../../../README.md#building-the-examples) in the main README.
