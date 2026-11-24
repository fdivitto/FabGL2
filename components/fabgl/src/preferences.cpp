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


#include <string.h>

#include "nvs_flash.h"

#include "preferences.h"


namespace fabgl {


Preferences::Preferences()
  : m_handle(0),
    m_opened(false),
    m_readOnly(false)
{
}


Preferences::~Preferences()
{
  end();
}


bool Preferences::begin(char const * name, bool readOnly)
{
  end();

  auto r = nvs_flash_init();
  if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    r = nvs_flash_init();
  }
  if (r != ESP_OK)
    return false;

  m_readOnly = readOnly;
  m_opened   = (nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &m_handle) == ESP_OK);
  return m_opened;
}


void Preferences::end()
{
  if (m_opened) {
    if (!m_readOnly)
      nvs_commit(m_handle);
    nvs_close(m_handle);
    m_opened = false;
  }
}


bool Preferences::clear()
{
  return m_opened && !m_readOnly && nvs_erase_all(m_handle) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


bool Preferences::remove(char const * key)
{
  return m_opened && !m_readOnly && nvs_erase_key(m_handle, key) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


bool Preferences::isKey(char const * key)
{
  return m_opened && nvs_find_key(m_handle, key, nullptr) == ESP_OK;
}


bool Preferences::putInt(char const * key, int32_t value)
{
  return m_opened && !m_readOnly && nvs_set_i32(m_handle, key, value) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


int32_t Preferences::getInt(char const * key, int32_t defaultValue)
{
  int32_t value = defaultValue;
  if (m_opened)
    nvs_get_i32(m_handle, key, &value);
  return value;
}


bool Preferences::putUInt(char const * key, uint32_t value)
{
  return m_opened && !m_readOnly && nvs_set_u32(m_handle, key, value) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


uint32_t Preferences::getUInt(char const * key, uint32_t defaultValue)
{
  uint32_t value = defaultValue;
  if (m_opened)
    nvs_get_u32(m_handle, key, &value);
  return value;
}


bool Preferences::putBool(char const * key, bool value)
{
  return m_opened && !m_readOnly && nvs_set_u8(m_handle, key, value ? 1 : 0) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


bool Preferences::getBool(char const * key, bool defaultValue)
{
  uint8_t value = defaultValue ? 1 : 0;
  if (m_opened)
    nvs_get_u8(m_handle, key, &value);
  return value != 0;
}


bool Preferences::putString(char const * key, char const * value)
{
  return m_opened && !m_readOnly && nvs_set_str(m_handle, key, value) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


size_t Preferences::getString(char const * key, char * value, size_t maxLen)
{
  size_t len = maxLen;
  if (m_opened && value && maxLen > 0 && nvs_get_str(m_handle, key, value, &len) == ESP_OK)
    return len;
  return 0;
}


bool Preferences::putBytes(char const * key, void const * value, size_t len)
{
  return m_opened && !m_readOnly && nvs_set_blob(m_handle, key, value, len) == ESP_OK && nvs_commit(m_handle) == ESP_OK;
}


size_t Preferences::getBytes(char const * key, void * buffer, size_t maxLen)
{
  size_t len = maxLen;
  if (m_opened && nvs_get_blob(m_handle, key, buffer, &len) == ESP_OK)
    return len;
  return 0;
}


} // end of namespace
