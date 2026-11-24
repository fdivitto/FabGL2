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


#include "esp_random.h"
#include "fabgl.h"


// random integer in the range [0, max)
static inline int randomInt(int max) { return max > 0 ? (int) (esp_random() % (uint32_t) max) : 0; }
// random integer in the range [min, max)
static inline int randomInt(int min, int max) { return max > min ? min + (int) (esp_random() % (uint32_t) (max - min)) : min; }

#include "fabui.h"

#include "App.h"



fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;



void setup()
{
  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);
}


void loop()
{
  MyApp().runAsync(&DisplayController, 3500).joinAsyncRun();  // why this? Just to use a larger stack!
}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
