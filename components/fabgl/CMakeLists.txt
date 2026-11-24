# FabGL2 - ESP-IDF component
#
# Requires ESP-IDF v6.1 or newer. Only components shipped with ESP-IDF are used.

idf_component_register(
  SRC_DIRS     src
               src/comdrivers
               src/devdrivers
               src/dispdrivers
               src/emudevs
               src/network
  INCLUDE_DIRS src
               src/fonts
               src/images
               src/comdrivers
               src/devdrivers
               src/dispdrivers
               src/emudevs
               src/network
  REQUIRES     esp_driver_gpio
               esp_driver_spi
               esp_driver_i2c
               esp_driver_uart
               esp_driver_sdspi
               esp_driver_sdm
               esp_driver_dac
               esp_adc
               esp_timer
               esp_wifi
               esp_netif
               esp_event
               esp_http_client
               mbedtls
               lwip
               fatfs
               spiffs
               sdmmc
               vfs
               nvs_flash
               spi_flash
               ulp
               soc
               hal
  PRIV_REQUIRES esp_mm esp_psram esp_hw_support esp_rom efuse
)

# -fno-strict-aliasing: the library reinterprets pixel and buffer data through pointers of different types
target_compile_options(${COMPONENT_LIB} PRIVATE -O2 -fno-strict-aliasing)
