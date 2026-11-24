Memory, performance and troubleshooting {#guide_tips}
=======================================

[TOC]

# Memory {#tips_memory}

The frame buffer is the largest memory user. It is allocated in internal RAM, split into several
blocks to make use of fragmented memory. While allocating it, FabGL always leaves at least
FABGLIB_MINFREELARGESTBLOCK bytes (40 KB) free for the application.

**If there is not enough memory, the viewport height is reduced instead of failing.** If the bottom of the screen
is missing, check `DisplayController.getViewPortHeight()` and, if needed:
- use a controller with fewer colors (fabgl::VGA16Controller instead of fabgl::VGAController, or VGA4/VGA2);
- use a lower resolution, or a smaller viewport (`viewPortWidth` and `viewPortHeight` parameters of setResolution());
- avoid double buffering;
- initialize WiFi and other memory-hungry components before the display, or lower their buffers.

Use `heap_caps_get_free_size(MALLOC_CAP_8BIT)` and `heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)` to check
the available memory. PSRAM cannot hold the frame buffer, but applications can use it for their own data
(the `VGA/PCEmulator` example needs it).


# CPU cores and tasks {#tips_cores}

- Palette-based VGA controllers and fabgl::VGATextController generate the signal with an interrupt handler that
  runs on the core given by FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE (the core not used by WiFi). Keep heavy
  application work on the other core: the examples run `app_main()` on CPU 1 for this reason.
- Long-running loops that never block can trigger the task watchdog: insert a `vTaskDelay()` or disable the idle
  task check on the busy core (`CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n`, as in the examples).
- Drawing, GUI and terminal calls should come from one task (see @ref canvas_queue). Use uiApp::postEvent() to
  communicate with a GUI from other tasks.


# Troubleshooting {#tips_troubleshooting}

| Symptom | Things to check |
|---|---|
| No image or "out of range" on the monitor | Try another modeline (`VGA_640x480_60Hz` is accepted by almost every monitor); check the HSync and VSync GPIOs |
| Image shifted or cut on one side | Adjust it with moveScreen() and shrinkScreen(), or use `VGA/ModelineStudio` |
| Lower part of the screen missing | Not enough memory: see @ref tips_memory |
| Flickering | Drawing outside the vertical retrace (enableBackgroundPrimitiveExecution(false)), too many or too large sprites, or WiFi on the video core |
| Keyboard or mouse not responding | `CONFIG_ULP_COPROC_ENABLED`, `CONFIG_ULP_COPROC_TYPE_FSM` and `CONFIG_ULP_COPROC_RESERVE_MEM` (>= 2112) must be set; check the PS2Preset and the GPIOs; check isKeyboardAvailable() / isMouseAvailable() |
| No keys from getNextVirtualKey() | PS2Controller must be started with KbdMode::CreateVirtualKeysQueue (the default) |
| No sound | Call play(true) or use playSound(); enable the waveform generators; check the GPIO (25 for DAC) and the volume |
| Characters lost in the serial terminal | Increase Terminal::inputQueueSize and enable flow control |


# Porting from FabGL 1.x {#tips_porting}

FabGL2 no longer supports Arduino. The [migration guide](../MIGRATION.md) lists the changed APIs and
the replacements for Arduino classes (Preferences, Serial, WiFi, HTTP).
