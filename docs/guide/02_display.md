Choosing and configuring a display controller {#guide_display}
=============================================

[TOC]

# Available controllers {#display_controllers}

All bitmapped controllers derive from fabgl::BitmappedDisplayController and can be used with fabgl::Canvas,
fabgl::Terminal and fabgl::uiApp. They differ in the number of colors, in the memory used by the frame
buffer and in the CPU time needed to generate the signal.

| Controller | Output | Colors | Frame buffer | Frame buffer at 640x480 | CPU usage at 640x480 |
|---|---|---|---|---|---|
| fabgl::VGAController | VGA | 64 (8 with 5 GPIOs) | 1 byte per pixel | 300 KB (too large: use lower resolutions) | no signal generation load (DMA) |
| fabgl::VGA16Controller | VGA | 16 from a 64-color palette | 4 bits per pixel | about 153 KB | up to 19% of one core |
| fabgl::VGA8Controller | VGA | 8 from a 64-color palette | 3 bits per pixel | about 113 KB | up to 23% of one core |
| fabgl::VGA4Controller | VGA | 4 from a 64-color palette | 2 bits per pixel | about 75 KB | up to 11% of one core |
| fabgl::VGA2Controller | VGA | 2 from a 64-color palette | 1 bit per pixel | about 40 KB | up to 10% of one core |
| fabgl::VGATextController | VGA | 16, text only | character map | less than 50 KB in total | up to 30% of one core |
| fabgl::VGADirectController | VGA | 64 | none: you draw each scanline | a few scanline buffers | depends on your callback |
| fabgl::CVBS16Controller | PAL/NTSC composite | 16 from a palette | 4 bits per pixel | depends on the mode | depends on the mode |

How to choose:
- **Many colors at low resolution** (games, 320x200, 400x300): fabgl::VGAController.
- **High resolution user interfaces and terminals** (640x480 and above): fabgl::VGA16Controller, or
  fabgl::VGA2Controller / fabgl::VGA4Controller when memory is tight (WiFi, large applications).
- **Text only, maximum free memory**: fabgl::VGATextController (no Canvas, fixed 640x480, 80 columns).
- **Effects generated on the fly** (racing games, demos): fabgl::VGADirectController.
- **TV output**: fabgl::CVBS16Controller.

Only one display controller can be active at a time.


# GPIOs {#display_gpios}

VGA controllers offer three `begin()` overloads:

```cpp
DisplayController.begin();                       // 64 colors, default pinout:
                                                 // R = 22/21, G = 19/18, B = 5/4, HSync = 23, VSync = 15
DisplayController.begin(GPIO_NUM_22, GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18,
                        GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_23, GPIO_NUM_15);  // 64 colors, custom pinout
DisplayController.begin(GPIO_NUM_22, GPIO_NUM_19, GPIO_NUM_5,
                        GPIO_NUM_23, GPIO_NUM_15);                          // 8 colors, 5 GPIOs
```

The default pinout matches the TTGO VGA32 board. See @ref confVGA for the
resistor DAC schematic.

fabgl::CVBS16Controller uses one DAC output, for example `DisplayController.begin(GPIO_NUM_25)`.


# Resolutions and modelines {#display_resolution}

The resolution is set with `setResolution()`, using a Linux-style modeline. fabglconf.h defines ready-made
modelines, such as `VGA_320x200_75Hz`, `VGA_400x300_60Hz`, `VGA_640x350_70Hz`, `VGA_640x480_60Hz`,
`SVGA_800x600_56Hz` and `SVGA_1024x768_60Hz`:

```cpp
DisplayController.setResolution(VGA_640x480_60Hz);

// custom modeline
DisplayController.setResolution("\"640x480@60Hz\" 25.175 640 656 752 800 480 490 492 525 -HSync -VSync");
```

The optional `viewPortWidth` and `viewPortHeight` parameters reduce the drawable area (and the memory used)
while keeping the timings of the full resolution. The `VGA/ModelineStudio` example lets you tune modelines
interactively; once a good position is found, `moveScreen()` and `shrinkScreen()` can be used to adjust the image
on a specific monitor.

Composite modes are selected by name: "P-PAL-B", "I-PAL-B", "P-NTSC-M", "I-NTSC-M" and their "-WIDE"
variants ("P" = progressive, "I" = interlaced).


# Double buffering {#display_doublebuffer}

Pass `doubleBuffered = true` to `setResolution()` to allocate a second frame buffer. Drawing then always happens
on the back buffer, and fabgl::Canvas::swapBuffers() shows it at the next vertical retrace:

```cpp
DisplayController.setResolution(VGA_320x200_75Hz, -1, -1, true);
fabgl::Canvas cv(&DisplayController);

while (true) {
  cv.clear();
  // ... draw the whole frame ...
  cv.swapBuffers();
}
```

Double buffering doubles the frame buffer memory, so it is practical only at low resolutions.


# Palettes {#display_palette}

Palette-based controllers (VGA2/4/8/16 and CVBS16) map each pixel to a palette entry. Drawing with an RGB888
color selects the nearest palette entry. The palette can be changed with `setPaletteItem()`:

```cpp
DisplayController.setPaletteItem(0, RGB888(0, 0, 64));   // dark blue background
```

Changing a palette entry immediately changes all the pixels that use it.
