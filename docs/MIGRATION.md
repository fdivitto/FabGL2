# Migrating from FabGL 1.x (Arduino) to FabGL2 (ESP-IDF 6.1)

FabGL2 is a pure ESP-IDF component. Arduino core, Arduino IDE and PlatformIO "arduino" framework are no longer supported.

## Project structure

| FabGL 1.x                                 | FabGL2                                                      |
|-------------------------------------------|-------------------------------------------------------------|
| `src/`, `library.properties`, `library.json`, `component.mk` | `components/fabgl/` (`CMakeLists.txt`, `Kconfig`, `idf_component.yml`, `src/`) |
| `examples/VGA/Sprites/Sprites.ino`        | `examples/VGA/Sprites/` ESP-IDF project: `CMakeLists.txt`, `sdkconfig.defaults`, `main/Sprites.cpp` |
| `setup()` / `loop()`                      | `extern "C" void app_main()` (examples still use `setup()`/`loop()` functions called by `app_main()`) |

Header names and namespaces are unchanged (`#include "fabgl.h"`, `fabgl::...`).

## Arduino API replacements

| Arduino                                   | ESP-IDF / FabGL2                                            |
|-------------------------------------------|-------------------------------------------------------------|
| `delay(ms)`                               | `vTaskDelay(pdMS_TO_TICKS(ms))`                             |
| `millis()` / `micros()`                   | `esp_timer_get_time() / 1000` / `esp_timer_get_time()`      |
| `random(n)`                               | `esp_random() % n`                                          |
| `pinMode()` / `digitalRead()` / `digitalWrite()` | `fabgl::configureGPIO()`, `gpio_get_level()`, `gpio_set_level()` |
| `Serial.printf()`                         | `printf()`                                                  |
| `Serial` as `Stream` (read/available/write, `Terminal.setLogStream()`) | `fabgl::UARTStream` (ESP-IDF UART driver)  |
| `Preferences`                             | `fabgl::Preferences` (same methods, implemented on NVS)     |
| `WiFi.begin()` / `WiFi.status()`          | `fabgl::WiFiConnection::connect()` / `state()`              |
| `WiFi.scanNetworks()`                     | `fabgl::WiFiScanner`                                        |
| `WiFiClient`                              | `fabgl::TCPClient` (BSD sockets)                            |
| `HTTPClient`                              | `fabgl::HTTPRequest` (`GET()` with `onData` delegate, or `open()`/`read()`/`close()` streaming, HTTPS with certificate bundle) |
| `SPIFFS.begin()` / `SPIFFS.open()`        | `fabgl::FileBrowser::mountSPIFFS()` + `fopen()` (a partition table with a `spiffs` partition is required) |
| `Wire` based `fabgl::I2C`                 | `fabgl::I2C` (same API, ESP-IDF `driver/i2c_master.h`; added `writeRead()` and `probe()`) |
| `fabgl::ICMP::hostIP()` (Arduino `IPAddress`) | `hostIP()` returns `esp_ip4_addr_t`, `hostIPString()` returns a dotted string |
| `ESP.restart()`                           | `esp_restart()`                                             |
| `disableCore0WDT()`                       | `esp_task_wdt_delete(xTaskGetIdleTaskHandleForCore(0))`     |

## Legacy ESP-IDF drivers replaced inside the library

| Removed / legacy                          | Replacement                                                 |
|-------------------------------------------|-------------------------------------------------------------|
| `driver/adc.h` (`adc1_get_raw`, ...)      | `esp_adc/adc_oneshot.h` (see `fabgl::ADC1_configure()` / `ADC1_read()`) |
| `driver/dac.h` (`dac_i2s_enable`, ...)    | DAC LL HAL (`fabgl::DAC_enableDMA()`, `DAC_outputEnable()`), `driver/dac_oneshot.h` in examples |
| `driver/sigmadelta.h`                     | `driver/sdm.h`                                              |
| `driver/periph_ctrl.h`, `PERIPH_I2S1_MODULE` | `fabgl::I2S_moduleEnable()` / `I2S_moduleDisable()` (RCC atomic) |
| `rtc_clk_apll_enable(en, sdm0, ...)`      | `fabgl::APLL_enable()` / `APLL_disable()`                   |
| `gpio_matrix_out()` / `gpio_matrix_in()`  | `esp_rom_gpio_connect_out_signal()` / `esp_rom_gpio_connect_in_signal()` |
| `rom/lldesc.h`                            | `esp32/rom/lldesc.h`                                        |
| `esp_spi_flash.h`                         | `esp_private/cache_utils.h`                                 |
| `esp_spiram_init()` (PCEmulator)          | `CONFIG_SPIRAM=y` + `CONFIG_SPIRAM_USE_MEMMAP=y`            |
| `f_mkfs()` / `f_fdisk()`                  | `esp_vfs_fat_sdcard_format()`                               |
| `esp_task_wdt_init(timeout, panic)`       | `esp_task_wdt_reconfigure()`                                |
| `HSPI_HOST` / `VSPI_HOST`                 | `SPI2_HOST` / `SPI3_HOST`                                   |

## Configuration

The PS/2 controller requires the ULP FSM coprocessor (`CONFIG_ULP_COPROC_ENABLED=y`, `CONFIG_ULP_COPROC_TYPE_FSM=y`)
and at least 2112 bytes of reserved RTC slow memory (`CONFIG_ULP_COPROC_RESERVE_MEM`). Including the component automatically selects
`CONFIG_ULP_COPROC_ENABLED`; the reserved memory size must be set in `sdkconfig.defaults` (a `static_assert` reports it).

Examples run `app_main()` on CPU 1 (`CONFIG_ESP_MAIN_TASK_AFFINITY_CPU1=y`) with 8KB of stack, like the Arduino `loop()` task,
and disable the idle task watchdog on CPU 1 (`CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1=n`) because many examples busy-loop.
