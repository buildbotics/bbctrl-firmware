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

#ifndef CB_EVENT_DNSCALLBACK_H
#define CB_EVENT_DNSCALLBACK_H

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>
#include <cbang/util/MemberFunctor.h>

#include <vector>


namespace cb {
  namespace Event {
    class DNSCallback {
      IPAddress source;

    public:
      SmartPointer<DNSCallback> self;

      virtual ~DNSCallback() {}

      void setSource(const IPAddress &source) {this->source = source;}
      const IPAddress &getSource() const {return source;}

      virtual void operator()(int error, std::vector<IPAddress> &addrs,
                              int ttl) = 0;
    };


    CBANG_FUNCTOR3(DNSFunctor, DNSCallback, void, operator(), int,
                   std::vector<IPAddress> &, int);
    CBANG_MEMBER_FUNCTOR3(DNSMemberFunctor, DNSCallback, void, operator(), int,
                          std::vector<IPAddress> &, int);
  }
}

#endif // CB_EVENT_DNSCALLBACK_H

