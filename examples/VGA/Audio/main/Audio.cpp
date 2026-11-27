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



#include "esp_system.h"
#include "fabgl.h"
#include "fabui.h"

#include "speech.h"
#include "loop.h"
#include "mario.h"


fabgl::VGA16Controller DisplayController;
fabgl::PS2Controller   PS2Controller;
SoundGenerator         soundGenerator;

// dark theme of the library: palette, windows and controls (uitheme.h)
fabgl::uiDarkTheme     theme;


void setup()
{

  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();
  DisplayController.setResolution(VGA_640x480_60Hz);
  theme.setupPalette(&DisplayController);

  // adjust this to center screen in your monitor
  //DisplayController.moveScreen(20, -2);
  //DisplayController.moveScreen(-6, 0);
}


class ChannelFrame : public uiFrame {

  SineWaveformGenerator sine;
  SquareWaveformGenerator square;
  NoiseWaveformGenerator noise;
  TriangleWaveformGenerator triangle;
  SawtoothWaveformGenerator sawtooth;
  SamplesGenerator loop = SamplesGenerator(loopSamples, sizeof(loopSamples));
  SamplesGenerator speech = SamplesGenerator(speechSamples, sizeof(speechSamples));
  SamplesGenerator mario = SamplesGenerator(marioSamples, sizeof(marioSamples));
  WaveformGenerator * curGen = nullptr;

  uiLabel * volumeLabel, * frequencyLabel;
  uiSlider * volumeSlider, * frequencySlider;
  uiComboBox * genComboBox;
  uiCheckBox * enableCheck;

public:

  ChannelFrame(uiWindow * parent)
   : uiFrame(parent, "", Point(0, 0), Size(408, 98))
  {
    static int channel_ = 0;
    static int y = 4, x = 4;
    static int startingFreq = 150;

    app()->moveWindow(this, x, y);
    y += size().height + 4;
    if (y + size().height > parent->size().height - 4) {
      x += 25;
      y = x;
    }

    frameProps().resizeable = false;
    frameProps().hasCloseButton = false;

    setTitleFmt("Channel %d", ++channel_);

    int cx = clientPos().X;
    int cy = clientPos().Y;

    // volume slider
    volumeLabel = new uiLabel(this, "", Point(cx + 4, cy + 6));
    volumeSlider = new uiSlider(this, Point(cx + 86, cy + 5), Size(140, 18), uiOrientation::Horizontal);
    volumeSlider->onChange = [&]() {
      volumeLabel->setTextFmt("Volume %d", volumeSlider->position());
      if (curGen)
        curGen->setVolume(volumeSlider->position());
    };
    volumeSlider->setup(0, 127, 16);
    volumeSlider->setPosition(100);

    // generator combobox
    new uiStaticLabel(this, "Type", Point(cx + 236, cy + 6));
    genComboBox = new uiComboBox(this, Point(cx + 272, cy + 4), Size(126, 20), 104);
    genComboBox->items().append("Sine Wave");
    genComboBox->items().append("Square Wave");
    genComboBox->items().append("Triangle Wave");
    genComboBox->items().append("Sawtooth Wave");
    genComboBox->items().append("Noise");
    genComboBox->items().append("Loop");
    genComboBox->items().append("Speech");
    genComboBox->items().append("Mario");
    genComboBox->selectItem(0);
    genComboBox->onChange = [&]() {
      setGenerator(genComboBox->selectedItem());
    };

    // frequency slider
    frequencyLabel = new uiLabel(this, "", Point(cx + 4, cy + 30));
    frequencySlider = new uiSlider(this, Point(cx + 86, cy + 29), Size(312, 18), uiOrientation::Horizontal);
    frequencySlider->onChange = [&]() {
      frequencyLabel->setTextFmt("Freq %d Hz", frequencySlider->position());
      if (curGen)
        curGen->setFrequency(frequencySlider->position());
    };
    frequencySlider->setup(0, 8000, 500);
    frequencySlider->setPosition(startingFreq);
    startingFreq += 50;

    // enable checkbox
    enableCheck = new uiCheckBox(this, Point(cx + 4, cy + 54), Size(16, 16));
    new uiStaticLabel(this, "Enable", Point(cx + 26, cy + 54));
    enableCheck->onChange = [&]() {
      if (curGen)
        curGen->enable(enableCheck->checked());
    };

    // set default generator
    setGenerator(0);
  }

  void setGenerator(int index) {
    soundGenerator.detach(curGen);
    switch (index) {
      case 0:
        curGen = &sine;
        break;
      case 1:
        curGen = &square;
        break;
      case 2:
        curGen = &triangle;
        break;
      case 3:
        curGen = &sawtooth;
        break;
      case 4:
        curGen = &noise;
        break;
      case 5:
        curGen = &loop;
        break;
      case 6:
        curGen = &speech;
        break;
      case 7:
        curGen = &mario;
        break;
    }
    soundGenerator.attach(curGen);
    curGen->enable(enableCheck->checked());
    curGen->setFrequency(frequencySlider->position());
  }
};


class CommandsFrame : public uiFrame {

public:
  CommandsFrame(uiWindow * parent)
   : uiFrame(parent, "Main Controls", Point(496, 4), Size(140, 212))
  {
    frameProps().resizeable = false;
    frameProps().hasMaximizeButton = false;
    frameProps().hasMinimizeButton = false;
    frameProps().hasCloseButton = false;

    int cx = clientPos().X;
    int cy = clientPos().Y;

    auto addChannelBtn = new uiButton(this, "Add Channel", Point(cx + 6, cy + 6), Size(90, 24), uiButtonKind::Button, true, fabgl::uiThemeStyle::ButtonAccent);
    addChannelBtn->onClick = [=, this]() {
      if (heap_caps_get_free_size(MALLOC_CAP_8BIT) < 5000)
        app()->messageBox("Out of memory!", "I'm sorry, there is no more memory available!", "OK");
      else
        new ChannelFrame(parent);
    };

    auto resetBtn = new uiButton(this, "Reset", Point(cx + 6, cy + 38), Size(90, 24));
    resetBtn->onClick = [=, this]() {
      if (app()->messageBox("Reset ESP32", "Are you sure?", "Yes", "No", nullptr, uiMessageBoxIcon::Question) == uiMessageBoxResult::Button1)
        esp_restart();
    };

    // global enable checkbox
    auto enableCheck = new uiCheckBox(this, Point(cx + 6, cy + 72), Size(16, 16));
    new uiStaticLabel(this, "Enable", Point(cx + 28, cy + 72));
    enableCheck->setChecked(true);
    enableCheck->onChange = [=]() {
      soundGenerator.play(enableCheck->checked());
    };

    // main volume slider
    auto volumeLabel = new uiLabel(this, "", Point(cx + 78, cy + 166));
    auto volumeSlider = new uiSlider(this, Point(cx + 108, cy + 6), Size(18, 154), uiOrientation::Vertical);
    volumeSlider->onChange = [=]() {
      volumeLabel->setTextFmt("Vol %d", volumeSlider->position());
      soundGenerator.setVolume(volumeSlider->position());
    };
    volumeSlider->setup(0, 127, 16);
    volumeSlider->setPosition(100);
  }

};


class MyApp : public uiApp {

  void init() {

    // the theme styles every window and control, message boxes and desktop included
    theme.apply(this);

    new ChannelFrame(rootWindow());
    new ChannelFrame(rootWindow());
    new ChannelFrame(rootWindow());
    new ChannelFrame(rootWindow());

    new CommandsFrame(rootWindow());

    soundGenerator.play(true);

  }
} app;


void loop()
{
  app.run(&DisplayController);
}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
