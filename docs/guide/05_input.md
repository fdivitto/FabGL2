Keyboard and mouse {#guide_input}
==================

[TOC]

# Initializing the PS/2 ports {#input_init}

fabgl::PS2Controller handles up to two PS/2 ports and creates the fabgl::Keyboard and fabgl::Mouse objects.
Call `begin()` once, before using the keyboard or the mouse:

```cpp
fabgl::PS2Controller PS2Controller;

PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1);   // port 0: GPIO 33/32, port 1: GPIO 26/27

fabgl::Keyboard * keyboard = PS2Controller.keyboard();
fabgl::Mouse *    mouse    = PS2Controller.mouse();
```

The preset tells what is connected to each port (see fabgl::PS2Preset). The second parameter (fabgl::KbdMode)
selects how keyboard events are processed:

| KbdMode | What you can use |
|---|---|
| KbdMode::NoVirtualKeys | raw scancodes only (Keyboard::getNextScancode()) |
| KbdMode::GenerateVirtualKeys | Keyboard::isVKDown(), events delivered to uiApp and Terminal |
| KbdMode::CreateVirtualKeysQueue (default) | everything above, plus the virtual key queue (Keyboard::getNextVirtualKey()) |

Devices are detected during `begin()`: use Keyboard::isKeyboardAvailable() and Mouse::isMouseAvailable() to
check that they are present. Custom GPIOs can be assigned with the other PS2Controller::begin() overload; see
@ref confPS2 for the connection circuit.


# Reading the keyboard {#input_keyboard}

The keyboard driver translates scancodes into *virtual keys* (fabgl::VirtualKey, such as `fabgl::VK_a`,
`fabgl::VK_RETURN`, `fabgl::VK_LEFT`), independent of the keyboard layout, and, when possible, into ASCII characters:

```cpp
VirtualKeyItem item;
while (true) {
  if (keyboard->getNextVirtualKey(&item)) {        // waits for the next key event
    if (item.down) {
      if (item.ASCII)
        printf("character '%c'\n", item.ASCII);
      else if (item.vk == fabgl::VK_F1)
        printf("F1 pressed\n");
      if (item.CTRL && item.vk == fabgl::VK_c)
        printf("CTRL-C\n");
    }
  }
}
```

fabgl::VirtualKeyItem also reports the state of SHIFT, CTRL, ALT, GUI and the lock keys at the time of the event.
To test whether a key is currently held down (useful in games), use `keyboard->isVKDown(fabgl::VK_SPACE)`.

The default layout is US. Select another one with Keyboard::setLayout():

```cpp
keyboard->setLayout(&fabgl::ItalianLayout);   // also UK, German, Spanish, French, Belgian, Norwegian, Japanese
```

The keyboard LEDs are controlled with Keyboard::setLEDs(), and the typematic rate with Keyboard::setTypematicRateAndDelay().


# Reading the mouse {#input_mouse}

The mouse reports relative movements (fabgl::MouseDelta):

```cpp
MouseDelta delta;
if (mouse->getNextDelta(&delta, 0))                // 0 = do not wait
  printf("dx = %d dy = %d wheel = %d left = %d\n",
         delta.deltaX, delta.deltaY, delta.deltaZ, delta.buttons.left);
```

Most applications prefer an absolute position on the screen. setupAbsolutePositioner() converts the movements
into a position inside the given area and, when a display controller is passed, moves the mouse pointer
automatically:

```cpp
mouse->setupAbsolutePositioner(DisplayController.getViewPortWidth(), DisplayController.getViewPortHeight(),
                               false, &DisplayController);
DisplayController.setMouseCursor(CursorName::CursorPointerShadowed);

auto & status = mouse->status();                  // updated in the background
printf("x = %d y = %d\n", status.X, status.Y);
```

When the `createAbsolutePositionsQueue` parameter is true, each change of position or buttons is also queued
and can be read with Mouse::getNextStatus().

uiApp sets up the absolute positioner by itself: GUI applications do not need any of the code above.
