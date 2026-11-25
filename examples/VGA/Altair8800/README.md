# Altair8800

Emulator of the **MITS Altair 8800** computer with Intel 8080 or Zilog Z80 CPU, 64 KB of RAM, up to four
88-DCDD 8" floppy disk drives and serial I/O boards. It boots **CP/M** (1.4, 2.2, 3), Altair Disk BASIC or
Altair DOS from the disk images included in the firmware. Watch it on
[YouTube](https://www.youtube.com/watch?v=y0opVifEyS8).

The video output is a 80x25 text terminal (ADM-31 emulation by default) on VGA; the keyboard is a PS/2 keyboard.

## Getting started

Flash the example and wait for the `A>` prompt: drive A contains CP/M 2.2, drive B contains games.

```
A>DIR
A>B:
B>DIR
B>LADDER
```

Some programs to try:

| Disk | Programs |
|---|---|
| A: CP/M 2.2 (`cpm22_dsk`) | MBASIC, ASM, MAC, M80/L80, DDT, ED, PIP, STAT, SUBMIT, DUMP, SURVEY, FORMAT, PCGET/PCPUT |
| B: Games (`games_dsk`) | LADDER, CATCHUM, ALIENS, OTHELLO, DUMPSTAR, and in MBASIC: STARTRK, LUNAR, CHASE, TICTAK (`MBASIC STARTRK`) |

If a game draws garbage, configure it for the terminal: `LADCONF` (Ladder) and `CATCONF` (Catchum) select the
terminal type, or change the emulated terminal from the emulator menu.

## Disk drives

| Drive | Default content | Writable |
|---|---|---|
| A: | CP/M 2.2 system disk (image in flash) | With an SD card: yes (copied to `diskA.dsk` on the card). Without: read only |
| B: | Games disk (image in flash) | Same as A: (`diskB.dsk`) |
| C: | `diskC.dsk` on SD card or SPIFFS | Yes. **Format it first** with `FORMAT C:` |
| D: | `diskD.dsk` on SD card or SPIFFS | Yes. **Format it first** with `FORMAT D:` |

The emulator uses the SD card if present, otherwise the internal SPIFFS partition (formatted automatically on first
use). Files on drives C: and D: survive a reboot.

### Changing the disk images

The included images are selected at compile time with the `DRIVE_A` ... `DRIVE_D` defines at the top of
`main/Altair8800.cpp`:

```cpp
#define DISKFORMAT Disk_338K
#define DRIVE_A cpm22_dsk     // image in flash
#define DRIVE_B games_dsk     // image in flash
#define DRIVE_C "diskC.dsk"   // file on SD card / SPIFFS
#define DRIVE_D "diskD.dsk"
```

Available 8" images (`DISKFORMAT Disk_338K`, folder `main/disks`):

| Image | Content | Use as |
|---|---|---|
| `cpm22_dsk` | CP/M 2.2 system | A |
| `games_dsk` | Games | B |
| `turbopascal3_dsk` | Turbo Pascal 3 | B |
| `wordstar3_dsk`, `wordstar4_dsk` | WordStar 3 / 4 | B |
| `multiplan_dsk` | Multiplan (+ WordStar) | B |
| `dbaseii_dsk` | dBase II (+ WordStar) | B |
| `BDSC_dsk` | BDS C compiler | B |
| `langs_dsk` | Languages: BBC BASIC, Forth 83, DX-Forth, XLISP, ... | B |
| `cpm141_dsk` | CP/M 1.4 system | A |
| `cpm3_disk1_dsk` | CP/M 3 system (requires the Z80 CPU) | A |
| `cpm3_disk2_dsk`, `cpm3_build_dsk` | CP/M 3 utilities and system build files | B |
| `DOS_1_dsk`, `DOS_2_dsk` | Altair DOS | A |
| `basic5_dsk` | Altair Disk BASIC 5 | A |

Mini disk images (5.25", `DISKFORMAT MiniDisk_76K`, folder `main/minidisks`): `cpm22_disk1_minidsk`,
`cpm22_disk2_minidsk`, `basic300_5F_minidisk` (requires the i8080 CPU).

Altair DOS and Disk BASIC ask some questions at boot. Suggested answers are in the comments next to their
`#include` lines in `Altair8800.cpp` (for example Disk BASIC: `MEMORY SIZE?` Enter, `LINEPRINTER?` `C`, then
Enter to the remaining questions).

Each 8" image is 337568 bytes (77 tracks x 32 sectors x 137 bytes). To add your own image, convert the raw
Altair `.dsk` file to a C header containing a `const uint8_t name[] = { ... };` array (for example with
`xxd -i`), include it and assign it to a drive.

## Emulator menu (`F12` or `Pause`)

Press `F12` or `Pause` at any time to open the emulator menu. Press the letter of an option, `Enter` to go back to
the emulated machine.

| Key | Option |
|---|---|
| `Z` | Reset the Altair (reboot) |
| `S` | Send a disk image to the serial port (USB) |
| `R` | Receive a disk image from the serial port (USB) |
| `F` | Format the file system (SD card or SPIFFS): deletes all the disk files! |
| `U` | CPU: Z80 / i8080 (reset required) |
| `P` | Real CPU speed: emulate the original 2 MHz speed, or run as fast as possible |
| `T` | Terminal emulation: ANSI, ADM 3A, ADM 31, Hazeltine 1500, Osborne, Kaypro, VT52, ANSI Legacy |
| `K` | Keyboard layout |
| `C` | Colors: Green/Black, Yellow/Black, White/Black, Black/White, Yellow/Blue, Black/Yellow |
| `L` | Rows: 24, 25 or max |
| `G` | CRT mode (retro scanlines, only with the graphical display controller, see below) |
| `O` | Reset the configuration to the defaults |

The settings are saved in NVS.

## Transferring files with a PC

The Altair has three serial boards: SIO at port 00h (TTY) and 10h (CRT) are connected to the screen and keyboard,
SIO at port 12h is connected to the **USB serial port** (115200 baud). The scripts in [tools](tools) use it
(they need Python 3 with `pyserial`, and `xmodem` for `transfile.py`: `pip3 install pyserial xmodem`):

**Single files (XMODEM)**: run `PCGET` (receive) or `PCPUT` (send) on the Altair, then on the PC:

```
A>PCGET MYFILE.COM                                   (on the Altair)
python3 tools/transfile.py MYFILE.COM to /dev/ttyUSB0      (on the PC)

A>PCPUT MBASIC.COM                                   (on the Altair)
python3 tools/transfile.py MBASIC.COM from /dev/ttyUSB0    (on the PC)
```

**Whole disk images**: open the emulator menu and choose `R` (receive) or `S` (send) and the drive letter, then on
the PC:

```
python3 tools/transdisk.py mydisk.dsk to /dev/ttyUSB0      (PC -> Altair, after "R")
python3 tools/transdisk.py mydisk.dsk from /dev/ttyUSB0    (Altair -> PC, start this before "S")
```

Close any serial monitor before running the scripts.

## What it demonstrates

- The `fabgl::i8080` and `fabgl::Z80` CPU cores
- `fabgl::Terminal` emulations (ADM-31, ADM-3A, VT52, ...) used as the console of an emulated computer
- `fabgl::VGATextController`: 80x25 text with a very small memory footprint. Comment out
  `#define USE_TEXTUAL_DISPLAYCONTROLLER` to use `fabgl::VGA16Controller` instead (enables the CRT mode)
- `fabgl::FileBrowser` (SD card / SPIFFS) for writable disk files, `fabgl::Preferences` for the settings

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 (CLK = GPIO33, DATA = GPIO32)
- Optional SD card (TTGO VGA32 slot: MISO = GPIO16 (2 on PICO-D4), MOSI = GPIO17 (12 on PICO-D4), CLK = GPIO14,
  CS = GPIO13)

## Build

The example ships a `partitions.csv` (2 MB application + SPIFFS for the writable disks).
See [Building the examples](../../../README.md#building-the-examples) in the main README.
