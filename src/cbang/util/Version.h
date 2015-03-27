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

#ifndef CB_VERSION_H
#define CB_VERSION_H

#include <cbang/Exception.h>
#include <cbang/StdTypes.h>
#include <cbang/SStream.h>
#include <cbang/String.h>
#include <cbang/geom/Vector.h>

#include <ostream>
#include <vector>


namespace cb {
  // TODO Remove the dependency on Vector
  template <typename T = uint8_t>
  class VersionBase : public Vector<3, T> {
    using Vector<3, T>::data;

  public:
    VersionBase(T verMajor = 0, T verMinor = 0, T verRevision = 0) :
      Vector<3, T>(verMajor, verMinor, verRevision) {}

    VersionBase(const std::string &s) {
      if (s.find_first_not_of("1234567890. ") != std::string::npos)
        CBANG_THROWS("Invalid character in version string: "
                     << String::hexdump(s));

      std::vector<std::string> parts;
      String::tokenize(s, parts, ".");

      if (parts.empty() || 3 < parts.size())
        CBANG_THROWS("Error parsing version string: '" << s << "'");

      setMajor(parsePart(parts[0]));
      if (1 < parts.size()) setMinor(parsePart(parts[1]));
      if (2 < parts.size()) setRevision(parsePart(parts[2]));
    }


    void setMajor(T major) {data[0] = major;}
    void setMinor(T minor) {data[1] = minor;}
    void setRevision(T rev) {data[2] = rev;}

    T getMajor() const {return data[0];}
    T getMinor() const {return data[1];}
    T getRevision() const {return data[2];}

    T &getMajor() {return data[0];}
    T &getMinor() {return data[1];}
    T &getRevision() {return data[2];}

    int compare(const VersionBase<T> &v) const {
      if (getMajor() != v.getMajor()) return getMajor() - v.getMajor();
      if (getMinor() != v.getMinor()) return getMinor() - v.getMinor();
      return getRevision() - v.getRevision();
    }

    bool operator<(const VersionBase<T> &v) const {return compare(v) < 0;}
    bool operator<=(const VersionBase<T> &v) const {return compare(v) <= 0;}
    bool operator>(const VersionBase<T> &v) const {return compare(v) > 0;}
    bool operator>=(const VersionBase<T> &v) const {return compare(v) >= 0;}
    bool operator==(const VersionBase<T> &v) const {return compare(v) == 0;}
    bool operator!=(const VersionBase<T> &v) const {return compare(v) != 0;}

    operator std::string () const {return toString();}

    std::string toString(bool trimRev = true) const {
      std::string s = String(getMajor()) + "." + String(getMinor());
      if (!trimRev || getRevision()) s += "." + String(getRevision());
      return s;
    }


    static T parsePart(const std::string &part) {
      if (part.empty()) CBANG_THROW("Invalid version string, part is empty");
      if (part.find_first_not_of("0") == std::string::npos) return 0;
      return String::parse<T>(String::trimLeft(part, "0"));
    }


    static SmartPointer<VersionBase<T> > parse(const std::string &s)
    {return new VersionBase<T>(s);}
  };


  template <typename T> inline static
  std::ostream &operator<<(std::ostream &stream, const VersionBase<T> &v) {
    return stream << v.toString();
  }


  typedef VersionBase<uint8_t> VersionU8;
  typedef VersionBase<uint16_t> VersionU16;


  class Version : public VersionU8 {
  public:
    Version(uint8_t verMajor = 0, uint8_t verMinor = 0,
            uint8_t verRevision = 0) :
      VersionU8(verMajor, verMinor, verRevision) {}

    Version(uint32_t v):
      VersionU8((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff) {}

    Version(const std::string &s) : VersionU8(s) {}

    operator uint32_t () const {
      return ((uint32_t)getMajor() << 16) | ((uint32_t)getMinor() << 8) |
        getRevision();
    }


    static SmartPointer<Version> parse(const std::string &s)
    {return new Version(s);}
  };
}

#endif // CB_VERSION_H

