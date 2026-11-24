Terminal {#guide_terminal}
========

[TOC]

fabgl::Terminal is an ANSI/VT100 compatible terminal that draws on any display controller (including
fabgl::VGATextController) and reads the PS/2 keyboard. It can be used in two ways: as a text console for the
application itself, or as a serial terminal connected to another computer.

Terminal derives from fabgl::Stream, so it offers `write()`, `print()`, `printf()`, `read()` and `available()`.


# Terminal as application console {#terminal_local}

```cpp
fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;
fabgl::Terminal        Terminal;

extern "C" void app_main()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);

  Terminal.begin(&DisplayController);          // 80 columns, rows depend on the resolution
  Terminal.connectLocally();                   // keyboard input is read by the application
  Terminal.enableCursor(true);

  Terminal.write("\e[1;33mHello\e[0m from the terminal\r\n");
  Terminal.printf("Size: %d x %d\r\n", Terminal.getColumns(), Terminal.getRows());

  while (true) {
    int c = Terminal.read(-1);                 // wait for a key
    if (c == 0x0D)
      Terminal.write("\r\n");
    else if (c >= 32 && c < 127)
      Terminal.write(c);
  }
}
```

Remember that the terminal follows the ANSI conventions: a new line needs `"\r\n"`.

fabgl::LineEditor provides line editing (cursor keys, insert/delete, home/end) on top of a terminal:

```cpp
fabgl::LineEditor ed(&Terminal);
Terminal.write("> ");
char const * line = ed.edit();
```

fabgl::TerminalController moves the cursor, changes colors and reads the cursor position without writing escape
sequences by hand.


# Serial terminal {#terminal_serial}

To connect the terminal to a serial port, use fabgl::SerialPort and fabgl::SerialPortTerminalConnector:

```cpp
fabgl::SerialPort                  SerialPort;
fabgl::SerialPortTerminalConnector SerialPortTerminalConnector;

// after Terminal.begin():
SerialPort.setSignals(GPIO_NUM_34, GPIO_NUM_2);                          // RX, TX
SerialPort.setup(2, 115200, 8, 'N', 1, FlowControl::Software);           // UART 2
SerialPortTerminalConnector.connect(&SerialPort, &Terminal);
```

From now on the characters received from the serial port are displayed, and the keys typed on the keyboard
are sent to the serial port. The `VGA/AnsiTerminal` example is a complete, configurable serial terminal.


# Emulations and FabGL sequences {#terminal_emulation}

The native mode is ANSI/VT100 (with many VT220+ extensions). Other terminals can be emulated with
Terminal::setTerminalType(), for example `TermType::ADM3A` or `TermType::VT52`.

Besides the standard sequences, Terminal understands FabGL-specific sequences (they start with `ESC _`) to draw
graphics, play sounds, read the mouse and control GPIOs: see @ref specialTermEscapes.


# Fonts and memory {#terminal_fonts}

Terminal chooses a font that fits the requested number of columns and rows; loadFont() selects a different
fixed-width font. The colors used for bold, italic and other attributes can be changed with
Terminal::setColorForAttribute().

The input queue (Terminal::inputQueueSize) and the task stack sizes must be set before begin(). Increase
inputQueueSize if characters are lost at high baud rates.
