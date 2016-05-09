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

#ifndef CBANG_ORDERED_DICT_H
#define CBANG_ORDERED_DICT_H

#include <map>
#include <vector>
#include <string>

#include <cbang/Exception.h>

namespace cb {
  template <typename T, typename KEY = std::string>
  class OrderedDict : protected std::vector<std::pair<KEY, T> > {
    typedef T type_t;
    typedef std::vector<std::pair<KEY, type_t> > vector_t;
    typedef std::map<KEY, typename vector_t::size_type> dict_t;
    dict_t dict;

  public:
    void clear() {
      vector_t::clear();
      dict.clear();
    }


    typedef typename vector_t::size_type size_type;
    using vector_t::empty;
    using vector_t::size;

    typedef typename vector_t::const_iterator iterator;
    typedef typename vector_t::const_iterator const_iterator;
    iterator begin() const {return vector_t::begin();}
    iterator end() const {return vector_t::end();}


    void update(const OrderedDict<T> &o) {
      for (iterator it = o.begin(); it != o.end(); it++)
        insert(it->first, it->second);
    }


    int lookup(const KEY &key) const {
      typename dict_t::const_iterator it = dict.find(key);
      return it == dict.end() ? -1 : it->second;
    }


     size_type indexOf(const KEY &key) const {
      typename dict_t::const_iterator it = dict.find(key);
      if (it == dict.end()) CBANG_THROWS("Key '" << key << "' not found");
      return it->second;
    }


    const KEY &keyAt(size_type i) const {
      if (size() <= i) CBANG_THROWS("Index " << i << " out of range");
      return this->at(i).first;
    }


    bool has(const KEY &key) const {
      return dict.find(key) != dict.end();
    }


    const typename OrderedDict::type_t &
    get(size_type i) const {
      if (size() <= i) CBANG_THROWS("Index " << i << " out of range");
      return this->at(i).second;
    }


    typename OrderedDict::type_t &
    get(size_type i) {
      if (size() <= i) CBANG_THROWS("Index " << i << " out of range");
      return this->at(i).second;
    }


    const typename OrderedDict::type_t &
    get(size_type i, const typename OrderedDict::type_t &defaultValue) const {
      if (size() <= i) return defaultValue;
      return this->at(i).second;
    }


    const typename OrderedDict::type_t &
    get(const KEY &key) const {return this->at(indexOf(key)).second;}


    typename OrderedDict::type_t &
    get(const KEY &key) {return this->at(indexOf(key)).second;}


    const typename OrderedDict::type_t &
    get(const KEY &key,
        const typename OrderedDict::type_t &defaultValue) const {
      typename dict_t::const_iterator it = dict.find(key);
      if (it == dict.end() || size() <= it->second)
        return defaultValue;
      return this->at(it->second).second;
    }


    size_type insert(const KEY &key, const type_t &value) {
      typename dict_t::const_iterator it = dict.find(key);
      if (it == dict.end()) {
        dict.insert(typename dict_t::value_type(key, size()));
        vector_t::push_back(typename vector_t::value_type(key, value));

        return size() - 1;
      }

      this->at(it->second) = typename vector_t::value_type(key, value);

      return it->second;
    }


    typename OrderedDict::type_t &
    operator[](size_type i) {
      if (size() <= i) CBANG_THROWS("Index " << i << " out of range");
      return this->at(i).second;
    }


    const typename OrderedDict::type_t &
    operator[](size_type i) const {return get(i);}


    typename OrderedDict::type_t &operator[](const KEY &key) {
      typename dict_t::iterator it = dict.find(key);
      if (it == dict.end()) {
        dict[key] = size();
        vector_t::push_back(typename vector_t::value_type(key, T()));
        return vector_t::back().second;
      }

      return this->at(it->second).second;
    }


    const typename OrderedDict::type_t &
    operator[](const KEY &key) const {return get(key);}


    // NOTE: There is no erase() because it would not be constant time with out
    // using std::list instead of std::vector but then get(size_type i) would
    // no longer be constant time.
  };
}

#endif // CBANG_ORDERED_DICT_H
