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

#ifndef CBANG_TRIANGLE_H
#define CBANG_TRIANGLE_H

#include "Vector.h"

namespace cb {
  typedef Vector<3, Vector2I> Triangle2I;
  typedef Vector<3, Vector2U> Triangle2U;
  typedef Vector<3, Vector2D> Triangle2D;
  typedef Vector<3, Vector2F> Triangle2F;

  typedef Vector<3, Vector3I> Triangle3I;
  typedef Vector<3, Vector3U> Triangle3U;
  typedef Vector<3, Vector3D> Triangle3D;
  typedef Vector<3, Vector3F> Triangle3F;
}

#endif // CBANG_TRIANGLE_H
