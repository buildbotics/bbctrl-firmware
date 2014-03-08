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

#ifndef DEFAULT_CATCH_H
#define DEFAULT_CATCH_H

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#ifdef DEBUG
#define CBANG_CATCH_LOCATION << "\nCaught at: " << CBANG_FILE_LOCATION
#else // DEBUG
#define CBANG_CATCH_LOCATION
#endif // DEBUG


#define CBANG_CATCH_CBANG(LOG, MSG)                                     \
  catch (const cb::Exception &e) {                                      \
    LOG("Exception" << MSG << ": " << e CBANG_CATCH_LOCATION);          \
  }

#define CBANG_CATCH_STD(LOG, MSG)                                   \
  catch (const std::exception &e) {                                 \
    LOG("std::exception" << MSG << ": " << e.what()                 \
        CBANG_CATCH_LOCATION);                                      \
  }

#define CBANG_CATCH_UNKNOWN(LOG, MSG)                               \
  catch (...) {                                                     \
    LOG("unknown exception" << MSG CBANG_CATCH_LOCATION);           \
  }


#define CBANG_CATCH_ALL(LOG, MSG)                                   \
  CBANG_CATCH_CBANG(LOG, MSG)                                       \
  CBANG_CATCH_STD(LOG, MSG)                                         \
  CBANG_CATCH_UNKNOWN(LOG, MSG)

#define CBANG_CATCH_CS(LOG, MSG)                                    \
  CBANG_CATCH_CBANG(LOG, MSG)                                       \
  CBANG_CATCH_STD(LOG, MSG)


#ifdef DEBUG
#define CBANG_CATCH(LOG, MSG) CBANG_CATCH_CBANG(LOG, MSG)
#else // DEBUG
#define CBANG_CATCH(LOG, MSG) CBANG_CATCH_ALL(LOG, MSG)
#endif // DEBUG

#define CBANG_TRY_CATCH(LOG, EXPR, MSG) try {EXPR;} CBANG_CATCH(LOG, MSG)

#define CBANG_CATCH_ERROR CBANG_CATCH(CBANG_LOG_ERROR, "")
#define CBANG_CATCH_WARNING CBANG_CATCH(CBANG_LOG_WARNING, "")

#define CBANG_TRY_CATCH_ERROR(EXPR) CBANG_TRY_CATCH(CBANG_LOG_ERROR, EXPR, "")
#define CBANG_TRY_CATCH_WARNING(EXPR) \
  CBANG_TRY_CATCH(CBANG_LOG_WARNING, EXPR, "")

#ifdef USING_CBANG
#define CATCH(LOG, MSG) CBANG_CATCH(LOG, MSG)
#define CATCH_ERROR CBANG_CATCH_ERROR
#define CATCH_WARNING CBANG_CATCH_WARNING
#define TRY_CATCH(LOG, EXPR, MSG) CBANG_TRY_CATCH(LOG, EXPR, MSG)
#define TRY_CATCH_ERROR(EXPR) CBANG_TRY_CATCH_ERROR(EXPR)
#define TRY_CATCH_WARNING(EXPR) CBANG_TRY_CATCH_WARNING(EXPR)
#endif // USING_CBANG

#endif // DEFAULT_CATCH_H
