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

#ifndef CBANG_SCRIPT_BARE_MEMBER_FUNCTOR_H
#define CBANG_SCRIPT_BARE_MEMBER_FUNCTOR_H

#include "Function.h"

#include <cbang/util/MemberFunctor.h>


namespace cb {
  namespace Script {
    CBANG_MEMBER_FUNCTOR(BareMemberFunctorBase, Handler, void, evalCB);

    template <typename T>
    class BareMemberFunctor : public Function, public BareMemberFunctorBase<T> {
    public:
      BareMemberFunctor(const std::string &name, T *obj,
                    typename BareMemberFunctorBase<T>::member_t member,
                    unsigned minArgs = 0, unsigned maxArgs = 0,
                    const std::string &help = "",
                    const std::string &argHelp = "",
                    bool autoEvalArgs = true) :
        Function(name, minArgs, maxArgs, help, argHelp, autoEvalArgs),
        BareMemberFunctorBase<T>(obj, member) {}

      // From Handler
      bool eval(const Context &ctx) {
        BareMemberFunctorBase<T>::evalCB();
        return true;
      }
    };
  }
}

#endif // CBANG_SCRIPT_BARE_MEMBER_FUNCTOR_H

