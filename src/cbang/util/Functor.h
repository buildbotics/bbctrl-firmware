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

#ifndef CBANG_FUNCTOR_H
#define CBANG_FUNCTOR_H


#define CBANG_FUNC(CLASS, BASE, RETURN, CALLBACK, CONST, ARGS, PARAMS)  \
  class CLASS : public BASE {                                           \
  public:                                                               \
    typedef RETURN (*func_t)(ARGS) CONST;                               \
  protected:                                                            \
    func_t func;                                                        \
  public:                                                               \
    CLASS(func_t func) : func(func) {}                                  \
    RETURN CALLBACK(ARGS) CONST {return (*func)(PARAMS);}               \
  }


#define CBANG_ARGS1(ARG1) ARG1 arg1
#define CBANG_ARGS2(ARG1, ARG2) ARG1 arg1, ARG2 arg2
#define CBANG_ARGS3(ARG1, ARG2, ARG3) ARG1 arg1, ARG2 arg2, ARG3 arg3
#define CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4) \
  ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4
#define CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5) \
  ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5


#define CBANG_PARAMS1 arg1
#define CBANG_PARAMS2 arg1, arg2
#define CBANG_PARAMS3 arg1, arg2, arg3
#define CBANG_PARAMS4 arg1, arg2, arg3, arg4
#define CBANG_PARAMS5 arg1, arg3, arg3, arg4, arg5


#define CBANG_FUNCTOR(CLASS, PARENT, RETURN, CALLBACK)  \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, , ,)

#define CBANG_FUNCTOR1(CLASS, PARENT, RETURN, CALLBACK, ARG1)   \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, ,                 \
             CBANG_ARGS1(ARG1), CBANG_PARAMS1)

#define CBANG_FUNCTOR2(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2) \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, ,                     \
             CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2)

#define CBANG_FUNCTOR3(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, ARG3) \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, ,                         \
             CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3)

#define CBANG_FUNCTOR4(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, ARG3, \
                       ARG4)                                            \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, ,                         \
             CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4)

#define CBANG_FUNCTOR5(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2,     \
                       ARG3, ARG4, ARG5)                                \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, ,                         \
             CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5)


#define CBANG_CONST_FUNCTOR(CLASS, PARENT, RETURN, CALLBACK)    \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const, ,)

#define CBANG_CONST_FUNCTOR1(CLASS, PARENT, RETURN, CALLBACK, ARG1) \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const,                \
             CBANG_ARGS1(ARG1), CBANG_PARAMS1)

#define CBANG_CONST_FUNCTOR2(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2) \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const,                    \
             CBANG_ARGS2(ARG1, ARG2), CBANG_PARAMS2)

#define CBANG_CONST_FUNCTOR3(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                             ARG3)                                      \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const,                    \
             CBANG_ARGS3(ARG1, ARG2, ARG3), CBANG_PARAMS3)

#define CBANG_CONST_FUNCTOR4(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                             ARG3, ARG4)                                \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const,                    \
             CBANG_ARGS4(ARG1, ARG2, ARG3, ARG4), CBANG_PARAMS4)

#define CBANG_CONST_FUNCTOR5(CLASS, PARENT, RETURN, CALLBACK, ARG1, ARG2, \
                             ARG3, ARG4, ARG5)                          \
  CBANG_FUNC(CLASS, PARENT, RETURN, CALLBACK, const,                    \
             CBANG_ARGS5(ARG1, ARG2, ARG3, ARG4, ARG5), CBANG_PARAMS5)


#endif // CBANG_FUNCTOR_H

