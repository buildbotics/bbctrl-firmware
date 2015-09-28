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

#ifndef OPTION_ACTION_H
#define OPTION_ACTION_H

#include <cbang/util/MemberFunctor.h>


namespace cb {
  class Option;

  /// The non-templated base class of OptionAction.
  class OptionActionBase {
  public:
    virtual ~OptionActionBase() {}
    virtual int operator()(Option &option) = 0;
  };


  class BareOptionActionBase : public OptionActionBase {
  public:
    virtual int operator()() = 0;

    // From OptionActionBase
    int operator()(Option &option) {return operator()();}
  };


  /**
   * Used to execute an fuction when a command line option is encountered.
   * This makes it possible to load a configuration file in the middle of
   * command line processing.  Thus making the options overridable by
   * subsequent options.
   */
  CBANG_MEMBER_FUNCTOR(BareOptionAction, BareOptionActionBase, int, operator());
  CBANG_MEMBER_FUNCTOR1(OptionAction, OptionActionBase, int, operator(), \
                        Option &);
}

#endif // OPTION_ACTION_H
