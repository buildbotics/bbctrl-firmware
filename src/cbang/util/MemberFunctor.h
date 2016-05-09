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

#ifndef CBANG_MEMBER_FUNCTOR_H
#define CBANG_MEMBER_FUNCTOR_H

#include "Functor.h"


#define CBANG_MF(CLASS, BASE, RETURN, CALLBACK, CONST, ARGS, PARAMS)    \
  template <typename T>                                                 \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef RETURN (T::*member_t)(ARGS) CONST;                          \
  protected:                                                            \
    T *obj;                                                             \
    member_t member;                                                    \
  public:                                                               \
    CLASS(T *obj, member_t member) : obj(obj), member(member) {}        \
    RETURN CALLBACK(ARGS) CONST {return (*obj.*member)(PARAMS);}        \
  }


#define CBANG_MEMBER_FUNCTOR(CLASS, PARENT, RETURN, CALLBACK)   \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, , ,)

#define CBANG_MEMBER_FUNCTOR1(CLASS, PARENT, RETURN, CALLBACK, ARG1)    \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, ,                           \
           CBANG_ARGS1(ARG1), CBANG_PARAMS1)

#define CBANG_MEMBER_FUNCTOR2(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2) \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, ,                           \
           CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2)

#define CBANG_MEMBER_FUNCTOR3(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3)                                     \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, ,                           \
           CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3)

#define CBANG_MEMBER_FUNCTOR4(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3, ARG4)                               \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, ,                           \
           CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4)

#define CBANG_MEMBER_FUNCTOR5(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                              ARG3, ARG4, ARG5)                         \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, ,                           \
           CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5)


#define CBANG_CONST_MEMBER_FUNCTOR(CLASS, PARENT, RETURN, CALLBACK) \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const, ,)

#define CBANG_CONST_MEMBER_FUNCTOR1(CLASS, PARENT, RETURN, CALLBACK, ARG1) \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const,                      \
           CBANG_ARGS1(ARG1), CBANG_PARAMS1)

#define CBANG_CONST_MEMBER_FUNCTOR2(CLASS, PARENT, RETURN, CALLBACK, ARG1, \
                                    ARG2)                               \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const,                      \
           CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2)

#define CBANG_CONST_MEMBER_FUNCTOR3(CLASS, PARENT, RETURN, CALLBACK, ARG1, \
                                    ARG2, ARG3)                         \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const,                      \
           CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3)

#define CBANG_CONST_MEMBER_FUNCTOR4(CLASS, PARENT, RETURN, CALLBACK, ARG1, \
                                    ARG2, ARG3, ARG4)                   \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const,                      \
           CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4)

#define CBANG_CONST_MEMBER_FUNCTOR5(CLASS, PARENT, RETURN, CALLBACK, ARG1, \
                                    ARG2, ARG3, ARG4, ARG5)             \
  CBANG_MF(CLASS, PARENT, RETURN, CALLBACK, const,                      \
           CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5)


#include "Callback.h"


namespace cb {
  CBANG_CONST_MEMBER_FUNCTOR(MemberFunctor, Callback, bool, operator());
}

#endif // CBANG_MEMBER_FUNCTOR_H
