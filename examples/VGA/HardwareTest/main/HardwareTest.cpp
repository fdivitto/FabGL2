/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2026 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


 /*

  MCP23S17 pins:
    MISO = 35   (2 on TTGO VGA32)
    MOSI = 12
    CLK  = 14
    CS   = none (13 on FabGL development board, shared and inverted with MicroSD)
    INT  = 36 (interrupt pin connected to INTA of MCP23S17)
 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "fabgl.h"


// random integer in the range [0, max)
static inline int randomInt(int max) { return max > 0 ? (int) (esp_random() % (uint32_t) max) : 0; }
// random integer in the range [min, max)
static inline int randomInt(int min, int max) { return max > min ? min + (int) (esp_random() % (uint32_t) (max - min)) : min; }

#include "fabui.h"
#include "devdrivers/MCP23S17.h"
#include "network/netutils.h"



fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;

// dark theme of the library without setupPalette(): the palette keeps the 16 standard colors for the color test,
// the theme colors are approximated by the display (the green accent becomes bright green)
fabgl::uiDarkTheme     theme(fabgl::uiThemeAccent::Green);


#define MCP_INT 36


struct TestApp : public uiApp {

  uiFrame *        frame;
  uiButton *       soundButton;
  SoundGenerator * soundGenerator;
  uiTimerHandle    soundTimer;
  uiButton *       sdButton;
  uiStaticLabel *  sdResultLabel;
  uiComboBox *     gpioInComboBox;
  uiButton *       gpioInState;
  uiComboBox *     gpioOutComboBox;
  uiButton *       gpioOutState;
  uiTimerHandle    gpioTimer;
  uiButton *       wifiButton;
  uiLabel *        wifiResultLabel;
  uiLabel *        extgpioLabel[16];
  uiButton *       extgpioPlay;
  uiLabel *        extIntLabel;

  int              gpioIn, gpioOut;
  bool             gpioInPrevState;
  int              intOffDelay;

  fabgl::MCP23S17  mcp;


  ~TestApp() {
    delete soundGenerator;
  }


  void init() {

    // the theme styles every window and control, message boxes included
    theme.apply(this);

    frame = new uiFrame(rootWindow(), "Hardware test", UIWINDOW_PARENTCENTER, Size(555, 380));

    constexpr int x = 125;   // controls column (labels on the left)
    int y = frame->clientPos().Y + 4;

    // colors
    static const char * COLORSL[8] = { "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White" };
    static const char * COLORSH[8] = { "B. Black", "B. Red", "B. Green", "B. Yellow", "B. Blue", "B. Magenta", "B. Cyan", "B. White" };
    for (int i = 0; i < 8; ++i) {
      int left = 60;
      int hspc = 60;
      int vspc = 30;
      new uiStaticLabel(frame, COLORSL[i], Point(left + i * hspc, y), true, fabgl::uiThemeStyle::LabelDim);
      new uiColorBox(frame, Point(left + i * hspc, y + 15), Size(50, 14), (Color)i);
      new uiStaticLabel(frame, COLORSH[i], Point(left + i * hspc, y + vspc), true, fabgl::uiThemeStyle::LabelDim);
      new uiColorBox(frame, Point(left + i * hspc, y + 15 + vspc), Size(50, 14), (Color)(i+ 8));
    }

    y += 80;

    // keyboard test
    new uiStaticLabel(frame, "KEYBOARD TEST:", Point(10, y));
    new uiTextEdit(frame, "Please type something", Point(x, y - 3), Size(420, 22));

    y += 40;

    // sound test
    soundGenerator = new SoundGenerator;
    new uiStaticLabel(frame, "SOUND TEST:", Point(10, y));
    soundButton = new uiButton(frame, "Start", Point(x, y - 3), Size(80, 22), uiButtonKind::Switch);
    soundButton->onChange = [&]() {
      if (soundButton->down()) {
        soundButton->setText("Stop");
        soundTimer = setTimer(frame, 250);
      } else {
        soundButton->setText("Start");
        killTimer(soundTimer);
      }
    };

    y += 40;

    // SD Card test
    new uiStaticLabel(frame, "SDCARD TEST:", Point(10, y));
    sdButton = new uiButton(frame, "Start", Point(x, y - 3), Size(80, 22));
    sdButton->onClick = [&]() {
      testSD();
    };
    sdResultLabel = new uiStaticLabel(frame, "", Point(x + 100, y));

    y += 44;

    // internal GPIOs test
    static const int GPIOS_IN[] = { 0, 1, 2, 3, 12, 13, 14, 16, 17, 18, 34, 35, 36, 39 };
    static const int GPIOS_OUT[] = { 0, 1, 2, 3, 12, 13, 14, 16, 17, 18 };
    new uiStaticLabel(frame, "GPIO TEST:", Point(10, y));

    new uiStaticLabel(frame, "In", Point(x, y - 20), true, fabgl::uiThemeStyle::LabelDim);
    gpioInComboBox = new uiComboBox(frame, Point(x, y - 3), Size(38, 22), 80);
    for (int i = 0; i < sizeof(GPIOS_IN) / sizeof(int); ++i)
      gpioInComboBox->items().appendFmt("%d", GPIOS_IN[i]);
    gpioInComboBox->onChange = [&]() {
      gpioIn = gpioInComboBox->selectedItem() > -1 ? GPIOS_IN[gpioInComboBox->selectedItem()] : GPIOS_IN[10];
      fabgl::configureGPIO((gpio_num_t)(gpioIn), GPIO_MODE_INPUT);
    };
    gpioIn = GPIOS_IN[10];
    fabgl::configureGPIO((gpio_num_t)(gpioIn), GPIO_MODE_INPUT);
    gpioInComboBox->selectItem(10);
    new uiStaticLabel(frame, "State", Point(x + 46, y - 20), true, fabgl::uiThemeStyle::LabelDim);
    gpioInState = new uiButton(frame, "", Point(x + 46, y - 3), Size(25, 22), uiButtonKind::Switch);
    gpioInPrevState = 0;

    new uiStaticLabel(frame, "Out", Point(x + 110, y - 20), true, fabgl::uiThemeStyle::LabelDim);
    gpioOutComboBox = new uiComboBox(frame, Point(x + 110, y - 3), Size(38, 22), 80);
    for (int i = 0; i < sizeof(GPIOS_OUT) / sizeof(int); ++i)
      gpioOutComboBox->items().appendFmt("%d", GPIOS_OUT[i]);
    gpioOutComboBox->onChange = [&]() {
      gpioOut = gpioOutComboBox->selectedItem() > -1 ? GPIOS_OUT[gpioOutComboBox->selectedItem()] : GPIOS_OUT[2];
      fabgl::configureGPIO((gpio_num_t)(gpioOut), GPIO_MODE_OUTPUT);
      updateGPIOOutState();
    };
    gpioOut = GPIOS_OUT[2];
    fabgl::configureGPIO((gpio_num_t)(gpioOut), GPIO_MODE_OUTPUT);
    gpioOutComboBox->selectItem(2);
    new uiStaticLabel(frame, "State", Point(x + 156, y - 20), true, fabgl::uiThemeStyle::LabelDim);
    gpioOutState = new uiButton(frame, "", Point(x + 156, y - 3), Size(25, 22), uiButtonKind::Switch);
    gpioOutState->setDown(false);
    gpioOutState->onChange = [&]() {
      updateGPIOOutState();
    };
    updateGPIOOutState();

    gpioTimer = setTimer(frame, 100);

    y += 44;


    // external GPIOs test

    if (initMCP()) {
      new uiStaticLabel(frame, "EXT GPIO TEST:", Point(10, y));
      new uiStaticLabel(frame, "Outputs", Point(x, y - 12), true, fabgl::uiThemeStyle::LabelDim);
      new uiStaticLabel(frame, "Inputs", Point(x + 11 * 22, y - 12), true, fabgl::uiThemeStyle::LabelDim);
      constexpr int w = 20;
      constexpr int h = 22;
      for (int i = 0; i < 16; ++i) {
        extgpioLabel[i] = new uiLabel(frame, "", Point(x + i * (w + 2), y + 3), Size(w, h), true, fabgl::uiThemeStyle::LabelDim);
        extgpioLabel[i]->labelStyle().textColor = theme.ink(fabgl::uiThemeInk::Black);
        extgpioLabel[i]->labelStyle().textAlign = uiHAlign::Center;
        extgpioLabel[i]->setTextFmt("%c%d", i < 8 ? 'A' : 'B', i & 7);
        if (i >= MCP_B3) {
          // configure as IN
          mcp.configureGPIO(i, fabgl::MCPDir::Input);
          extgpioLabel[i]->labelStyle().backgroundColor = RGB888(64, 0, 0);
          // generate a flag/interrupt whenever input changes
          mcp.enableInterrupt(i, fabgl::MCPIntTrigger::PreviousChange);
        } else {
          // configure as OUT
          mcp.configureGPIO(i, fabgl::MCPDir::Output);
          extgpioLabel[i]->labelStyle().backgroundColor = RGB888(0, 64, 0);
          auto pmcp = &mcp;
          extgpioLabel[i]->onClick = [=, this]() {
            extGPIOSet(i, !pmcp->readGPIO(i));
          };
        }
      }

      // MCP_INT pin
      extIntLabel = new uiLabel(frame, "INT", Point(x + 16 * (w + 2), y + 3), Size(25, h), true, fabgl::uiThemeStyle::LabelDim);
      extIntLabel->labelStyle().textColor = theme.ink(fabgl::uiThemeInk::Black);
      extIntLabel->labelStyle().textAlign = uiHAlign::Center;
      extIntLabel->labelStyle().backgroundColor = RGB888(64, 64, 0);
      mcp.enableINTMirroring(true); // INTA = INTB
      fabgl::configureGPIO((gpio_num_t)(MCP_INT), GPIO_MODE_INPUT);

      // play button
      extgpioPlay = new uiButton(frame, "Play", Point(x + 16 * (w + 2) + 31, y + 3), Size(42, 22));
      extgpioPlay->onClick = [&]() { playExtGPIOS(); };

      y += 50;
    }


    // wifi
    new uiStaticLabel(frame, "WIFI TEST:", Point(10, y));
    wifiButton = new uiButton(frame, "Start", Point(x, y - 3), Size(80, 22));
    wifiButton->onClick = [&]() {
      testWifi();
    };
    wifiResultLabel = new uiLabel(frame, "", Point(x + 100, y));


    // timer
    frame->onTimer = [&](uiTimerHandle h) {
      // sound test
      if (h == soundTimer) {
        soundGenerator->playSound(SineWaveformGenerator(), randomInt(100, 3000), 230, 100);
      }
      // internal and external GPIOs
      if (h == gpioTimer) {
        updateGPIOInState();
      }
    };


    y = frame->size().height - 36;
    new uiStaticLabel(frame, "FabGL - Copyright 2019-2026 by Fabrizio Di Vittorio", Point(175, y), true, fabgl::uiThemeStyle::LabelDim);
    new uiStaticLabel(frame, "WWW.FABGL.COM", Point(240, y + 15), true, fabgl::uiThemeStyle::LabelDim);

  }


  void updateGPIOOutState() {
    if (gpioOutState->down()) {
      gpioOutState->setText("1");
      gpio_set_level((gpio_num_t)(gpioOut), 1);
    } else {
      gpioOutState->setText("0");
      gpio_set_level((gpio_num_t)(gpioOut), 0);
    }
  }

  void updateGPIOInState() {
    // read internal GPIOs
    if (gpioInPrevState != gpio_get_level((gpio_num_t)(gpioIn))) {
      gpioInPrevState = !gpioInPrevState;
      gpioInState->setText(gpioInPrevState ? "1" : "0");
      gpioInState->repaint();
    }
    // read external GPIOs
    if (mcp.available()) {
      // check MCP_INT has been triggered correctly
      if (gpio_get_level((gpio_num_t)(MCP_INT)) == 0) {
        extIntLabel->labelStyle().backgroundColor = RGB888(255, 255, 0);
        extIntLabel->repaint();
        intOffDelay = 2;
      } else if (--intOffDelay == 0) {
        extIntLabel->labelStyle().backgroundColor = RGB888(64, 64, 0);
        extIntLabel->repaint();
      }
      if (mcp.getPortIntFlags(MCP_PORTB)) {
        // read B3...B7 GPIOs
        for (int i = MCP_B3; i <= MCP_B7; ++i) {
          extgpioLabel[i]->labelStyle().backgroundColor = mcp.readGPIO(i) ? RGB888(255, 0, 0) : RGB888(64, 0, 0);
          extgpioLabel[i]->repaint();
        }
      }
    }
  }

  bool initMCP() {
    return mcp.begin();
  }

  void extGPIOSet(int gpio, bool value) {
    mcp.writeGPIO(gpio, value);
    extgpioLabel[gpio]->labelStyle().backgroundColor = value ? RGB888(0, 255, 0) : RGB888(0, 64, 0);
    extgpioLabel[gpio]->repaint();
  }

  void playExtGPIOS() {
    showWindow(extgpioPlay, false);
    // single left<->right
    for (int j = 0; j < 4; ++j) {
      for (int i = MCP_A0; i <= MCP_B2; ++i) {
        extGPIOSet(i, true);
        if (i > MCP_A0)
          extGPIOSet(i - 1, false);
        processEvents();
        vTaskDelay(pdMS_TO_TICKS(80));
      }
      for (int i = MCP_B2; i >= MCP_A0; --i) {
        extGPIOSet(i, true);
        if (i < MCP_B2)
          extGPIOSet(i + 1, false);
        processEvents();
        vTaskDelay(pdMS_TO_TICKS(80));
      }
    }
    // all together
    for (int j = 0; j < 4; ++j) {
      for (int i = MCP_A0; i <= MCP_B2; ++i) {
        extGPIOSet(i, true);
        processEvents();
        vTaskDelay(pdMS_TO_TICKS(80));
      }
      for (int i = MCP_A0; i <= MCP_B2; ++i) {
        extGPIOSet(i, false);
        processEvents();
        vTaskDelay(pdMS_TO_TICKS(80));
      }
    }
    // flashing
    for (int j = 0; j < 10; ++j) {
      for (int i = MCP_A0; i <= MCP_B2; ++i)
        extGPIOSet(i, true);
      processEvents();
      vTaskDelay(pdMS_TO_TICKS(120));
      for (int i = MCP_A0; i <= MCP_B2; ++i)
        extGPIOSet(i, false);
      processEvents();
      vTaskDelay(pdMS_TO_TICKS(120));
    }
    showWindow(extgpioPlay, true);
  }

  void testSD() {
    // disable MCP (just because SD should be initialized before MCP)
    bool mcpAvailable = mcp.available();
    mcp.end();
    // mount test
    FileBrowser fb;
    fb.unmountSDCard();
    bool r = fb.mountSDCard(false, "/SD", 1);
    if (!r) {
      sdResultLabel->labelStyle().textColor = Color::BrightRed;
      sdResultLabel->setText("Mount Failed!");
      if (mcpAvailable)
        initMCP();
      return;
    }
    // write test
    fb.setDirectory("/SD");
    constexpr int size = 16384;
    bool ok = true;
    auto fname = fb.createTempFilename();
    auto f = fopen(fname, "wb");
    if (f) {
      for (int i = 0; i < size; ++i)
        if (fputc(i & 0xff, f) != (i & 0xff)) {
          ok = false;
          break;
        }
      fclose(f);
    }
    if (!f || !ok) {
      free(fname);
      sdResultLabel->labelStyle().textColor = Color::BrightRed;
      sdResultLabel->setText("Write Failed!");
      return;
    }
    // read test
    f = fopen(fname, "rb");
    if (f) {
      for (int i = 0; i < size; ++i)
        if (fgetc(f) != (i & 0xff)) {
          ok = false;
          break;
        }
      fclose(f);
    }
    free(fname);
    fb.unmountSDCard();
    if (mcpAvailable)
      initMCP();
    if (!f || !ok) {
      sdResultLabel->labelStyle().textColor = Color::BrightRed;
      sdResultLabel->setText("Read Failed!");
      return;
    }

    sdResultLabel->labelStyle().textColor = Color::Green;
    sdResultLabel->setText("Success!");
  }

  void testWifi() {
    fabgl::WiFiScanner scanner;
    if (scanner.scan(0, true)) {
      if (scanner.count() == 0) {
        wifiResultLabel->labelStyle().textColor = Color::Yellow;
        wifiResultLabel->setText("Ok, No Network Found!");
      } else {
        wifiResultLabel->labelStyle().textColor = Color::Green;
        wifiResultLabel->setTextFmt("Ok, Found %d Networks", scanner.count());
      }
    } else {
      wifiResultLabel->labelStyle().textColor = Color::BrightRed;
      wifiResultLabel->setText("Wifi Scan Failed!");
    }
  }

} app;


void setup()
{
  //printf("MALLOC_CAP_32BIT: %d bytes\r\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));
  //printf("MALLOC_CAP_8BIT: %d bytes\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));

  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.queueSize = 350;  // trade UI speed using less RAM and allow both WiFi and SD Card FS
  DisplayController.begin();
  DisplayController.setResolution(VESA_640x480_75Hz, 570, 390);
}




void loop()
{
  app.runAsync(&DisplayController);
  vTaskDelete(NULL);
}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
