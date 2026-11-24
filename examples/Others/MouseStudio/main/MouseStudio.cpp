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


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fabgl.h"


// UART0 (console) stream
fabgl::UARTStream Console;

#include "esp_chip_info.h"


static int chipRevision()
{
  esp_chip_info_t info;
  esp_chip_info(&info);
  return info.revision;
}


fabgl::PS2Controller PS2Controller;



void printInfo();


void printHelp()
{
  printf("\n\nPS/2 Mouse Studio\n");
  printf("Chip Revision: %d   Chip Frequency: %d MHz\n", chipRevision(), (int) fabgl::getCPUFrequencyMHz());

  printInfo();

  printf("Commands:\n");
  printf("  r = Reset  s = Change Sample Rate  e = Change Resolution  c = Change Scaling\n");
  printf("Various:\n");
  printf("  h = Print This help\n\n");
}


void printInfo()
{
  auto mouse = PS2Controller.mouse();

  if (mouse->isMouseAvailable()) {
    printf("%s", "Device Id = ");
    switch (mouse->identify()) {
      case PS2DeviceType::OldATKeyboard:
        printf("%s", "\"Old AT Keyboard\"");
        break;
      case PS2DeviceType::MouseStandard:
        printf("%s", "\"Standard Mouse\"");
        break;
      case PS2DeviceType::MouseWithScrollWheel:
        printf("%s", "\"Mouse with scroll wheel\"");
        break;
      case PS2DeviceType::Mouse5Buttons:
        printf("%s", "\"5 Buttons Mouse\"");
        break;
      case PS2DeviceType::MF2KeyboardWithTranslation:
        printf("%s", "\"MF2 Keyboard with translation\"");
        break;
      case PS2DeviceType::M2Keyboard:
        printf("%s", "\"MF2 keyboard\"");
        break;
      default:
        printf("%s", "\"Unknown\"");
        break;
    }
  } else
    printf("%s", "Mouse Error!");
  printf("%s", "\n");
}



void setup()
{
  Console.begin();

  vTaskDelay(pdMS_TO_TICKS(500));  // avoid garbage into the UART
  printf("%s", "\n\nReset\n");

  PS2Controller.begin();

  printHelp();
}


const uint8_t SampleRates[] = { 10, 20, 40, 60, 80, 100, 200 };
int currentSampleRateIndex = 5; // reset is 100 samples/sec


int currentResolution = 2;      // reset is 2 (4 counts/mm)


int currentScaling = 1;         // reset is 1 (1:1)


void loop()
{
  auto mouse = PS2Controller.mouse();

  if (Console.available() > 0) {
    char c = Console.read();
    switch (c) {
      case 'h':
        printHelp();
        break;
      case 'r':
        mouse->reset();
        printInfo();
        break;
      case 's':
        ++currentSampleRateIndex;
        if (currentSampleRateIndex == sizeof(SampleRates))
          currentSampleRateIndex = 0;
        if (mouse->setSampleRate(SampleRates[currentSampleRateIndex]))
          printf("Sample rate = %d\n", SampleRates[currentSampleRateIndex]);
        break;
      case 'e':
        ++currentResolution;
        if (currentResolution == 4)
          currentResolution = 0;
        if (mouse->setResolution(currentResolution))
          printf("Resolution = %d\n", 1 << currentResolution);
        break;
      case 'c':
        ++currentScaling;
        if (currentScaling == 3)
          currentScaling = 1;
        if (mouse->setScaling(currentScaling))
          printf("Scaling = 1:%d\n", currentScaling);
        break;
    }
  }

  if (mouse->deltaAvailable()) {
    MouseDelta mouseDelta;
    mouse->getNextDelta(&mouseDelta);

    printf("deltaX = %d  deltaY = %d  deltaZ = %d  leftBtn = %d  midBtn = %d  rightBtn = %d\n",
                  mouseDelta.deltaX, mouseDelta.deltaY, mouseDelta.deltaZ,
                  mouseDelta.buttons.left, mouseDelta.buttons.middle, mouseDelta.buttons.right);
  }

}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
