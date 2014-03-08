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

#ifndef OPTION_ACTION_H
#define OPTION_ACTION_H

#include <string>
#include <vector>

namespace cb {
  class Option;

  /// The non-templated base class of OptionAction.
  class OptionActionBase {
  public:
    virtual ~OptionActionBase() {}
    virtual int operator()(Option &option) = 0;
  };

  /**
   * Used to execute an fuction when a command line option is encountered.
   * This makes it possible to load a configuration file in the middle of
   * command line processing.  Thus making the options overridable by
   * subsequent options.
   */
  template <typename T>
  class OptionAction : public OptionActionBase {
    T *obj;
    typedef int (T::*fpt_t)(Option &option);
    fpt_t fpt;

    typedef int (T::*fpt_noargs_t)();
    fpt_noargs_t fpt_noargs;

  public:
    OptionAction(T *obj, fpt_t fpt) : obj(obj), fpt(fpt), fpt_noargs(0) {}
    OptionAction(T *obj, fpt_noargs_t fpt_noargs) :
      obj(obj), fpt(0), fpt_noargs(fpt_noargs) {}

    virtual ~OptionAction() {}

    virtual int operator()(Option &option) {
      if (fpt) return (*obj.*fpt)(option);
      else return (*obj.*fpt_noargs)();
    }
  };
}

#endif // OPTION_ACTION_H
