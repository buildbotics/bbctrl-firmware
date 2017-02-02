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

#ifndef CBANG_MSC_VER_REGISTRY_H
#define CBANG_MSC_VER_REGISTRY_H
#ifdef _MSC_VER

#include <cbang/StdTypes.h>

#include <string>

namespace cb {
  class Win32Registry {
  public:
    static bool has(const std::string &path);

    static uint32_t getU32(const std::string &path);
    static uint64_t getU64(const std::string &path);
    static std::string getString(const std::string &path);
    static uint32_t getBinary(const std::string &path, char *buffer,
                              uint32_t size);

    static void set(const std::string &path, uint32_t value);
    static void set(const std::string &path, uint64_t value);
    static void set(const std::string &path, const std::string &value);
    static void set(const std::string &path, const char *buffer, uint32_t size);

    static void remove(const std::string &path);
  };
}

#endif // _MSC_VER
#endif // CBANG_MSC_VER_REGISTRY_H
