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

#ifndef CBANG_OPTION_PROXY_H
#define CBANG_OPTION_PROXY_H

#include <cbang/config/Options.h>

namespace cb {
  /**
   * This class is used to allow different sets of configuration options
   * to be set in different contexts.  Such as different project runs.
   * Options which are not "localize" are defered to the parent Options
   * class.
   *
   * Options set here shadow options set in the Options class
   * passed at construction.
   */
  class OptionProxy : public Options {
    const Options &options;

  public:
    OptionProxy(const Options &options) : options(options) {}

    // From OptionMap
    bool has(const std::string &key) const;
    /// @return True if the options is local to this proxy.
    bool local(const std::string &key) const;
    /// Make the option local to this proxy.
    const SmartPointer<Option> &localize(const std::string &key);
    /// Get an option.  Possibly defering to the parent Options.
    const SmartPointer<Option> &get(const std::string &key) const;
  };
}

#endif // CBANG_OPTION_PROXY_H
