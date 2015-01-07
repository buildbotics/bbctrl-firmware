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

#include "JSONHandler.h"
#include "Request.h"
#include "Buffer.h"
#include "BufferDevice.h"
#include "Headers.h"

#include <cbang/String.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace cb::Event;


bool JSONHandler::operator()(Request &req) {
  const URI &uri = req.getURI();

  // Setup JSON output
  Buffer output;
  BufferStream<> ostr(output);
  JSON::Writer writer(ostr, 0, !uri.has("pretty"),
                      uri.has("python_mode") ? JSON::Writer::PYTHON_MODE :
                      JSON::Writer::JSON_MODE);

  req.setContentType("application/json");

  try {
    // Parse JSON message
    JSON::ValuePtr msg;

    Headers hdrs = req.getInputHeaders();
    if (hdrs.hasContentType() &&
        String::startsWith(hdrs.getContentType(), "application/json")) {

      msg = req.getInputJSON();

    } else if (!uri.empty()) {
      msg = new JSON::Dict;

      for (URI::const_iterator it = uri.begin(); it != uri.end(); it++)
        msg->insert(it->first, it->second);
    }


    // Dispatch JSON call
    if (msg.isNull()) LOG_DEBUG(5, "JSON Call: " << uri.getPath() << "()");
    else LOG_DEBUG(5, "JSON Call: " << uri.getPath() << '(' << *msg << ')');

    (*this)(req, msg, writer);

    // Make sure JSON stream is complete
    writer.close();

    req.reply(output);

  } catch (const Exception &e) {
    LOG_ERROR(e);

    // Reset output buffer
    output.clear();
    writer.reset();

    // Send error message
    writer.beginList();
    writer.append("error");
    writer.append(e.getMessage());
    writer.endList();
    writer.close();

    req.reply(output);
  }

  return true;
}
