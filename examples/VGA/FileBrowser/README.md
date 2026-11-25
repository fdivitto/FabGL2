# FileBrowser

A small **file manager** with a graphical user interface. It browses the internal flash (SPIFFS) and the SD card,
manages files and directories and **downloads files from the Internet** (HTTP and HTTPS).

The window (400x300, 64 colors) contains the file browser on the left, the command buttons on the right and, at
the bottom, the used/free space of the current drive and the WiFi status.

## Usage

The file browser root shows the available drives: `flash` (SPIFFS) and `SD` (SD card, if inserted). Double-click
(or press `Enter` on) a directory to enter it, `..` to go up.

| Button | Action |
|---|---|
| **Create Dir** | Creates a directory in the current folder |
| **Create Empty File** | Creates an empty file in the current folder |
| **Rename** | Renames the selected file or directory |
| **Delete** | Deletes the selected file or directory (asks a confirmation) |
| **Format** | Formats the current drive, or the drive selected in the root (asks a confirmation; all data is lost) |
| **Setup WiFi** | Asks network name and password, saves them in NVS and connects. At the next boot the example connects automatically |
| **Download** | Asks a URL (HTTP or HTTPS) and downloads the file into the current folder; the file name is the last part of the URL. An error message shows the HTTP code if the download fails |

Both drives are formatted automatically if they cannot be mounted.

## What it demonstrates

- `uiFileBrowser` control, `uiApp::inputBox()` and `messageBox()`
- `fabgl::FileBrowser`: SPIFFS and SD card mount, directory operations, `format()`, `getFSInfo()`
- `fabgl::WiFiConnection`, `fabgl::Preferences` (NVS) and `fabgl::HTTPRequest` (HTTPS with the certificate
  bundle, redirects)
- Memory tuning to run VGA, WiFi, TLS and the SD card together (see `sdkconfig.defaults`)

## Hardware

- VGA output (default pins)
- PS/2 keyboard on port 0 and PS/2 mouse on port 1
- Optional SD card. Default connections (TTGO VGA32 slot): MISO = GPIO16 (2 on PICO-D4 boards),
  MOSI = GPIO17 (12 on PICO-D4), CLK = GPIO14, CS = GPIO13

## Build

The example ships a `partitions.csv` (2 MB application + SPIFFS).
See [Building the examples](../../../README.md#building-the-examples) in the main README.
