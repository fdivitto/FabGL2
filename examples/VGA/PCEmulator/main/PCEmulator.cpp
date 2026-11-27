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


bool checkWiFi()
{
  wifiConnected = tryToConnect();
  if (!wifiConnected) {

    // configure WiFi?
    if (ibox.message("WiFi Configuration", "Configure WiFi?", "No", "Yes") == InputResult::Enter) {

      // repeat until connected or until user cancels
      do {

        // yes, scan for networks showing a progress dialog box
        int networksCount = 0;
        fabgl::WiFiScanner scanner;
        ibox.progressBox("", nullptr, false, 200, [&](fabgl::ProgressForm * form) {
          form->update(0, "Scanning WiFi networks...");
          if (scanner.scan(16))
            networksCount = scanner.count();
        });

        // are there available WiFi?
        if (networksCount > 0) {

          // yes, show a selectable list
          StringList list;
          for (int i = 0; i < networksCount; ++i)
            list.appendFmt("%s (%d dBm)", (char const *) scanner.get(i)->ssid, scanner.get(i)->rssi);
          int s = ibox.menu("WiFi Configuration", "Please select a WiFi network", &list);

          // user selected something?
          if (s > -1) {
            // yes, ask for WiFi password
            char psw[32] = "";
            if (ibox.textInput("WiFi Configuration", "Insert WiFi password", psw, 31, "Cancel", "OK", true) == InputResult::Enter) {
              // user pressed OK, connect to WiFi...
              preferences.putString("SSID", (char const *) scanner.get(s)->ssid);
              preferences.putString("WiFiPsw", psw);
              wifiConnected = tryToConnect();
              // show to user the connection state
              if (wifiConnected)
                ibox.message("", "Connection succeeded!");
            } else
              break;
          } else
            break;
        } else {
          // there is no WiFi
          ibox.message("", "No WiFi network found!");
          break;
        }

      } while (!wifiConnected);

    }

  }

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
  FILE *            file;
  volatile int64_t  received;
  volatile int64_t  total;      // -1 = unknown
  volatile int      retries;    // reconnections done
  volatile bool     abort;      // set by the UI
  volatile bool     success;
  SemaphoreHandle_t done;
};


// Downloads URL into job->file. Runs on the core used by WiFi (the VGA signal is generated by the other core),
// so network, TLS and SD card work don't delay the video interrupt.
// A connection that stalls or drops is reopened and the download resumes from the received bytes (HTTP Range).
static void downloadTask(void * arg)
{
  constexpr int MAXRETRIES = 10;  // consecutive reconnections without receiving data

  auto job = (DownloadJob *) arg;
  auto buf = (uint8_t*) SOC_EXTRAM_DATA_LOW; // use PSRAM as buffer (the machine isn't running yet)
  bool writeError = false;
  int  failures   = 0;

  // WiFi power save (modem sleep) limits the download speed. Disable it during the download (the board restarts
  // without WiFi after downloading, so the emulator isn't affected).
  wifi_ps_type_t savedPS = WIFI_PS_MIN_MODEM;
  esp_wifi_get_ps(&savedPS);
  esp_wifi_set_ps(WIFI_PS_NONE);

  fabgl::HTTPRequest http;
  while (!job->abort && !writeError && failures <= MAXRETRIES) {
    if (failures > 0) {
      job->retries = job->retries + 1;
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    int64_t offset = job->received;
    int status = http.open(job->URL, offset);
    if (status != 200 && status != 206)
      ESP_LOGW("download", "open at %lld: HTTP status %d", (long long) offset, status);
    if (offset == 0 && status == 200) {
      job->total = http.contentLength();
    } else if (offset > 0 && status == 206) {
      // resumed
    } else if (offset > 0 && status == 200) {
      // server ignores Range: restart from the beginning
      if (fseek(job->file, 0, SEEK_SET) != 0)
        break;
      job->received = 0;
      job->total    = http.contentLength();
    } else if (status >= 400 && status < 500 && status != 408 && status != 429) {
      break;  // permanent error (not found, forbidden...)
    } else {
      ++failures;
      http.close();
      continue;
    }
    // read until the connection fails or ends
    while (!job->abort) {
      int c = http.read(buf, 16384);
      if (c <= 0) {
        if (c < 0 || (job->total >= 0 && job->received < job->total))
          ESP_LOGW("download", "read returned %d at %lld", c, (long long) job->received);
        break;
      }
      if (fwrite(buf, 1, c, job->file) != c) {
        ESP_LOGE("download", "SD card write failed at %lld (errno %d)", (long long) job->received, errno);
        writeError = true;
        break;
      }
      job->received = job->received + c;
      failures = 0;
    }
    http.close();
    if (job->total >= 0 ? job->received >= job->total : (job->received > 0 && !writeError)) {
      job->success = !job->abort && !writeError;
      break;
    }
    ++failures;
  }

  esp_wifi_set_ps(savedPS);
  xSemaphoreGive(job->done);
  vTaskDelete(nullptr);
}


// download specified filename from URL
bool downloadURL(char const * URL, FILE * file)
{
  downloadOK = false;
  if (!file)
    return false;

  char const * filename = strrchr(URL, '/') + 1;

  DownloadJob job = { };
  job.URL   = URL;
  job.file  = file;
  job.total = -1;
  job.done  = xSemaphoreCreateBinary();

  ibox.progressBox("", "Abort", true, 380, [&](fabgl::ProgressForm * form) {
    form->update(0, "Preparing to download %s", filename);
    // the UI task stays on this core, the download runs on the other one
    if (xTaskCreatePinnedToCore(downloadTask, "download", 8192, &job, 5, nullptr, CoreUsage::quietCore() < 0 ? 0 : CoreUsage::quietCore()) != pdPASS)
      return;
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
  return downloadOK;
}


// return filename if successfully downloaded or already exist
char const * getDisk(char const * url)
{
  FileBrowser fb(SD_MOUNT_PATH);

  char const * filename = nullptr;
  if (url) {
    if (strncmp("http://", url, 7) == 0 || strncmp("https://", url, 7) == 0) {
      // this is actually an URL
      filename = strrchr(url, '/') + 1;
      if (filename && !fb.exists(filename, false)) {
        // disk doesn't exist, try to download
        if (!checkWiFi())
          return nullptr;
        auto file = fb.openFile(filename, "wb");
        bool success = downloadURL(url, file);
        fclose(file);
        if (!success) {
          fb.remove(filename);
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
  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);

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

  ibox.begin(VGA_640x480_60Hz, 500, 400, 4);
  ibox.setBackgroundColor(RGB888(0, 0, 0));

  ibox.onPaint = [&](Canvas * canvas) { drawInfo(canvas); };

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

  // show a list of machine configurations

  ibox.setAutoOK(6);
  int idx = preferences.getInt("dconf", 0);

  for (bool showDialog = true; showDialog; ) {

    loadMachineConfiguration(&mconf);

    StringList dconfs;
    for (auto conf = mconf.getFirstItem(); conf; conf = conf->next)
      dconfs.append(conf->desc);
    dconfs.select(idx, true);

    ibox.setupButton(0, "Files");
    ibox.setupButton(1, "Machine", "Edit;New;Remove", 52);
    auto r = ibox.select("Machine Configurations", "Please select a machine configuration", &dconfs, nullptr, "Run");

    idx = dconfs.getFirstSelected();

    switch (r) {
      case InputResult::ButtonExt0:
        // Browse Files
        ibox.folderBrowser("Browse Files", SD_MOUNT_PATH);
        break;
      case InputResult::ButtonExt1:
        // Machine
        switch (ibox.selectedSubItem()) {
          // Edit
          case 0:
            editConfigDialog(&ibox, &mconf, idx);
            break;
          // New
          case 1:
            newConfigDialog(&ibox, &mconf, idx);
            break;
          // Remove
          case 2:
            delConfigDialog(&ibox, &mconf, idx);
            break;
        };
        break;
      case InputResult::Enter:
        // Run
        showDialog = false;
        break;
      default:
        break;
    }

    // next selection will not have timeout
    ibox.setAutoOK(0);
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
