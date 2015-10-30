/******************************************************************************\

             This file is part of the Buildbotics Machine Controller.

                    Copyright (c) 2015-2016, Buildbotics LLC
                               All rights reserved.

        The Buildbotics Machine Controller is free software: you can
        redistribute it and/or modify it under the terms of the GNU
        General Public License as published by the Free Software
        Foundation, either version 2 of the License, or (at your option)
        any later version.

        The Buildbotics Webserver is distributed in the hope that it will
        be useful, but WITHOUT ANY WARRANTY; without even the implied
        warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
        PURPOSE.  See the GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this software.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#ifndef BBMC_APP_H
#define BBMC_APP_H

#include "Server.h"

#include <cbang/ServerApplication.h>
#include <cbang/net/IPAddress.h>

#include <cbang/event/Base.h>
#include <cbang/event/DNSBase.h>
#include <cbang/event/Client.h>

namespace cb {
  namespace Event {class Event;}
}


namespace bbmc {
  class App : public cb::ServerApplication {
    cb::Event::Base base;
    cb::Event::DNSBase dns;
    cb::Event::Client client;

    Server server;

  public:
    App();

    static bool _hasFeature(int feature);

    cb::Event::Base &getEventBase() {return base;}
    cb::Event::DNSBase &getEventDNS() {return dns;}
    cb::Event::Client &getEventClient() {return client;}

    Server &getServer() {return server;}

    // From cb::Application
    int init(int argc, char *argv[]);
    void run();

    void lifelineEvent(cb::Event::Event &e, int signal, unsigned flags);
    void signalEvent(cb::Event::Event &e, int signal, unsigned flags);
  };
}

#endif // BBMC_APP_H

