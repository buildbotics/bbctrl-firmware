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

#ifndef CBANG_STRING_MAP_H
#define CBANG_STRING_MAP_H

#include <cbang/Exception.h>

#include <string>
#include <cctype>
#include <map>
#include <iostream>


namespace cb {
  template <typename LESS_T = std::less<std::string> >
  class StringMapBase : public std::map<std::string, std::string, LESS_T> {
  public:
    typedef std::map<std::string, std::string, LESS_T> Super_T;
    typedef typename Super_T::iterator iterator;
    typedef typename Super_T::const_iterator const_iterator;
    typedef typename Super_T::value_type value_type;
    using Super_T::find;
    using Super_T::end;
    using Super_T::insert;


    void set(const std::string &key, const std::string &value) {
      std::pair<iterator, bool> result = insert(value_type(key, value));
      if (!result.second) result.first->second = value;
    }


    void unset(const std::string &key) {Super_T::erase(key);}


    void append(const std::string &key, const std::string &value) {
      iterator it = find(key);
      if (it == end()) set(key, value);
      else it->second += value;
    }


    const std::string &get(const std::string &key) const {
      const_iterator it = find(key);
      if (it == end()) CBANG_THROWS("'" << key << "' not set");
      return it->second;
    }


    const std::string &get(const std::string &key,
                           const std::string &defaultValue) const {
      const_iterator it = Super_T::find(key);
      return it == end() ? defaultValue : it->second;
    }


    using Super_T::operator[];
    const std::string &operator[](const std::string &key) const
    {return get(key);}


    bool has(const std::string &key) const {return find(key) != end();}
  };


  /// Case-insensitive string compare
  struct string_ci_less {
    bool operator()(const std::string &s1, const std::string &s2) const {
      // This is longwinded but avoids making copies of the strings
      std::string::const_iterator it1 = s1.begin();
      std::string::const_iterator it2 = s2.begin();

      while (true) {
        if (it1 == s1.end()) return it2 != s2.end();
        if (it2 == s2.end()) return false;
        char c1 = std::tolower(*it1++);
        char c2 = std::tolower(*it2++);
        if (c1 < c2) return true;
        if (c2 < c1) return false;
      }
    }
  };


  typedef StringMapBase<> StringMap;
  typedef StringMapBase<string_ci_less> CIStringMap;
}

#endif // CBANG_STRING_MAP_H
