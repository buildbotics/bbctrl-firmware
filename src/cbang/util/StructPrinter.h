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

#ifndef CBANG_STRUCT_PRINTER_H
#define CBANG_STRUCT_PRINTER_H

#include <cbang/xml/XMLWriter.h>

#include <string>
#include <ostream>
#include <iomanip>

namespace cb {
  template<typename T>
  class StructPrinter {
    const T &s;

  public:
    StructPrinter(const T &s) : s(s) {}
    virtual ~StructPrinter() {}

    static const char *getStructName() {return T::Struct::getStructName();}

    static unsigned getStructNameWidth()
    {return T::Struct::getStructNameWidth();}

    static unsigned getMemberCount() {return T::Struct::getMemberCount();}
    static unsigned getRawMemberCount() {return T::Struct::getRawMemberCount();}

    static bool skipMember(unsigned index) {return false;}

    static const char *getMemberName(unsigned index)
    {return T::Struct::getMemberName(index);}

    virtual std::string toString(unsigned index) const
    {return s.getStruct().toString(index);}

    virtual std::string getHTMLClass(unsigned index) const
    {return s.getStruct().getMemberName(index);}

    virtual std::ostream &print(std::ostream &stream, unsigned index) const
    {return s.getStruct().print(stream, index);}

    void writeHTML(XMLWriter &writer, unsigned index) const {
      writer.text(s.toString(index));
    }


    static void writeHTMLRowHeader(XMLWriter &writer) {
      XMLAttributes attrs;

      writer.startElement("tr");

      for (unsigned i = 0; i < T::getMemberCount(); i++) {
        if (T::skipMember(i)) continue;
        attrs["class"] = T::getMemberName(i);
        writer.simpleElement("th", T::getMemberName(i), attrs);
      }

      writer.endElement("tr");
    }


    void writeHTMLRow(XMLWriter &writer) const {
      XMLAttributes attrs;

      writer.startElement("tr");

      for (unsigned i = 0; i < T::getMemberCount(); i++) {
        if (T::skipMember(i)) continue;
        attrs["class"] = s.getHTMLClass(i);
        writer.startElement("td", attrs);
        s.writeHTML(writer, i);
        writer.endElement("td");
      }

      writer.endElement("tr");
    }


    void writeHTMLTable(XMLWriter &writer) const {
      XMLAttributes attrs;

      attrs["class"] = T::getStructName();
      writer.startElement("table", attrs);

      for (unsigned i = 0; i < T::getMemberCount(); i++) {
        if (T::skipMember(i)) continue;

        writer.startElement("tr");

        attrs["class"] = T::getMemberName(i);
        writer.simpleElement("th", T::getMemberName(i), attrs);

        attrs["class"] = s.getHTMLClass(i);
        writer.startElement("td", attrs);
        s.writeHTML(writer, i);
        writer.endElement("td");

        writer.endElement("tr");
      }

      writer.endElement("table");
    }


    std::ostream &print(std::ostream &stream) const {
      for (unsigned i = 0; i < T::getMemberCount(); i++) {
        if (T::skipMember(i)) continue;

        stream << std::setw(T::getStructNameWidth())
               << T::getMemberName(i) << " ";
        s.print(stream, i) << '\n';
      }

      return stream;
    }

    static void makeLink(XMLWriter &writer, const std::string &href,
                         const std::string &text) {
      XMLAttributes attrs;
      attrs["href"] = href;
      writer.simpleElement("a", text, attrs);
    }
  };

  template<typename T> inline static
  std::ostream &operator<<(std::ostream &stream, const StructPrinter<T> &p) {
    return p.print(stream);
  }
}

#endif // CBANG_STRUCT_PRINTER_H

