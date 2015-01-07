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

#include "Context.h"

#include <cbang/log/Logger.h>
#include <cbang/xml/XMLWriter.h>

using namespace std;
using namespace cb::HTTP;


void Context::errorPage(StatusCode status, const string &message) const {
  Connection &con = getConnection();
  Response &response = con.getResponse();

  response.setStatus(status);
  response.setContentType("text/html");

  string err =
    SSTR((unsigned)status << ' '
         << String::replace(status.toString(), '_', ' ')
         << ' ' << con.getRequest().getURI());

  if (!message.empty()) err += string(": ") + message;

  XMLWriter writer(con);
  writer.simpleElement("h1", err);
  con << flush;

  LOG_WARNING(con << ':' << err);
}
