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

#ifndef CBANG_OPTION_ACTION_SET_H
#define CBANG_OPTION_ACTION_SET_H

#include "OptionAction.h"
#include "Option.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

namespace cb {
  template <typename T>
  class OptionActionSet : public OptionActionBase {
    T &ref;

  public:
    OptionActionSet(T &ref) : ref(ref) {}

    virtual int operator()(Option &option) {
      set(option.toString());
      return 0;
    }

    void set(const std::string &value) {
      ref = T::parse(value);
    }
  };

  template <>
  inline void OptionActionSet<std::string>::set(const std::string &value) {
    ref = value;
  }

  template <>
  inline void OptionActionSet<uint8_t>::set(const std::string &value) {
    ref = String::parseU8(value);
  }

  template <>
  inline void OptionActionSet<int8_t>::set(const std::string &value) {
    ref = String::parseS8(value);
  }

  template <>
  inline void OptionActionSet<uint16_t>::set(const std::string &value) {
    ref = String::parseU16(value);
  }

  template <>
  inline void OptionActionSet<int16_t>::set(const std::string &value) {
    ref = String::parseS16(value);
  }

  template <>
  inline void OptionActionSet<uint32_t>::set(const std::string &value) {
    ref = String::parseU32(value);
  }

  template <>
  inline void OptionActionSet<int32_t>::set(const std::string &value) {
    ref = String::parseS32(value);
  }

  template <>
  inline void OptionActionSet<uint64_t>::set(const std::string &value) {
    ref = String::parseU64(value);
  }

  template <>
  inline void OptionActionSet<int64_t>::set(const std::string &value) {
    ref = String::parseS64(value);
  }

  template <>
  inline void OptionActionSet<uint128_t>::set(const std::string &value) {
    ref = String::parseU128(value);
  }

  template <>
  inline void OptionActionSet<double>::set(const std::string &value) {
    ref = String::parseDouble(value);
  }

  template <>
  inline void OptionActionSet<float>::set(const std::string &value) {
    ref = String::parseFloat(value);
  }

  template <>
  inline void OptionActionSet<bool>::set(const std::string &value) {
    ref = String::parseBool(value);
  }
}

#endif // CBANG_OPTION_ACTION_SET_H
