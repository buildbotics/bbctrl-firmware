/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
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

#ifndef CBANG_STRING_H
#define CBANG_STRING_H

#include <cbang/StdTypes.h>

#include <string>
#include <vector>
#include <istream>

#include <stdarg.h>

namespace cb {
  /// Used for convenient conversion of basic data types to and from std::string
  class String : public std::string {
  public:
    static const std::string DEFAULT_DELIMS;
    static const std::string DEFAULT_LINE_DELIMS;
    static const std::string LETTERS_LOWER_CASE;
    static const std::string LETTERS_UPPER_CASE;
    static const std::string LETTERS;
    static const std::string NUMBERS;
    static const std::string ALPHA_NUMERIC;

    /// Copy at most @param n bytes from @param s.
    String(const char *s, size_type n);

    // Pass through constructors
    /// See std::string
    String(const char *s) : std::string(s) {}

    /// See std::string
    String(const std::string &s) : std::string(s) {}

    /// See std::string
    String(const std::string &s, size_type pos, size_type n = npos) :
      std::string(s, pos, n) {}

    /// See std::string
    String(size_type n, char c) : std::string(n, c) {}

    // Conversion constructors
    /// Convert a 32-bit signed value to a string.
    explicit String(int32_t x);

    /// Convert a 32-bit unsigned value to a string.
    explicit String(uint32_t x);

    /// Convert a 64-bit signed value a string.
    explicit String(int64_t x);

    /// Convert a 64-bit unsigned value to a string.
    explicit String(uint64_t x);

    /// Convert a 128-bit unsigned value to a string.
    explicit String(uint128_t x);

    /// Convert a double value to a string.
    explicit String(double x);

    /// Convert a boolean value to a string.
    explicit String(bool x);

    static std::string printf(const char *format, ...);
    static std::string vprintf(const char *format, va_list ap);

    static unsigned tokenize(const std::string &s,
                             std::vector<std::string> &tokens,
                             const std::string &delims = DEFAULT_DELIMS,
                             bool allowEmpty = false);
    static unsigned tokenizeLine(std::istream &stream,
                                 std::vector<std::string> &tokens,
                                 const std::string &delims = DEFAULT_DELIMS,
                                 const std::string &lineDelims =
                                 DEFAULT_LINE_DELIMS,
                                 unsigned maxLength = 1024);

    static uint8_t parseU8(const std::string &s);
    static int8_t parseS8(const std::string &s);
    static uint16_t parseU16(const std::string &s);
    static int16_t parseS16(const std::string &s);
    static uint32_t parseU32(const std::string &s);
    static int32_t parseS32(const std::string &s);
    static uint64_t parseU64(const std::string &s);
    static int64_t parseS64(const std::string &s);
    static uint128_t parseU128(const std::string &s);
    static double parseDouble(const std::string &s);
    static float parseFloat(const std::string &s);
    static bool parseBool(const std::string &s);

    static std::string trimLeft(const std::string &s,
                                const std::string &delims = DEFAULT_DELIMS);
    static std::string trimRight(const std::string &s,
                                 const std::string &delims = DEFAULT_DELIMS);
    static std::string trim(const std::string &s,
                            const std::string &delims = DEFAULT_DELIMS);
    static std::string toUpper(const std::string &s);
    static std::string toLower(const std::string &s);
    static std::string capitalize(const std::string &s);
    static std::ostream &fill(std::ostream &stream, const std::string &str,
                              unsigned currentColumn = 0,
                              unsigned indent = 0,
                              unsigned maxColumn = 80);
    static std::string fill(const std::string &str,
                            unsigned currentColumn = 0,
                            unsigned indent = 0,
                            unsigned maxColumn = 80);

    static bool endsWith(const std::string &s, const std::string &part);
    static bool startsWith(const std::string &s, const std::string &part);
    static std::string bar(const std::string &title = "", unsigned width = 80,
                           const std::string &chars = "*");
    static std::string hexdump(const char *data, unsigned size);
    static std::string escapeRE(const std::string &s);
    static std::string escapeC(const std::string &s);
    static std::string unescapeC(const std::string &s);
    static std::string join(const std::vector<std::string> &s,
                            const std::string &delim = " ");

    /// Regular expression find
    static std::size_t find(const std::string &s, const std::string &pattern,
                            std::vector<std::string> *groups = 0);
    /// Replace a single character
    static std::string replace(const std::string &s, char search, char replace);
    /// Regular expression replace
    static std::string replace(const std::string &s, const std::string &search,
                               const std::string &replace);
    static std::string transcode(const std::string &s,
                                 const std::string &search,
                                 const std::string &replace);
  };
}
#endif // CBANG_STRING_H
