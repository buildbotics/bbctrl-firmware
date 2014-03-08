/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2012, Cauldron Development LLC
                 Copyright (c) 2003-2012, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2 of
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

#include <cbang/security/Cipher.h>
#include <cbang/iostream/CipherStream.h>
#include <cbang/os/SystemUtilities.h>

#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>

namespace io = boost::iostreams;

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <key>" << endl;
    return 1;
  }

  const unsigned char *key = (unsigned char *)argv[1];

  io::filtering_istream in;
  CipherStream encrypt(new Cipher("bf-cbc", true, key));

  in.push(encrypt);
  in.push(cin);

  io::filtering_ostream out;
  CipherStream decrypt(new Cipher("bf-cbc", false, key));

  out.push(decrypt);
  out.push(cout);

  SystemUtilities::cp(in, out);
  out.flush(); // Flush

  return 0;
}
