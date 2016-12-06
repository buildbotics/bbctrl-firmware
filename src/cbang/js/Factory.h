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

#pragma once

#include "Value.h"
#include "Function.h"

#include <cbang/StdTypes.h>


namespace cb {
  namespace js {
    class Factory {
    public:
      virtual ~Factory() {}

      virtual SmartPointer<Value> create(const std::string &value) = 0;
      virtual SmartPointer<Value>
      create(const char *value, unsigned length = -1) {
        return create(length < 0 ? std::string(value) :
                               std::string(value, length));
      }
      virtual SmartPointer<Value> create(double value) = 0;
      virtual SmartPointer<Value> create(float value)
      {return create((double)value);}
      virtual SmartPointer<Value> create(int64_t value)
      {return create((double)value);}
      virtual SmartPointer<Value> create(uint64_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(int32_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(uint32_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(int16_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(uint16_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(int8_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(uint8_t value)
      {return create((int64_t)value);}
      virtual SmartPointer<Value> create(const Function &func) = 0;
      virtual SmartPointer<Value> createArray(unsigned size = 0) = 0;
      virtual SmartPointer<Value> createObject() = 0;
      virtual SmartPointer<Value> createBoolean(bool value) = 0;
      virtual SmartPointer<Value> createUndefined() = 0;
      virtual SmartPointer<Value> createNull() = 0;
    };
  }
}
