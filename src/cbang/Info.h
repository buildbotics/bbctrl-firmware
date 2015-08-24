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

#ifndef CBANG_INFO_H
#define CBANG_INFO_H

#include <cbang/util/Singleton.h>

#include <string>
#include <ostream>
#include <map>
#include <list>


namespace cb {
  namespace JSON {class List; class Sink;}

  class XMLWriter;

  class Info : public Singleton<Info> {
    // TODO Could probably replace this with cb::OrderedDict<>
    template <typename Key, typename Value>
    class ordered_map :
      public std::map<Key, Value>,
      public std::list<typename std::map<Key, Value>::value_type *> {
    public:
      typedef std::map<Key, Value> map_t;
      typedef std::list<typename map_t::value_type *> list_t;

      typedef typename list_t::iterator iterator;
      typedef typename list_t::const_iterator const_iterator;
      typedef typename map_t::iterator map_iterator;
      typedef typename map_t::const_iterator const_map_iterator;
      typedef typename map_t::value_type value_type;

      using list_t::begin;
      using list_t::end;

      map_iterator map_begin() {return map_t::begin();}
      const_map_iterator map_begin() const {return map_t::begin();}
      map_iterator map_end() {return map_t::end();}
      const_map_iterator map_end() const {return map_t::end();}

      using map_t::insert;
    };

    typedef ordered_map<std::string, std::string> category_t;
    typedef ordered_map<std::string, category_t> categories_t;

    unsigned maxKeyLength;
    categories_t categories;

  public:
    Info(Inaccessible) : maxKeyLength(0) {}

    void add(const std::string &category, const std::string &key,
             const std::string &value, bool prepend = false);
    const std::string &get(const std::string &category,
                           const std::string &key) const;
    bool has(const std::string &category, const std::string &key) const;

    std::ostream &print(std::ostream &stream, unsigned width = 80,
                        bool wrap = true) const;
    void write(XMLWriter &writer) const;
    SmartPointer<JSON::List> getJSONList() const;
    void write(JSON::Sink &sink) const;
  };

  inline static
  std::ostream &operator<<(std::ostream &stream, const Info &info) {
    return info.print(stream);
  }
}

#endif // CBANG_INFO_H

