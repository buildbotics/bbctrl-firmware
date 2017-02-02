/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#ifdef _MSC_VER
#include "Win32Registry.h"
#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

using namespace std;
using namespace cb;


static HKEY parseKey(const string &path, string &key, string &name) {
  size_t first = path.find("\\");
  size_t last = path.find_last_of("\\");
  string root =
    String::toUpper(first == string::npos ? path : path.substr(0, first));

  // Key
  if (first == last) key = string();
  else key = path.substr(first + 1, last - first - 1);

  // Name
  name = first == string::npos ? string() : path.substr(last + 1);

  // Root
  if (root == "HKCR" || root == "HKEY_CLASSES_ROOT")
    return HKEY_CLASSES_ROOT;
  if (root == "HKCC" || root == "HKEY_CURRENT_CONFIG")
    return HKEY_CURRENT_CONFIG;
  if (root == "HKCU" || root == "HKEY_CURRENT_USER")
    return HKEY_CURRENT_USER;
  if (root == "HKLM" || root == "HKEY_LOCAL_MACHINE")
    return HKEY_LOCAL_MACHINE;
  if (root == "HKU" || root == "HKEY_USERS")
    return HKEY_USERS;

  THROWS("Invalid root key " << root);
}


static HKEY openRegKey(HKEY root, const string &key, REGSAM sam) {
  HKEY hKey;
  long ret = RegOpenKeyEx(root, (LPCTSTR)key.c_str(), 0, sam, &hKey);

  if (ret)
    THROWS("Failed to open registry key '" << key << "': " << SysError(ret));

  return hKey;
}


static void setRegKey(const string &path, uint32_t type, const void *data,
                      uint32_t size) {
  string key, name;
  HKEY root = parseKey(path, key, name);
  HKEY hKey = openRegKey(root, key, KEY_SET_VALUE);

  long ret = RegSetValueEx(hKey, (LPCTSTR)name.c_str(), 0, type,
                           (const BYTE *)data, size);

  if (ret)
    THROWS("Failed to set registry key '" << path << "': " << SysError(ret));

  RegCloseKey(hKey);
}


static uint32_t getRegKey(const string &path, uint32_t type, void *data,
                          uint32_t size) {
  uint32_t expectedType = type;
  string key, name;
  HKEY root = parseKey(path, key, name);
  HKEY hKey = openRegKey(root, key, KEY_QUERY_VALUE);

  long ret = RegQueryValueEx(hKey, (LPCTSTR)name.c_str(), 0, (LPDWORD)&type,
                             (LPBYTE)data, (LPDWORD)&size);

  if (ret)
    THROWS("Failed to get registry key '" << path << "': " << SysError(ret));

  if (type != expectedType)
    THROWS("Type mismatch when reading key '" << path << "' " << type << "!="
           << expectedType);

  RegCloseKey(hKey);

  return size;
}


bool Win32Registry::has(const string &path) {
  string key, name;
  HKEY root = parseKey(path, key, name);

  HKEY hKey;
  if (RegOpenKeyEx(root, (LPCTSTR)key.c_str(), 0, KEY_QUERY_VALUE, &hKey))
    return false;

  bool exists = RegQueryValueEx(hKey, (LPCTSTR)name.c_str(), 0, 0, 0, 0) !=
    ERROR_FILE_NOT_FOUND;

  RegCloseKey(hKey);

  return exists;
}


uint32_t Win32Registry::getU32(const string &path) {
  uint32_t value;
  getRegKey(path, REG_DWORD, &value, sizeof(uint32_t));
  return value;
}


uint64_t Win32Registry::getU64(const string &path) {
  uint64_t value;
  getRegKey(path, REG_QWORD, &value, sizeof(uint64_t));
  return value;
}


string Win32Registry::getString(const string &path) {
  // Get size
  uint32_t size = getRegKey(path, REG_SZ, 0, 0);

  // Allocate buffer
  SmartPointer<char>::Array buffer = new char[size + 1];
  buffer[size] = 0;

  // Get string
  getRegKey(path, REG_SZ, buffer.get(), size);

  return buffer.get();
}


uint32_t Win32Registry::getBinary(const string &path, char *buffer,
                                  uint32_t size) {
  return getRegKey(path, REG_BINARY, buffer, size);
}


void Win32Registry::set(const string &path, uint32_t value) {
  setRegKey(path, REG_DWORD, &value, sizeof(uint32_t));
}


void Win32Registry::set(const string &path, uint64_t value) {
  setRegKey(path, REG_QWORD, &value, sizeof(uint64_t));
}


void Win32Registry::set(const string &path, const string &value) {
  setRegKey(path, REG_SZ, value.c_str(), value.length() + 1);
}


void Win32Registry::set(const string &path, const char *buffer, uint32_t size) {
  setRegKey(path, REG_BINARY, &buffer, size);
}


void Win32Registry::remove(const string &path) {
  string key, name;
  HKEY hKey = parseKey(path, key, name);

  key = key + '\\' + name;

  long ret = RegDeleteKey(hKey, (LPCTSTR)key.c_str());

  if (ret)
    THROWS("Failed to delete registry key '" << path << "': " << SysError(ret));
}

#endif // _MSC_VER
