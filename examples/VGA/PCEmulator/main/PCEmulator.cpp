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


 /* Instructions:

    - to run this application you need an ESP32 with PSRAM installed and an SD-CARD slot (ie TTGO VGA32 v1.4 or FabGL Development Board with WROVER)
    - PSRAM is mapped by ESP-IDF (CONFIG_SPIRAM_USE_MEMMAP=y, see sdkconfig.defaults) and accessed directly by the emulator
    - the partition table is provided (partitions.csv, large app partition)
    - build and flash with: idf.py build flash monitor

 */


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include <memory>
#include <unistd.h>

extern "C" {
#include "esp_psram.h"
}
#include "esp_sntp.h"
#include "esp_wifi.h"


#include "fabgl.h"
#include "network/netutils.h"

#include "mconf.h"
#include "machine.h"



// UART Pins for USB serial
#define UART_URX 3
#define UART_UTX 1



using std::unique_ptr;

using fabgl::StringList;
using fabgl::imin;
using fabgl::imax;



Preferences   preferences;
InputBox      ibox;
Machine     * machine;

// theme of the user interface (launcher, dialogs, configuration editor)
fabgl::uiDarkTheme theme(fabgl::uiThemeAccent::Orange);


// user interface resolution. 16 colors need 96 KB of RAM for the frame buffer. The menu shown while the emulator
// runs (SYSREQ) uses 4 colors: the machine needs the memory
#define UI_WIDTH      512
#define UI_HEIGHT     384
#define UI_COLORS     16
#define SYSREQ_WIDTH  500
#define SYSREQ_HEIGHT 400
#define SYSREQ_COLORS 4


#include "launcher.h"


// starts the display used by the user interface and sets the theme palette
static void beginUI(int width, int height, int colors)
{
  ibox.begin(VGA_640x480_60Hz, width, height, colors);
  if (colors == 16)
    theme.setupPalette(static_cast<fabgl::VGA16Controller *>(ibox.getDisplayController()));
  else
    theme.setupPalette(static_cast<fabgl::VGA4Controller *>(ibox.getDisplayController()));
  ibox.setBackgroundColor(theme.ink(fabgl::uiThemeInk::Desktop));
}


// noinit! Used to maintain datetime between reboots
__NOINIT_ATTR static timeval savedTimeValue;


static fabgl::WiFiConnection wifi;
static bool wifiConnected = false;
static bool downloadOK    = false;


// try to connected using saved parameters
bool tryToConnect()
{
  bool connected = wifi.state() == fabgl::WiFiConnectionState::Connected;
  if (!connected) {
    char SSID[32] = "";
    char psw[32]  = "";
    if (preferences.getString("SSID", SSID, sizeof(SSID)) && preferences.getString("WiFiPsw", psw, sizeof(psw))) {
      ibox.progressBox("", "Abort", true, 200, [&](fabgl::ProgressForm * form) {
        wifi.connect(SSID, psw, 0);   // just start connection
        for (int i = 0; i < 32 && wifi.state() != fabgl::WiFiConnectionState::Connected; ++i) {
          if (!form->update(i * 100 / 32, "Connecting to %s...", SSID))
            break;
          vTaskDelay(pdMS_TO_TICKS(500));
        }
        connected = (wifi.state() == fabgl::WiFiConnectionState::Connected);
      });
      // show to user the connection state
      if (!connected) {
        wifi.disconnect();
        ibox.message("", "WiFi Connection failed!");
      }
    }
  }
  return connected;
}


// scans the WiFi networks, asks to select one and its password, then connects. Repeats until connected or canceled
bool configureWiFi()
{
  bool connected = false;
  do {

    // scan for networks showing a progress dialog box
    int networksCount = 0;
    fabgl::WiFiScanner scanner;
    ibox.progressBox("", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
      form->update(0, "Scanning WiFi networks...");
      if (scanner.scan(16))
        networksCount = scanner.count();
    });

    // there is no WiFi
    if (networksCount == 0) {
      ibox.message("WiFi Configuration", "No WiFi network found!");
      break;
    }

    // show a selectable list
    StringList list;
    for (int i = 0; i < networksCount; ++i)
      list.appendFmt("%s (%d dBm)", (char const *) scanner.get(i)->ssid, scanner.get(i)->rssi);
    int s = ibox.menu("WiFi Configuration", "Please select a WiFi network", &list);
    if (s < 0)
      break;

    // ask for WiFi password
    char psw[32] = "";
    if (ibox.textInput("WiFi Configuration", "Insert WiFi password", psw, 31, "Cancel", "OK", true) != InputResult::Enter)
      break;

    // connect to WiFi...
    preferences.putString("SSID", (char const *) scanner.get(s)->ssid);
    preferences.putString("WiFiPsw", psw);
    connected = tryToConnect();
    // show to user the connection state
    if (connected)
      ibox.message("WiFi Configuration", "Connection succeeded!");

  } while (!connected);

  return connected;
}


bool checkWiFi()
{
  wifiConnected = tryToConnect();
  // configure WiFi?
  if (!wifiConnected && ibox.message("WiFi Configuration", "Configure WiFi?", "No", "Yes") == InputResult::Enter)
    wifiConnected = configureWiFi();
  return wifiConnected;
}


// handle soft restart
void shutdownHandler()
{
  // save current datetime into Preferences
  gettimeofday(&savedTimeValue, nullptr);
}


void updateDateTime()
{
  // Set timezone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // get datetime from savedTimeValue? (noinit section)
  if (esp_reset_reason() == ESP_RST_SW) {
    // adjust time taking account elapsed time since ESP32 started
    savedTimeValue.tv_usec += (int) esp_timer_get_time();
    savedTimeValue.tv_sec  += savedTimeValue.tv_usec / 1000000;
    savedTimeValue.tv_usec %= 1000000;
    settimeofday(&savedTimeValue, nullptr);
    return;
  }

  if (checkWiFi()) {

    // we need time right now
    ibox.progressBox("", nullptr, true, 200, [&](fabgl::ProgressForm * form) {
      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
      esp_sntp_setservername(0, "pool.ntp.org");
      esp_sntp_init();
      for (int i = 0; i < 12 && sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED; ++i) {
        form->update(i * 100 / 12, "Getting date-time from SNTP...");
        vTaskDelay(pdMS_TO_TICKS(500));
      }
      esp_sntp_stop();
      ibox.setAutoOK(2);
      ibox.message("", "Date and Time updated. Restarting...");
      #ifndef FABGL_EMULATED
      esp_restart();
      #endif
    });

  } else {

    // set default time
    struct tm tm = { };
    tm.tm_hour = 8;
    tm.tm_mday = 14;
    tm.tm_mon  = 7;
    tm.tm_year = 84;
    timeval now = { };
    now.tv_sec = mktime(&tm);
    settimeofday(&now, nullptr);

  }
}


// state shared between downloadURL() and downloadTask()
struct DownloadJob {
  char const *      URL;
  char const *      path;       // full path of the partial file (to reopen it after an SD card error)
  FILE *            file;       // partial file (".part"), positioned at its end. Replaced when reopened
  volatile int64_t  received;   // bytes in the file (initially the size of a previous partial download)
  volatile int64_t  total;      // -1 = unknown
  volatile int      retries;    // reconnections done
  volatile bool     abort;      // set by the UI
  volatile bool     success;
  char const *      error;      // reason of the failure (nullptr = none or aborted)
  SemaphoreHandle_t done;
};


// Downloads URL into job->file. Runs on the core used by WiFi (the VGA signal is generated by the other core),
// so network, TLS and SD card work don't delay the video interrupt.
// The download starts at job->received (a previous partial download is resumed with HTTP Range). A connection that
// stalls or drops is reopened and the download continues from the received bytes.
// The file is synced periodically, so after a reset or a power loss the next download resumes from the last sync.
// An SD card write error (seen after some MB, the card stops answering until it is initialized again) remounts the
// card, reopens the file and continues from its size.
static void downloadTask(void * arg)
{
  constexpr int     MAXRETRIES = 10;              // consecutive reconnections without receiving data
  constexpr int     MAXSDRECOVERIES = 8;          // SD card remounts after write errors
  constexpr int64_t SYNCEVERY  = 1024 * 1024;     // flush data and file size to the SD card every 1 MB

  auto job = (DownloadJob *) arg;
  // Receive buffer in internal DMA-capable RAM, allocated once. The SD card is written through SPI DMA: from a PSRAM
  // buffer the SPI driver allocates a temporary internal DMA buffer for every transaction, and with WiFi and TLS
  // using most of the internal RAM that allocation failed ("SD card write failed"). PSRAM is the fallback.
  constexpr int BUFSIZE = 8192;
  auto buf = (uint8_t *) heap_caps_malloc(BUFSIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  ESP_LOGI("download", "receive buffer in %s RAM, free heap %d bytes", buf ? "internal" : "PSRAM", (int) heap_caps_get_free_size(MALLOC_CAP_8BIT));
  bool bufInternal = (buf != nullptr);
  if (!bufInternal)
    buf = (uint8_t*) SOC_EXTRAM_DATA_LOW; // the machine isn't running yet
  bool fatal     = false;
  bool fromStart = false;   // request the whole file even if there is a partial one
  bool spaceOK   = false;   // free space already checked
  int  failures  = 0;
  int  sdRecoveries = 0;
  bool sdError   = false;   // an SD card write failed: recover before continuing
  int64_t lastSync = job->received;

  // the SD card stopped working: remount it (initializes the card again), reopen the partial file and continue from
  // its size (the data after the last sync may be lost: it is downloaded again)
  auto recoverSD = [&]() {
    fclose(job->file);
    job->file = nullptr;
    if (!FileBrowser::remountSDCard())
      return false;
    job->file = fopen(job->path, "r+b");
    if (!job->file)
      return false;
    setvbuf(job->file, nullptr, _IONBF, 0);
    if (fseek(job->file, 0, SEEK_END) != 0)
      return false;
    int64_t size = ftell(job->file);
    if (size < 0)
      return false;
    if (size < job->received)
      job->received = size;
    if (fseek(job->file, job->received, SEEK_SET) != 0)
      return false;
    lastSync = job->received;
    return true;
  };

  // discard the partial file and start again from the beginning
  auto truncateFile = [&]() {
    fflush(job->file);
    if (ftruncate(fileno(job->file), 0) != 0 || fseek(job->file, 0, SEEK_SET) != 0) {
      job->error = "SD card write failed";
      return false;
    }
    job->received = 0;
    lastSync      = 0;
    return true;
  };

  // WiFi power save (modem sleep) limits the download speed. Disable it during the download (the board restarts
  // without WiFi after downloading, so the emulator isn't affected).
  wifi_ps_type_t savedPS = WIFI_PS_MIN_MODEM;
  esp_wifi_get_ps(&savedPS);
  esp_wifi_set_ps(WIFI_PS_NONE);

  // no stdio buffer: fwrite() passes buf to FatFs as it is
  setvbuf(job->file, nullptr, _IONBF, 0);

  fabgl::HTTPRequest http;
  while (!job->abort && !fatal) {
    if (sdError) {
      sdError = false;
      job->retries = job->retries + 1;
      ESP_LOGW("download", "SD card error at %lld: remounting the card", (long long) job->received);
      if (++sdRecoveries > MAXSDRECOVERIES || !recoverSD()) {
        ESP_LOGE("download", "SD card recovery failed");
        job->error = "SD card write failed";
        break;
      }
      ESP_LOGI("download", "SD card remounted, continuing from %lld", (long long) job->received);
    }
    if (failures > MAXRETRIES) {
      job->error = "Connection lost";
      break;
    }
    if (failures > 0) {
      job->retries = job->retries + 1;
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    int64_t offset = fromStart ? 0 : job->received;
    int status = http.open(job->URL, offset);
    int64_t length = http.contentLength();    // whole file (200) or remaining part (206), -1 = unknown
    if (status != 200 && status != 206)
      ESP_LOGW("download", "open at %lld: HTTP status %d", (long long) offset, status);
    if (status == 200) {
      job->total = length;
      if (job->received > 0) {
        if (fromStart && length >= 0 && length == job->received) {
          // the partial file was already complete (reset before the rename)
          http.close();
          job->success = true;
          break;
        }
        // server ignores Range, or the partial file doesn't match: restart from the beginning
        ESP_LOGW("download", "restarting from the beginning (had %lld bytes)", (long long) job->received);
        if (!truncateFile()) {
          http.close();
          break;
        }
      }
      fromStart = false;
    } else if (status == 206 && offset > 0) {
      // resumed
      job->total = length >= 0 ? offset + length : job->total;
    } else if (status == 416 && offset > 0 && !fromStart) {
      // range not satisfiable: the partial file is already complete or bigger than the remote file.
      // Request the whole file to know its size.
      http.close();
      fromStart = true;
      continue;
    } else if (status >= 400 && status < 500 && status != 408 && status != 429) {
      job->error = status == 404 ? "File not found on the server" : "Download refused by the server";
      http.close();
      break;  // permanent error (not found, forbidden...)
    } else {
      ++failures;
      http.close();
      continue;
    }
    // check the free space once the size is known
    if (!spaceOK && job->total >= 0) {
      int64_t SDTotal, SDUsed;
      if (FileBrowser::getFSInfo(fabgl::DriveType::SDCard, 0, &SDTotal, &SDUsed)) {
        if (SDTotal - SDUsed < job->total - job->received) {
          ESP_LOGE("download", "not enough space: need %lld, free %lld", (long long) (job->total - job->received), (long long) (SDTotal - SDUsed));
          job->error = "Not enough space on the SD card";
          http.close();
          break;
        }
      }
      spaceOK = true;
    }
    // read until the connection fails or ends. Data is collected in buf and written in blocks that end on a sector
    // boundary of the file: FatFs then writes whole sectors straight from buf (aligned, internal RAM), without
    // temporary DMA buffers in the SD driver
    bool cleanEnd = false;
    int  fill     = 0;      // bytes in buf, not written yet
    auto writeBuf = [&]() {
      if (fill > 0 && fwrite(buf, 1, fill, job->file) != fill) {
        ESP_LOGE("download", "SD card write failed at %lld (errno %d, free heap %d, largest %d)", (long long) job->received, errno,
                 (int) heap_caps_get_free_size(MALLOC_CAP_8BIT), (int) heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        sdError = true;
        fill    = 0;      // downloaded again after the recovery
        return false;
      }
      job->received = job->received + fill;
      fill = 0;
      if (job->received - lastSync >= SYNCEVERY) {
        fflush(job->file);
        fsync(fileno(job->file));
        lastSync = job->received;
      }
      return true;
    };
    while (!job->abort) {
      int target = BUFSIZE - (int) (job->received % 512);   // the block ends on a sector boundary
      int c = http.read(buf + fill, target - fill);
      if (c <= 0) {
        cleanEnd = (c == 0);
        if (c < 0 || (job->total >= 0 && job->received + fill < job->total))
          ESP_LOGW("download", "read returned %d at %lld", c, (long long) (job->received + fill));
        break;
      }
      if (job->total >= 0 && job->received + fill + c > job->total)
        c = job->total - job->received - fill;   // never write more than the declared size
      fill += c;
      failures = 0;
      bool complete = job->total >= 0 && job->received + fill >= job->total;
      if ((fill == target || complete) && !writeBuf())
        break;
      if (complete)
        break;
    }
    // data received before the connection ended
    if (!sdError)
      writeBuf();
    http.close();
    if (sdError)
      continue;   // recover the SD card, then reconnect from the received bytes
    if (fatal || job->abort)
      break;
    // complete only when the declared size has been received. With an unknown size (no Content-Length) a clean end
    // of the content is the only indication available.
    if (job->total >= 0 ? job->received >= job->total : (cleanEnd && job->received > 0)) {
      if (fflush(job->file) != 0 || fsync(fileno(job->file)) != 0) {
        ESP_LOGE("download", "SD card sync failed at %lld", (long long) job->received);
        sdError = true;
        continue;
      }
      job->success = true;
      break;
    }
    ++failures;
  }

  if (bufInternal)
    heap_caps_free(buf);
  esp_wifi_set_ps(savedPS);
  xSemaphoreGive(job->done);
  vTaskDelete(nullptr);
}


// download URL into file (opened for writing and positioned at its end: a not empty file is a partial download
// to resume). On failure errorMsg points to the reason (nullptr when aborted by the user).
bool downloadURL(char const * URL, char const * path, FILE * & file, char const * * errorMsg)
{
  downloadOK = false;
  *errorMsg  = nullptr;
  if (!file) {
    *errorMsg = "Unable to create the file on the SD card";
    return false;
  }

  char const * filename = strrchr(URL, '/') + 1;

  DownloadJob job = { };
  job.URL      = URL;
  job.path     = path;
  int64_t startTime = esp_timer_get_time();
  job.file     = file;
  job.received = ftell(file);
  job.total    = -1;
  job.done     = xSemaphoreCreateBinary();
  if (job.received < 0)
    job.received = 0;
  const int64_t startReceived = job.received;

  ibox.progressBox("", "Abort", true, 380, [&](fabgl::ProgressForm * form) {
    form->update(0, job.received > 0 ? "Resuming download of %s" : "Preparing to download %s", filename);
    // the UI task stays on this core, the download runs on the other one
    if (xTaskCreatePinnedToCore(downloadTask, "download", 8192, &job, 5, nullptr, CoreUsage::quietCore() < 0 ? 0 : CoreUsage::quietCore()) != pdPASS) {
      job.error = "Out of memory";
      return;
    }
    while (xSemaphoreTake(job.done, pdMS_TO_TICKS(250)) == pdFALSE) {
      int64_t received = job.received;
      int64_t total    = job.total;
      bool cont;
      if (job.retries > 0)
        cont = form->update(total > 0 ? received * 100 / total : 0, "Downloading %s (%.2f / %.2f MB) retry %d", filename, received / 1048576.0, total / 1048576.0, job.retries);
      else
        cont = form->update(total > 0 ? received * 100 / total : 0, "Downloading %s (%.2f / %.2f MB)", filename, received / 1048576.0, total / 1048576.0);
      if (!cont)
        job.abort = true;   // the task ends after the current read (at most the network timeout)
    }
    downloadOK = job.success;
  });

  vSemaphoreDelete(job.done);
  file      = job.file;     // reopened after an SD card error (nullptr if that failed)
  *errorMsg = job.error;
  double secs = (esp_timer_get_time() - startTime) / 1000000.0;
  ESP_LOGI("download", "%s: %lld bytes in %.1f s (%.1f KB/s), minimum free heap %d bytes", downloadOK ? "completed" : "failed",
           (long long) (job.received - startReceived), secs, secs > 0 ? (job.received - startReceived) / 1024.0 / secs : 0.0,
           (int) heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
  return downloadOK;
}


// return filename if successfully downloaded or already exist
// The image is downloaded into "<filename>.part" and renamed when complete, so an image with its final name is never
// partial. A ".part" left by a failed, aborted or interrupted (reset, power loss) download is resumed next time.
char const * getDisk(char const * url)
{
  FileBrowser fb(SD_MOUNT_PATH);

  char const * filename = nullptr;
  if (url) {
    if (strncmp("http://", url, 7) == 0 || strncmp("https://", url, 8) == 0) {
      // this is actually an URL
      filename = strrchr(url, '/') + 1;
      if (filename && !fb.exists(filename, false)) {
        // disk doesn't exist, try to download
        if (!checkWiFi())
          return nullptr;
        char partname[strlen(filename) + 6];
        sprintf(partname, "%s.part", filename);
        bool resume = fb.exists(partname, false);
        auto file = fb.openFile(partname, resume ? "r+b" : "wb");
        if (file && resume)
          fseek(file, 0, SEEK_END);
        char const * errorMsg;
        char partpath[strlen(SD_MOUNT_PATH) + 1 + strlen(partname) + 1];
        sprintf(partpath, "%s/%s", SD_MOUNT_PATH, partname);
        bool success = downloadURL(url, partpath, file, &errorMsg);
        if (file)
          fclose(file);
        if (success) {
          fb.rename(partname, filename);
          if (!fb.filePathExists(filename)) {   // exists() checks the cached directory listing
            success  = false;
            errorMsg = "Unable to rename the file on the SD card";
          }
        }
        if (!success) {
          // keep the partial file: the download is resumed next time
          ibox.message("Download failed", errorMsg ? errorMsg : "Download aborted. It will be resumed next time.");
          downloadOK = false;
          return nullptr;
        }
      }
    } else {
      // this is just a file
      if (fb.filePathExists(url))
        filename = url;
    }
  }
  return filename;
}


// user pressed SYSREQ (ALT + PRINTSCREEN)
void sysReqCallback()
{
  machine->graphicsAdapter()->enableVideo(false);
  beginUI(SYSREQ_WIDTH, SYSREQ_HEIGHT, SYSREQ_COLORS);

  int s = ibox.menu("", "Select a command", "Restart Emulator;Restart Machine;Mount Disk;Continue");
  switch (s) {

    // Restart Emulator
    case 0:
      esp_restart();
      break;
      
    // Restart Machine
    case 1:
      machine->trigReset();
      break;

    // Mount Disk
    case 2:
    {
      int s = ibox.menu("", "Select Drive", "Floppy A (fd0);Floppy B (fd1)");
      if (s > -1) {
        constexpr int MAXNAMELEN = 256;
        unique_ptr<char[]> dir(new char[MAXNAMELEN + 1] { '/', 'S', 'D', 0 } );
        unique_ptr<char[]> filename(new char[MAXNAMELEN + 1] { 0 } );
        if (machine->diskFilename(s))
          strcpy(filename.get(), machine->diskFilename(s));
        if (ibox.fileSelector("Select Disk Image", "Image Filename", dir.get(), MAXNAMELEN, filename.get(), MAXNAMELEN) == InputResult::Enter) {
          machine->setDriveImage(s, filename.get());
        }
      }
      break;
    }

    // Continue
    default:
      break;
  }

  ibox.end();
  PS2Controller::keyboard()->enableVirtualKeys(false, false); // don't use virtual keys
  machine->graphicsAdapter()->enableVideo(true);
}


void setup()
{
  vTaskDelay(pdMS_TO_TICKS(500)); printf("\n\n\nReset\n\n");// DEBUG ONLY



  preferences.begin("PCEmulator", false);

  // uncomment to clear preferences
  //preferences.clear();
  
  // save some space reducing UI queue
  fabgl::BitmappedDisplayController::queueSize = 128;

  beginUI(UI_WIDTH, UI_HEIGHT, UI_COLORS);

  // every InputBox dialog gets the theme, with the header on top
  ibox.onInitApp = [&](uiApp * app) { theme.apply(app); };
  ibox.onPaint   = [&](Canvas * canvas) { drawInfo(canvas); };

  // This app requires PSRAM. PSRAM is just mapped into the address space (CONFIG_SPIRAM_USE_MEMMAP=y) at SOC_EXTRAM_DATA_LOW,
  // and it is directly accessed by the app. Compiler workaround for PSRAM bug (CONFIG_SPIRAM_CACHE_WORKAROUND)
  // is disabled because it slows down CPU too much (see sdkconfig.defaults).
  if (!esp_psram_is_initialized())
    ibox.message("Error!", "This app requires a board with PSRAM!", nullptr, nullptr);

  if (!FileBrowser::mountSDCard(false, SD_MOUNT_PATH, 12))   // two handles for each disk image (see Machine::diskRead())
    ibox.message("Error!", "This app requires a SD-CARD!", nullptr, nullptr);

  // uncomment to format SD!
  //FileBrowser::format(fabgl::DriveType::SDCard, 0);

  esp_register_shutdown_handler(shutdownHandler);

  updateDateTime();


  // machine configurations
  MachineConf mconf;

  // the launcher: select a machine configuration, edit them, browse files, configure WiFi
  int idx     = preferences.getInt("dconf", 0);
  int autoRun = 6;    // seconds, only the first time

  for (bool showLauncher = true; showLauncher; ) {

    loadMachineConfiguration(&mconf);

    LauncherApp launcher(&mconf, idx, autoRun);
    launcher.run(ibox.getDisplayController());
    idx     = launcher.selected;
    autoRun = 0;

    switch (launcher.action) {
      case LauncherAction::Run:
        showLauncher = false;
        break;
      case LauncherAction::Edit:
        editConfigDialog(&ibox, &mconf, idx);
        break;
      case LauncherAction::New:
        newConfigDialog(&ibox, &mconf, imax(idx, 0));
        break;
      case LauncherAction::Remove:
        delConfigDialog(&ibox, &mconf, idx);
        break;
      case LauncherAction::Files:
        ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
        break;
      case LauncherAction::WiFi:
        // only to save the credentials: the network is used when a disk must be downloaded
        configureWiFi();
        wifi.disconnect();
        break;
    }
  }

  idx = imax(idx, 0);
  preferences.putInt("dconf", idx);

  // setup selected configuration
  auto conf = mconf.getItem(idx);

  char const * diskFilename[DISKCOUNT];
  downloadOK = true;
  for (int i = 0; i < DISKCOUNT && downloadOK; ++i)
    diskFilename[i] = getDisk(conf->disk[i]);

  if (!downloadOK || (!diskFilename[0] && !diskFilename[2])) {
    // unable to get boot disks
    ibox.message("Error!", "Unable to get system disks!");
    esp_restart();
  }

  if (wifiConnected) {
    // disk downloaded from the Internet, need to reboot to fully disable wifi
    ibox.setAutoOK(2);
    ibox.message("", "Disks downloaded. Restarting...");
    #ifndef FABGL_EMULATED
    esp_restart();
    #endif
  }

  ibox.end();
  
  // without WiFi it is possible to increase SD card speed
  FileBrowser::setSDCardMaxFreqKHz(SDMMC_FREQ_DEFAULT);
  FileBrowser::remountSDCard();

  machine = new Machine;

  machine->setBaseDirectory(SD_MOUNT_PATH);
  for (int i = 0; i < DISKCOUNT; ++i)
    machine->setDriveImage(i, diskFilename[i], conf->cylinders[i], conf->heads[i], conf->sectors[i]);

  machine->setBootDrive(conf->bootDrive);
  
  auto serial1 = new SerialPort;
  serial1->setSignals(UART_URX, UART_UTX);
  machine->setCOM1(serial1);

  /*
  printf("MALLOC_CAP_32BIT : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
  printf("MALLOC_CAP_8BIT  : %d bytes (largest %d bytes)\r\n", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  printf("MALLOC_CAP_DMA   : %d bytes (largest %d bytes)\r\n\n", heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_largest_free_block(MALLOC_CAP_DMA));

  heap_caps_dump_all();
  */

  machine->setSysReqCallback(sysReqCallback);

  machine->run();
}



#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
namespace fabgl {
  extern volatile uint64_t s_vgapalctrlcycles;
}
using fabgl::s_vgapalctrlcycles;
#endif



void loop()
{

#if FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK
  static uint32_t tcpu = 0, s1 = 0, count = 0;
  tcpu = machine->ticksCounter();
  s_vgapalctrlcycles = 0;
  s1 = fabgl::getCycleCount();
  vTaskDelay(pdMS_TO_TICKS(1000));
  printf("%d\tCPU: %d", count, machine->ticksCounter() - tcpu);
  printf("   Graph: %lld / %d   (%d%%)\n", s_vgapalctrlcycles, fabgl::getCycleCount() - s1, (int)((double)s_vgapalctrlcycles/240000000*100));
  ++count;
#else

  vTaskDelete(NULL);

#endif
}


extern "C" void app_main()
{
  setup();
  while (true)
    loop();
}
