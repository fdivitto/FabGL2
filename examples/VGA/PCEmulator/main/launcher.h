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


#pragma once


/*
  Launcher: the start screen of the emulator.

  On the left the machine configurations (with the state of their disks), on the right the details of the selected
  one (boot drive, disk images, where they are), below the commands. The selected configuration starts by itself
  after a few seconds unless a key is pressed or the mouse is moved.

  The launcher only chooses: it quits with the selected action (LauncherAction) and setup() executes it.
*/


#include <sys/stat.h>

#include "mconf.h"


enum class LauncherAction {
  Run,
  Edit,
  New,
  Remove,
  Files,
  WiFi,
};


// where a disk image is
enum class DiskState {
  None,       // drive not used
  Ready,      // on the SD card
  Download,   // to download
  Resume,     // partially downloaded (".part")
  Missing,    // local file not found
};


struct DiskInfo {
  DiskState    state;
  int64_t      size;       // bytes on the SD card (Ready, Resume)
  char const * name;       // filename (last part of the URL) or path
  bool         remote;
};


static bool isURL(char const * path)
{
  return strncmp("http://", path, 7) == 0 || strncmp("https://", path, 8) == 0;
}


// size of a file relative to the SD card mount path, -1 if it doesn't exist (stat() reads the directory entry only,
// a seek to the end of a big image walks its cluster chain)
static int64_t sdFileSize(char const * path)
{
  char fullpath[strlen(SD_MOUNT_PATH) + 1 + strlen(path) + 6];
  sprintf(fullpath, "%s/%s", SD_MOUNT_PATH, path);
  struct stat st;
  return stat(fullpath, &st) == 0 ? st.st_size : -1;
}


static DiskInfo getDiskInfo(char const * disk)
{
  DiskInfo info = { DiskState::None, -1, nullptr, false };
  if (!disk || !disk[0])
    return info;
  info.remote = isURL(disk);
  info.name   = info.remote ? strrchr(disk, '/') + 1 : disk;
  info.size   = sdFileSize(info.name);
  if (info.size >= 0) {
    info.state = DiskState::Ready;
  } else if (info.remote) {
    char partname[strlen(info.name) + 6];
    sprintf(partname, "%s.part", info.name);
    info.size  = sdFileSize(partname);
    info.state = info.size >= 0 ? DiskState::Resume : DiskState::Download;
  } else
    info.state = DiskState::Missing;
  return info;
}


// a configuration with the state of its disks, read once when the launcher opens: painting doesn't access the
// SD card (an access between the erase and the redraw of an area makes it flicker)
struct LauncherItem {
  MachineConfItem * conf;
  DiskInfo          disks[DISKCOUNT];
  DiskState         state;            // the "worst" state of the disks

  void load(MachineConfItem * conf_)
  {
    conf  = conf_;
    state = DiskState::None;
    for (int i = 0; i < DISKCOUNT; ++i) {
      disks[i] = getDiskInfo(conf->disk[i]);
      if ((int) disks[i].state > (int) state)
        state = disks[i].state;
    }
  }
};


static char const * DriveName[DISKCOUNT] = { "Floppy A", "Floppy B", "Hard Disk C", "Hard Disk D" };
static char const * DriveLetter[DISKCOUNT] = { "A:", "B:", "C:", "D:" };


static void formatSize(char * buf, size_t len, int64_t size)
{
  if (size >= 1048576)
    snprintf(buf, len, "%.1f MB", size / 1048576.0);
  else
    snprintf(buf, len, "%d KB", (int) (size / 1024));
}


struct LauncherApp : public uiApp {

  using Ink   = fabgl::uiThemeInk;
  using Style = fabgl::uiThemeStyle;

  static constexpr int ItemHeight = 36;

  MachineConf *    mconf;
  int              selected;          // in: initial selection, out: selected configuration
  int              autoRunSeconds;    // 0 = don't start by itself
  LauncherAction   action;

  static constexpr int MaxItems = 64;

  int              itemsCount;
  LauncherItem *   items;             // MaxItems, on the heap (the launcher runs on the 8 KB main task stack)

  uiListBox *      list;
  uiPaintBox *     details;
  uiLabel *        hintLabel;
  uiLabel *        wifiLabel;
  uiTimerHandle    timer;


  LauncherApp(MachineConf * mconf_, int selected_, int autoRunSeconds_)
    : mconf(mconf_),
      selected(selected_),
      autoRunSeconds(autoRunSeconds_),
      action(LauncherAction::Run),
      itemsCount(0),
      items(new LauncherItem[MaxItems]),
      timer(nullptr)
  {
  }

  ~LauncherApp()
  {
    delete [] items;
  }


  void init()
  {
    ESP_LOGI("launcher", "free heap %d bytes, largest block %d bytes", (int) heap_caps_get_free_size(MALLOC_CAP_8BIT), (int) heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    theme.apply(this);
    rootWindow()->onPaint = [&]() { drawInfo(canvas()); };

    const int W = rootWindow()->clientSize().width;
    const int H = rootWindow()->clientSize().height;

    // machine configurations
    for (auto conf = mconf->getFirstItem(); conf && itemsCount < MaxItems; conf = conf->next)
      items[itemsCount++].load(conf);
    selected = itemsCount > 0 ? fabgl::iclamp(selected, 0, itemsCount - 1) : -1;

    constexpr int Top = 58;
    const int listW   = 270;
    const int listH   = H - Top - 64;

    new uiStaticLabel(rootWindow(), "Machines", Point(12, Top - 2), true, Style::LabelDim);
    list = new uiListBox(rootWindow(), Point(12, Top + 14), Size(listW, listH - 14));
    list->listBoxStyle().itemHeight = ItemHeight;
    for (int i = 0; i < itemsCount; ++i)
      list->items().append(items[i].conf->desc);
    if (selected >= 0)
      list->selectItem(selected);
    list->onPaintItem = [&](int index, Rect const & r, bool isSelected) { paintItem(index, r, isSelected); };
    list->onChange    = [&]() {
      stopCountdown();
      int sel = list->firstSelectedItem();
      if (sel != selected) {
        selected = sel;
        details->repaint();
      }
    };
    list->onDblClick  = [&]() { quitWith(LauncherAction::Run); };
    list->onKeyUp     = [&](uiKeyEventInfo const & key) { handleKey(key); };

    // details of the selected configuration
    new uiStaticLabel(rootWindow(), "Details", Point(listW + 22, Top - 2), true, Style::LabelDim);
    auto card = new uiPanel(rootWindow(), Point(listW + 22, Top + 14), Size(W - listW - 34, listH - 14), true, Style::Card);
    details = new uiPaintBox(card, Point(1, 1), Size(card->size().width - 2, card->size().height - 2));
    details->paintBoxStyle().backgroundColor = card->panelStyle().backgroundColor;
    details->onPaint = [&](Rect const & r) { paintDetails(); };

    // commands
    struct Command { char const * text; int width; LauncherAction action; };
    static const Command Commands[] = {
      { "Run",    84, LauncherAction::Run },
      { "Edit",   64, LauncherAction::Edit },
      { "New",    64, LauncherAction::New },
      { "Remove", 72, LauncherAction::Remove },
      { "Files",  64, LauncherAction::Files },
      { "WiFi",   64, LauncherAction::WiFi },
    };
    int x = 12;
    const int y = Top + listH + 8;
    for (auto const & cmd : Commands) {
      auto button = new uiButton(rootWindow(), cmd.text, Point(x, y), Size(cmd.width, 28), uiButtonKind::Button, true,
                                 cmd.action == LauncherAction::Run ? Style::ButtonAccent : Style::Button);
      auto a = cmd.action;
      button->onClick = [this, a]() { quitWith(a); };
      x += cmd.width + 6;
    }

    // status line: hints or countdown on the left, WiFi on the right
    hintLabel = new uiLabel(rootWindow(), "", Point(12, H - 18), Size(W - 190, 16), true, Style::LabelDim);
    wifiLabel = new uiLabel(rootWindow(), "", Point(W - 172, H - 18), Size(160, 16), true, Style::LabelDim);
    wifiLabel->labelStyle().textAlign = fabgl::uiHAlign::Right;
    char SSID[32];
    if (preferences.getString("SSID", SSID, sizeof(SSID)))
      wifiLabel->setTextFmt("WiFi: %s", SSID);
    else
      wifiLabel->setText("WiFi: not configured");
    showHints();

    // start by itself?
    if (autoRunSeconds > 0 && selected >= 0) {
      timer = setTimer(rootWindow(), 1000);
      rootWindow()->onTimer = [&](uiTimerHandle t) { countdownTick(); };
      showCountdown();
    }

    setActiveWindow(rootWindow());
    setFocusedWindow(list);
  }


  void quitWith(LauncherAction value)
  {
    if (itemsCount == 0 && (value == LauncherAction::Run || value == LauncherAction::Edit || value == LauncherAction::Remove)) {
      messageBox("Machines", "There are no machine configurations: create one with New", "OK", nullptr, nullptr, uiMessageBoxIcon::Info);
      return;
    }
    stopCountdown();
    action = value;
    rootWindow()->frameProps().fillBackground = false;
    quit(0);
  }


  void handleKey(uiKeyEventInfo const & key)
  {
    switch (key.VK) {
      case VirtualKey::VK_RETURN:
      case VirtualKey::VK_KP_ENTER:
        quitWith(LauncherAction::Run);
        break;
      case VirtualKey::VK_F2:
        quitWith(LauncherAction::Edit);
        break;
      case VirtualKey::VK_INSERT:
      case VirtualKey::VK_KP_INSERT:
        quitWith(LauncherAction::New);
        break;
      case VirtualKey::VK_DELETE:
      case VirtualKey::VK_KP_DELETE:
        quitWith(LauncherAction::Remove);
        break;
      case VirtualKey::VK_F3:
        quitWith(LauncherAction::Files);
        break;
      case VirtualKey::VK_F4:
        quitWith(LauncherAction::WiFi);
        break;
      default:
        stopCountdown();
        break;
    }
  }


  ////////////////////////////////////////////////////////
  // countdown

  void countdownTick()
  {
    // the user did something: stop
    int now = (int) (esp_timer_get_time() / 1000);
    if (lastUserActionTime() + 900 > now) {
      stopCountdown();
      return;
    }
    if (--autoRunSeconds <= 0)
      quitWith(LauncherAction::Run);
    else
      showCountdown();
  }

  void showCountdown()
  {
    hintLabel->labelStyle().textColor = theme.ink(Ink::Accent);
    hintLabel->setTextFmt("Starting in %d s - press a key to stay here", autoRunSeconds);
  }

  void stopCountdown()
  {
    if (timer) {
      killTimer(timer);
      timer = nullptr;
      showHints();
    }
  }

  void showHints()
  {
    hintLabel->labelStyle().textColor = theme.ink(Ink::TextDim);
    hintLabel->setText("Enter run  F2 edit  Ins new  Del remove  F3 files  F4 WiFi");
  }


  ////////////////////////////////////////////////////////
  // painting

  static RGB888 stateColor(DiskState state)
  {
    switch (state) {
      case DiskState::Ready:    return theme.ink(Ink::Green);
      case DiskState::Download: return theme.ink(Ink::Accent);
      case DiskState::Resume:   return theme.ink(Ink::Amber);
      case DiskState::Missing:  return theme.ink(Ink::Red);
      default:                  return theme.ink(Ink::TextDim);
    }
  }

  static char const * stateText(DiskState state)
  {
    switch (state) {
      case DiskState::Ready:    return "Ready";
      case DiskState::Download: return "Download needed";
      case DiskState::Resume:   return "Download incomplete";
      case DiskState::Missing:  return "Disk image missing";
      default:                  return "No disks";
    }
  }

  // small icon of the boot drive: floppy (square with shutter) or hard disk (box with a led)
  void paintDriveIcon(Canvas * cv, int x, int y, bool floppy, RGB888 color)
  {
    cv->setPenColor(color);
    if (floppy) {
      cv->drawRectangle(x, y, x + 15, y + 15);
      cv->drawRectangle(x + 4, y, x + 11, y + 5);        // shutter
      cv->drawRectangle(x + 3, y + 9, x + 12, y + 15);   // label
    } else {
      cv->drawRectangle(x, y + 3, x + 17, y + 12);
      cv->drawLine(x + 2, y + 9, x + 9, y + 9);
      cv->setBrushColor(color);
      cv->fillRectangle(x + 13, y + 8, x + 15, y + 9);   // led
    }
  }

  void paintItem(int index, Rect const & r, bool isSelected)
  {
    auto cv    = list->canvas();
    auto conf  = items[index].conf;
    bool light = isSelected && list->hasFocus();   // on the accent background everything is white
    RGB888 textColor = light ? theme.ink(Ink::White) : theme.ink(Ink::Text);
    RGB888 dimColor  = light ? theme.ink(Ink::White) : theme.ink(Ink::TextDim);

    cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false).Italic(false).Underline(false).Invert(0));

    paintDriveIcon(cv, r.X1 + 8, r.Y1 + 9, conf->bootDrive < 2, dimColor);

    cv->setPenColor(textColor);
    cv->drawTextWithEllipsis(fabgl::uiDarkTheme::normalFont(), r.X1 + 34, r.Y1 + 3, conf->desc, r.X2 - 4);

    auto state = items[index].state;
    cv->setPenColor(light ? theme.ink(Ink::White) : stateColor(state));
    cv->setBrushColor(light ? theme.ink(Ink::White) : stateColor(state));
    cv->fillRectangle(r.X1 + 34, r.Y1 + 23, r.X1 + 38, r.Y1 + 27);
    cv->drawText(fabgl::uiDarkTheme::smallFont(), r.X1 + 44, r.Y1 + 19, stateText(state));
  }

  // draws text in up to maxLines lines, breaking at the spaces. Returns the height used
  int drawWrapped(Canvas * cv, fabgl::FontInfo const * font, int x, int y, int maxX, char const * text, int maxLines)
  {
    int lines = 0;
    while (*text && lines < maxLines) {
      // longest part that fits
      int len = strlen(text), fit = 0;
      for (int i = 1; i <= len; ++i) {
        if (i < len && text[i] != ' ')
          continue;
        char part[i + 1];
        memcpy(part, text, i);
        part[i] = 0;
        if (x + cv->textExtent(font, part) > maxX)
          break;
        fit = i;
      }
      if (fit == 0 || lines == maxLines - 1) {
        cv->drawTextWithEllipsis(font, x, y, text, maxX);
        ++lines;
        break;
      }
      char part[fit + 1];
      memcpy(part, text, fit);
      part[fit] = 0;
      cv->drawText(font, x, y, part);
      text += fit;
      while (*text == ' ')
        ++text;
      y += font->height + 2;
      ++lines;
    }
    return lines * (font->height + 2);
  }

  void paintDetails()
  {
    auto cv = details->canvas();
    cv->setGlyphOptions(GlyphOptions().FillBackground(false).Bold(false).Italic(false).Underline(false).Invert(0));
    const int maxX = details->clientSize().width - 10;
    const auto smallFont  = fabgl::uiDarkTheme::smallFont();
    const auto normalFont = fabgl::uiDarkTheme::normalFont();

    if (selected < 0) {
      cv->setPenColor(theme.ink(Ink::TextDim));
      cv->drawText(normalFont, 10, 10, "No machine selected");
      return;
    }
    auto conf = items[selected].conf;

    int y = 8;
    cv->setPenColor(theme.ink(Ink::White));
    y += drawWrapped(cv, normalFont, 10, y, maxX, conf->desc, 2) + 6;

    cv->setPenColor(theme.ink(Ink::TextDim));
    cv->drawText(smallFont, 10, y, "Boot from");
    cv->setPenColor(theme.ink(Ink::Accent));
    cv->drawText(smallFont, 70, y, DriveName[conf->bootDrive]);
    y += smallFont->height + 8;

    cv->setPenColor(theme.ink(Ink::Border));
    cv->drawLine(10, y, maxX, y);
    y += 8;

    for (int i = 0; i < DISKCOUNT; ++i) {
      auto const & info = items[selected].disks[i];
      if (info.state == DiskState::None)
        continue;

      // drive letter badge
      cv->setBrushColor(i == conf->bootDrive ? theme.ink(Ink::AccentDark) : theme.ink(Ink::Hover));
      cv->fillRectangle(10, y, 31, y + 17);
      cv->setPenColor(theme.ink(Ink::White));
      cv->drawText(normalFont, 13, y + 1, DriveLetter[i]);

      // image name
      cv->setPenColor(theme.ink(Ink::Text));
      cv->drawTextWithEllipsis(normalFont, 38, y + 1, info.name, maxX);
      y += normalFont->height + 4;

      // where it is
      char size[16] = "";
      if (info.size >= 0)
        formatSize(size, sizeof(size), info.size);
      char status[64];
      switch (info.state) {
        case DiskState::Ready:
          snprintf(status, sizeof(status), "On SD card, %s", size);
          break;
        case DiskState::Download:
          snprintf(status, sizeof(status), "Downloaded at start");
          break;
        case DiskState::Resume:
          snprintf(status, sizeof(status), "Partial (%s), resumed at start", size);
          break;
        default:
          snprintf(status, sizeof(status), "Not found on SD card");
          break;
      }
      cv->setPenColor(stateColor(info.state));
      cv->drawTextWithEllipsis(smallFont, 38, y, status, maxX);
      y += smallFont->height + 2;

      // hard disk geometry
      if (i >= 2 && conf->cylinders[i] && conf->heads[i] && conf->sectors[i]) {
        char chs[40];
        snprintf(chs, sizeof(chs), "CHS %d / %d / %d", conf->cylinders[i], conf->heads[i], conf->sectors[i]);
        cv->setPenColor(theme.ink(Ink::TextDim));
        cv->drawText(smallFont, 38, y, chs);
        y += smallFont->height + 2;
      }
      y += 8;
    }
  }

};
