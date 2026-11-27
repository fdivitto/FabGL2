# PCEmulator

**IBM PC/XT emulator**: Intel 8086/8088 CPU, 1 MB of RAM, CGA (text, 320x200 and 640x200 graphics) and Hercules
(720x348) video, PC speaker, PS/2 keyboard and mouse, two floppy drives, two hard disks and a serial port.
It runs MS-DOS, FreeDOS, CP/M-86, Linux ELKS, Windows 3.0, GEM, GEOS and many DOS programs and games.
Watch it on [YouTube](https://www.youtube.com/watch?v=3I1U2nEoxIQ).

## Requirements

- An ESP32 board **with PSRAM** (the emulated RAM lives in PSRAM), for example TTGO VGA32 v1.4 or the Olimex
  ESP32-SBC-FabGL
- An **SD card** (FAT32): disk images and the machine configurations are stored on it
- WiFi, the first time, to download the disk images (or copy them to the SD card from a PC)
- VGA monitor, PS/2 keyboard, optional PS/2 mouse, audio on GPIO25 for the PC speaker

## Getting started

1. Insert the SD card and flash the example.
2. If the date and time are not known yet (cold boot) the emulator asks to configure the WiFi: choose a network and
   type the password. The time is taken from an NTP server (`pool.ntp.org`) and the board restarts.
   Without WiFi the date is set to 14 August 1984.
3. The **launcher** appears. Choose a machine and press **Run** (if you do nothing, the last used machine starts
   after 6 seconds).
4. If the disk images of the machine are not on the SD card yet, they are downloaded from www.fabglib.org (a
   progress bar shows the progress; big images take some minutes, dropped connections are resumed
   automatically). An image is downloaded as `<name>.part` and renamed only when complete: a download that
   fails, is aborted or is interrupted by a reset or a power loss is resumed from the `.part` file the next
   time the machine is selected. After the download the board restarts and boots the machine.

### Predefined machines

| Machine | Disk image |
|---|---|
| FreeDOS (floppy 1.44 MB) | `floppy_FREEDOS.img` |
| MS-DOS 3.31 (floppy 1.44 MB) | `floppy_MSDOS331.img` |
| CP/M 86 + Turbo Pascal 3 (floppy 1.44 MB) | `floppy_CPM86.img` |
| Linux ELKS 0.4.0 (floppy 1.44 MB) | `floppy_ELK040.img` |
| FreeDOS (HDD 8 MB) | `hd8_FREEDOS.img` |
| DOS Programs and Games (HDD 20 MB) | `hd20_DOSPROG.img` |
| DOS Programming Tools (HDD 20 MB) | `hd20_DOSDEV.img` |
| Windows 3.0 (HDD 20 MB) | `hd20_WINHERC.img` |
| GEM 3.13 (HDD 8 MB) | `hd8_GEM31.img` |
| GEOS 2.0 (HDD 20 MB) | `hd20_GEOS20.img` |
| MS-DOS 5.00 (HDD 8 MB) | `hd8_MSDOS500.img` |
| MS-DOS 6.22 (HDD 8 MB) | `hd8_MSDOS622.img` |
| SvarDOS (HDD 10 MB) | `hd10_SVARDOS.img` |

All the images are downloaded from the `downloads` folder of www.fabglib.org into the root of the SD card.
You can also download them with a PC and copy them to the SD card.

## Launcher

The start screen (dark theme of the library, `fabgl::uiDarkTheme`, at 512x384 with 16 colors) lists the machine
configurations on the left. Each one shows the boot drive (floppy or hard disk icon) and the state of its disk
images: **Ready** (all on the SD card), **Download needed**, **Download incomplete** (a `.part` file is resumed) or
**Disk image missing** (a local file that is not on the SD card). On the right the details of the selected machine:
boot drive and, for each drive, the image, where it is (size on the SD card, or downloaded at start) and the hard disk
geometry.

The selected machine starts by itself after 6 seconds when the board is switched on: a key or the mouse stops the
countdown.

| Button | Key | Action |
|---|---|---|
| **Run** | `Enter`, double click | Starts the selected machine (downloading its missing disk images) |
| **Edit** | `F2` | Edits the selected configuration |
| **New** | `Ins` | Creates a new configuration |
| **Remove** | `Del` | Removes the selected configuration |
| **Files** | `F3` | Browses the files and directories on the SD card |
| **WiFi** | `F4` | Selects the WiFi network and saves its password (used when an image must be downloaded) |

The configuration editor has these fields:

| Field | Meaning |
|---|---|
| Description | Name shown in the list |
| Floppy A / Floppy B | Image of drive A: / B: (a file on the SD card, or an HTTP / HTTPS URL to download it) |
| Hard Disk C / Hard Disk D | Image of the first / second hard disk, with its geometry (Cylinders, Heads, Sectors; leave 0 to compute it from the file size) |
| Boot from | Floppy A, Floppy B, Hard Disk C or Hard Disk D |
| **...** | Selects an image file from the SD card |
| **Create Disk Image** | Creates a new empty disk image: formatted floppy (320K, 360K, 720K, 1.2M, 1.44M, 2.88M, FAT12) or unformatted hard disk of the given size in MB (partition and format it from DOS with `FDISK` and `FORMAT`) |

`Enter` saves, `Esc` cancels. All the dialogs (WiFi, downloads, file browser, messages) use the same theme
(`InputBox::onInitApp`).

The configurations are saved in `mconfs.txt` in the root of the SD card. It is a text file, one machine per line,
that can also be edited on a PC:

```
desc "FreeDOS (A:) + DOS Programming Tools (C:)"  fd0 https://www.fabglib.org/downloads/floppy_FREEDOS.img  hd0 https://www.fabglib.org/downloads/hd20_DOSDEV.img
desc "My Own MSDOS" hd0 HDD_10M.IMG chs0 306,4,17 boot hd0
desc "Machine1" fd0 msdos3/disk1.img
```

Tags: `desc` (description), `fd0` / `fd1` (floppy A / B), `hd0` / `hd1` (hard disks), `chs0` / `chs1` (hard disk
geometry: cylinders,heads,sectors), `boot` (`fd0`, `fd1`, `hd0`, `hd1`; default `fd0`). File names are relative to
the SD card root. Delete `mconfs.txt` to restore the predefined list.

## Using images

- **Floppy images** must be raw sector images (`.img`, `.ima`) of 160K, 180K, 320K, 360K, 720K, 1.2M, 1.44M or
  2.88M.
- **Hard disk images** are raw images (no VHD header). When no geometry is given, it is computed from the file
  size (63 sectors per track, up to 16 heads). If the image was partitioned with a different geometry, specify it
  with `chs0` / `chs1` (or in the Cyls / Head / Sect fields).
- To exchange files with a PC, create a floppy image (*Create Disk*), mount it on the PC (e.g. with a disk image
  tool or `mtools`), copy the files, put it back on the SD card and mount it in the emulator (see *Mount Disk*
  below).

## Keys while the emulator runs

| Keys | Action |
|---|---|
| `Alt` + `PrtScr` (SysReq: press both, then release `PrtScr` and `Alt`) | Opens the emulator menu |

Emulator menu:

| Command | Action |
|---|---|
| Restart Emulator | Restarts the board and shows the machine list again |
| Restart Machine | Resets the emulated PC (like the reset button) |
| Mount Disk | Changes the image of Floppy A (fd0) or Floppy B (fd1) without restarting: useful for programs on several disks |
| Continue | Returns to the emulated PC |

The PS/2 mouse is seen by the PC as a PS/2 mouse: load a DOS mouse driver that supports PS/2 mice (for example
`CTMOUSE`) to use it in DOS programs.

## Serial port and I/O expander

- **COM1** is connected to the USB serial port (TX = GPIO1, RX = GPIO3): use a terminal program on the PC to talk
  with DOS programs (for example `CTTY COM1`, or a communication program).
- If a **MCP23S17** GPIO expander is connected, its pins are available on the I/O ports `E0h`...`E9h`. See the
  GW-BASIC and QBasic examples in [Extras/PC Emulator BASIC Programs](../../../Extras/PC%20Emulator%20BASIC%20Programs).

## What it demonstrates

- The `fabgl::i8086` CPU core and the emulated chips of the library: `PIC8259`, `PIT8253`, `i8042` (keyboard and
  mouse controller), `PC8250` (UART), `MC146818` (RTC), and the CGA/Hercules `GraphicsAdapter` rendered with
  `VGADirectController`
- PSRAM mapped and accessed directly as emulated RAM (`CONFIG_SPIRAM_USE_MEMMAP`)
- `fabgl::InputBox` for the setup UI, `fabgl::HTTPRequest` for HTTPS downloads with resume (HTTP Range),
  WiFi + SNTP for the real-time clock
- Running network and SD card work on the core not used by the VGA interrupt (`CoreUsage::quietCore()`)

## Build

The example ships a `partitions.csv` with a 3 MB application partition, and an `sdkconfig.defaults` that enables
PSRAM. See [Building the examples](../../../README.md#building-the-examples) in the main README.
