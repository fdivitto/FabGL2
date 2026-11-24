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


/**
 * @file
 *
 * @brief This file contains the fabgl::Preferences definition (key/value storage in NVS flash).
 */


#include <stdint.h>
#include <stddef.h>

#include "nvs.h"


namespace fabgl {


/**
 * @brief Simple key/value storage in non-volatile flash (NVS).
 *
 * A thin wrapper over the ESP-IDF NVS API. Each Preferences object works on one NVS namespace.
 * nvs_flash_init() is called automatically when needed.
 *
 * Example:
 *
 *     fabgl::Preferences preferences;
 *     preferences.begin("MyApp");
 *     int value = preferences.getInt("counter", 0);
 *     preferences.putInt("counter", value + 1);
 *
 * @ingroup storage
 */
class Preferences {

public:

  Preferences();
  ~Preferences();

  /**
   * @brief Opens the specified namespace
   *
   * @param name Namespace name (max 15 characters)
   * @param readOnly If true, the namespace is opened in read-only mode
   *
   * @return True on success
   */
  bool begin(char const * name, bool readOnly = false);

  /**
   * @brief Commits changes and closes the namespace
   */
  void end();

  /**
   * @brief Removes all keys of the namespace
   */
  bool clear();

  /**
   * @brief Removes the specified key
   */
  bool remove(char const * key);

  /**
   * @brief Determines whether the specified key exists
   */
  bool isKey(char const * key);

  bool    putInt(char const * key, int32_t value);
  int32_t getInt(char const * key, int32_t defaultValue = 0);

  bool     putUInt(char const * key, uint32_t value);
  uint32_t getUInt(char const * key, uint32_t defaultValue = 0);

  bool putBool(char const * key, bool value);
  bool getBool(char const * key, bool defaultValue = false);

  bool putString(char const * key, char const * value);

  /**
   * @brief Gets a string
   *
   * @param key Key name
   * @param value Destination buffer
   * @param maxLen Destination buffer size (including the terminating zero)
   *
   * @return Number of bytes copied (including the terminating zero), 0 if the key does not exist
   */
  size_t getString(char const * key, char * value, size_t maxLen);

  bool   putBytes(char const * key, void const * value, size_t len);
  size_t getBytes(char const * key, void * buffer, size_t maxLen);


private:

  nvs_handle_t m_handle;
  bool         m_opened;
  bool         m_readOnly;
};


} // end of namespace
