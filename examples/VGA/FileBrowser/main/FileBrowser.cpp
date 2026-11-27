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
 * Optional SD Card connections:
 *   MISO => GPIO 16  (2 for PICO-D4)
 *   MOSI => GPIO 17  (12 for PICO-D4)
 *   CLK  => GPIO 14
 *   CS   => GPIO 13
 *
 * To change above assignment fill other paramaters of FileBrowser::mountSDCard().
 */



#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#include "fabgl.h"
#include "network/netutils.h"
#include "fabui.h"



#define FORMAT_ON_FAIL     true

#define SPIFFS_MOUNT_PATH  "/flash"
#define SDCARD_MOUNT_PATH  "/SD"


Preferences preferences;
fabgl::WiFiConnection wifi;

fabgl::VGA16Controller DisplayController;   // 16 colors: half the memory of VGAController, palette for the theme
fabgl::PS2Controller   PS2Controller;

// dark theme of the library: palette, windows and controls (uitheme.h)
fabgl::uiDarkTheme     theme;


class MyApp : public uiApp {

  uiLabel *       WiFiStatusLbl;
  uiFileBrowser * fileBrowser;
  uiLabel *       freeSpaceLbl;

  void init() {
    // the theme styles every window and control, message boxes and input boxes included
    theme.apply(this);

    auto frame = new uiFrame(rootWindow(), "FileBrowser Example", Point(4, 12), Size(392, 276));
    frame->frameProps().hasCloseButton = false;

    int cx = frame->clientPos().X;
    int cy = frame->clientPos().Y;

    // buttons column on the right of the browser
    constexpr int BtnW = 142, BtnH = 24, BtnStep = 28;
    int bx = cx + 236;
    int by = cy + 8;

    // file browser
    fileBrowser = new uiFileBrowser(frame, Point(cx + 8, cy + 8), Size(220, 7 * BtnStep - 4));
    fileBrowser->setDirectory("/");
    fileBrowser->onChange = [&]() {
      updateFreeSpaceLabel();
    };

    // create directory button
    auto createDirBtn = new uiButton(frame, "Create Dir", Point(bx, by + 0 * BtnStep), Size(BtnW, BtnH));
    createDirBtn->onClick = [&]() {
      constexpr int MAXSTRLEN = 16;
      char dirname[MAXSTRLEN + 1] = "";
      if (inputBox("Create Directory", "Name", dirname, MAXSTRLEN, "Create", "Cancel") == uiMessageBoxResult::Button1) {
        fileBrowser->content().makeDirectory(dirname);
        updateBrowser();
      }
    };

    // create empty file button
    auto createEmptyFile = new uiButton(frame, "Create Empty File", Point(bx, by + 1 * BtnStep), Size(BtnW, BtnH));
    createEmptyFile->onClick = [&]() {
      constexpr int MAXSTRLEN = 16;
      char filename[MAXSTRLEN + 1] = "";
      if (inputBox("Create Empty File", "Name", filename, MAXSTRLEN, "Create", "Cancel") == uiMessageBoxResult::Button1) {
        int len = fileBrowser->content().getFullPath(filename);
        char fullpath[len];
        fileBrowser->content().getFullPath(filename, fullpath, len);
        FILE * f = fopen(fullpath, "wb");
        fclose(f);
        updateBrowser();
      }
    };

    // rename button
    auto renameBtn = new uiButton(frame, "Rename", Point(bx, by + 2 * BtnStep), Size(BtnW, BtnH));
    renameBtn->onClick = [&]() {
      int maxlen = fabgl::imax(16, (int) strlen(fileBrowser->filename()));
      char filename[maxlen + 1];
      strcpy(filename, fileBrowser->filename());
      if (inputBox("Rename File", "New name", filename, maxlen, "Rename", "Cancel") == uiMessageBoxResult::Button1) {
        fileBrowser->content().rename(fileBrowser->filename(), filename);
        updateBrowser();
      }
    };

    // delete button
    auto deleteBtn = new uiButton(frame, "Delete", Point(bx, by + 3 * BtnStep), Size(BtnW, BtnH));
    deleteBtn->onClick = [&]() {
      if (messageBox("Delete file/directory", "Are you sure?", "Yes", "Cancel") == uiMessageBoxResult::Button1) {
        fileBrowser->content().remove( fileBrowser->filename() );
        updateBrowser();
      }
    };

    // format button
    auto formatBtn = new uiButton(frame, "Format", Point(bx, by + 4 * BtnStep), Size(BtnW, BtnH));
    formatBtn->onClick = [&]() {
      // inside a drive format that drive, in the root directory format the selected drive
      auto driveType = fileBrowser->content().getCurrentDriveType();
      if (driveType == fabgl::DriveType::None && fileBrowser->filename()) {
        char path[strlen(fileBrowser->filename()) + 2];
        sprintf(path, "/%s", fileBrowser->filename());
        driveType = FileBrowser::getDriveType(path);
      }
      if (driveType == fabgl::DriveType::None) {
        messageBox("Format", "Select a drive to format", "OK", nullptr, nullptr, uiMessageBoxIcon::Warning);
        return;
      }
      char const * driveName = driveType == fabgl::DriveType::SDCard ? "SD Card" : "SPIFFS";
      char title[24];
      sprintf(title, "Format %s", driveName);
      if (messageBox(title, "All data will be lost. Are you sure?", "Yes", "Cancel") == uiMessageBoxResult::Button1) {
        // show "Formatting..." while formatting (format() blocks the UI)
        auto waitFrame = new uiFrame(rootWindow(), title, UIWINDOW_PARENTCENTER, Size(240, 70), false);
        waitFrame->frameProps().resizeable        = false;
        waitFrame->frameProps().moveable          = false;
        waitFrame->frameProps().hasCloseButton    = false;
        waitFrame->frameProps().hasMaximizeButton = false;
        waitFrame->frameProps().hasMinimizeButton = false;
        new uiLabel(waitFrame, "Formatting, please wait...", Point(waitFrame->clientPos().X + 16, waitFrame->clientPos().Y + 14));
        auto modalState = initModalWindow(waitFrame);
        processModalWindowEvents(modalState, 50);  // processes pending events (paints the dialog)
        canvas()->waitCompletion();
        bool formatted = FileBrowser::format(driveType, 0);
        endModalWindow(modalState);
        destroyWindow(waitFrame);
        if (!formatted)
          messageBox(title, "Format failed", "OK", nullptr, nullptr, uiMessageBoxIcon::Error);
        updateBrowser();
      }
    };

    // setup wifi button
    auto setupWifiBtn = new uiButton(frame, "Setup WiFi", Point(bx, by + 5 * BtnStep), Size(BtnW, BtnH));
    setupWifiBtn->onClick = [=, this]() {
      char SSID[32] = "";
      char psw[32]  = "";
      if (inputBox("WiFi Connect", "Network Name", SSID, sizeof(SSID), "OK", "Cancel") == uiMessageBoxResult::Button1 &&
          inputBox("WiFi Connect", "Password", psw, sizeof(psw), "OK", "Cancel") == uiMessageBoxResult::Button1) {
        preferences.putString("SSID", SSID);
        preferences.putString("WiFiPsw", psw);
        connectWiFi();
      }
    };

    // download file button
    auto downloadBtn = new uiButton(frame, "Download", Point(bx, by + 6 * BtnStep), Size(BtnW, BtnH));
    downloadBtn->onClick = [=, this]() {
      char URL[128] = "https://";
      if (inputBox("Download File", "URL", URL, sizeof(URL), "OK", "Cancel") == uiMessageBoxResult::Button1) {
        download(URL);
        updateFreeSpaceLabel();
      }
    };

    // free space label
    freeSpaceLbl = new uiLabel(frame, "", Point(cx + 8, cy + 212));
    updateFreeSpaceLabel();

    // wifi status label
    WiFiStatusLbl = new uiLabel(frame, "", Point(cx + 8, cy + 232));
    connectWiFi();

    setFocusedWindow(fileBrowser);
  }

  void updateBrowser()
  {
    fileBrowser->update();
    updateFreeSpaceLabel();
  }

  // connect to wifi using SSID and PSW from Preferences
  void connectWiFi()
  {
    WiFiStatusLbl->setText("WiFi Not Connected");
    WiFiStatusLbl->labelStyle().textColor = theme.ink(fabgl::uiThemeInk::Red);
    char SSID[32], psw[32];
    if (preferences.getString("SSID", SSID, sizeof(SSID)) && preferences.getString("WiFiPsw", psw, sizeof(psw))) {
      wifi.disconnect();
      if (wifi.connect(SSID, psw, 16000) == fabgl::WiFiConnectionState::Connected) {
        WiFiStatusLbl->setTextFmt("Connected to %s, IP is %s", wifi.SSID(), wifi.IPString());
        WiFiStatusLbl->labelStyle().textColor = theme.ink(fabgl::uiThemeInk::Green);
      }
    }
    WiFiStatusLbl->update();
  }

  // download from URL. Filename is the last part of the URL
  void download(char const * URL) {
    char const * slashPos = strrchr(URL, '/');
    if (slashPos) {
      char filename[slashPos - URL + 1];
      strcpy(filename, slashPos + 1);

      int fullpathLen = fileBrowser->content().getFullPath(filename);
      char fullpath[fullpathLen];
      fileBrowser->content().getFullPath(filename, fullpath, fullpathLen);
      FILE * f = fopen(fullpath, "wb");
      if (f) {
        fabgl::HTTPRequest http;
        http.onData = [&](void const * data, int len) {
          fwrite(data, 1, len, f);
        };
        int httpCode = http.GET(URL);
        fclose(f);
        if (httpCode != 200) {
          remove(fullpath);
          char msg[64];
          if (httpCode == 0)
            snprintf(msg, sizeof(msg), "Connection failed");
          else
            snprintf(msg, sizeof(msg), "Server replied with HTTP code %d", httpCode);
          messageBox("Download failed", msg, "OK", nullptr, nullptr, uiMessageBoxIcon::Error);
        }
        updateBrowser();
      } else
        messageBox("Download failed", "Cannot create the file", "OK", nullptr, nullptr, uiMessageBoxIcon::Error);
    }
  }

  // show used and free SD space
  void updateFreeSpaceLabel() {
    int64_t total, used;
    FileBrowser::getFSInfo(fileBrowser->content().getCurrentDriveType(), 0, &total, &used);
    freeSpaceLbl->setTextFmt("%lld KiB used, %lld KiB free", used / 1024, (total - used) / 1024);
    freeSpaceLbl->update();
  }

} app;


void setup()
{

  preferences.begin("FileBrowser", false);

  PS2Controller.begin(PS2Preset::KeyboardPort0_MousePort1, KbdMode::GenerateVirtualKeys);

  DisplayController.begin();

  // keep LOW!!! otherwise there isn't enough memory for WiFi!!!
  DisplayController.setResolution(VGA_400x300_60Hz);
  theme.setupPalette(&DisplayController);

  // adjust this to center screen in your monitor
  //DisplayController.moveScreen(-6, 0);

  Canvas cv(&DisplayController);
  cv.clear();
  cv.drawText(50, 170, "Initializing...");
  cv.waitCompletion();

  FileBrowser::mountSDCard(FORMAT_ON_FAIL, SDCARD_MOUNT_PATH);
  FileBrowser::mountSPIFFS(FORMAT_ON_FAIL, SPIFFS_MOUNT_PATH);
}


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
