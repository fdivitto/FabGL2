# ModelineStudio

Interactive tool to **find and tune VGA modelines** for a specific monitor. The screen (2 colors) shows the
current resolution and a frame around the visible area; every change is applied immediately and the resulting
modeline is printed on the serial console, ready to be copied into your code
(`DisplayController.setResolution("...")`).

## What it demonstrates

- The predefined resolutions (`VGA_640x480_60Hz`, `SVGA_800x600_56Hz`, ... up to 1280x768)
- `VGAController::setResolution()` with a modeline string or a `VGATimings` structure
- `moveScreen()` and `shrinkScreen()` to adjust position and size of the picture
- `fabgl::VGA2Controller`: the 2-color controller, which allows the highest resolutions with little memory

## Hardware

- VGA output (default pins)
- USB serial connection (115200 baud): all the commands are typed in the serial monitor
  (for example `idf.py -p PORT monitor`)

## Commands

Single characters typed in the serial monitor:

| Key | Command |
|---|---|
| `h` | Print the help |
| `+` / `-` | Next / previous predefined resolution (43 presets, starts at 640x480) |
| `x` | Restore the current preset (cancels moves, shrinks and timing changes) |
| `w` / `z` / `a` / `s` | Move the picture up / down / left / right |
| `n` / `N` | Shrink / enlarge horizontally |
| `m` / `M` | Shrink / enlarge vertically |
| `e` / `E` | Decrease / increase the horizontal front porch |
| `r` / `R` | Decrease / increase the horizontal sync pulse |
| `t` / `T` | Decrease / increase the horizontal back porch |
| `y` / `Y` | Decrease / increase the vertical front porch |
| `u` / `U` | Decrease / increase the vertical sync pulse |
| `i` / `I` | Decrease / increase the vertical back porch |
| `o` / `O` | Decrease / increase the pixel clock by 5 kHz |
| `.` | Change the order of the horizontal signals (front porch / sync / back porch / visible area first) |

Porch and sync changes keep the total line (or frame) length constant, so the refresh rate does not change.

To try a custom modeline, paste it in the serial monitor (it must start with `"` and end with Enter), for example:

```
"640x350@70Hz" 25.175 640 656 752 800 350 366 462 510 -HSync -VSync
```

After each command the console shows the modeline, the `moveScreen()`/`shrinkScreen()` calls to reproduce the
adjustments, screen and viewport size and free memory.

## Build

See [Building the examples](../../../README.md#building-the-examples) in the main README.
