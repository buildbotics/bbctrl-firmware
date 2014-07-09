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

#include <cbang/net/IPRangeSet.h>

#include <cbang/String.h>
#include <cbang/Zap.h>
#include <cbang/json/Sync.h>

using namespace std;
using namespace cb;


IPAddressRange IPRangeSet::get(unsigned i) const {
  return IPAddressRange(rangeSet.at(i * 2), rangeSet.at(i * 2 + 1));
}


void IPRangeSet::insert(const string &spec) {
  vector<string> tokens;
  String::tokenize(spec, tokens, " \r\n\t,;");

  for (unsigned i = 0; i < tokens.size(); i++)
    insert(IPAddressRange(tokens[i]));
}


void IPRangeSet::insert(const IPAddressRange &range) {
  uint32_t start = range.getStart();
  uint32_t end = range.getEnd();

  if (end < start) swap(start, end);

  unsigned startPos = find(start);
  unsigned endPos = find(end);

  // Which parts fall inside a range
  bool startInside = startPos & 1;
  bool endInside = endPos & 1;

  int sizeChange = -((endPos - startPos) & ~(unsigned)1);
  if (!startInside && !endInside) sizeChange += 2;

  shift(sizeChange, endPos);
  endPos += sizeChange;

  if (!startInside) rangeSet[startPos] = start;
  if (!endInside) rangeSet[endPos - 1] = end;
}


void IPRangeSet::erase(const string &spec) {
  vector<string> tokens;
  String::tokenize(spec, tokens, " \r\n\t,;");

  for (unsigned i = 0; i < tokens.size(); i++)
    erase(IPAddressRange(tokens[i]));
}


void IPRangeSet::erase(const IPAddressRange &range) {
  uint32_t start = range.getStart();
  uint32_t end = range.getEnd();

  if (end < start) swap(start, end);

  unsigned startPos = find(start);
  unsigned endPos = find(end);

  bool startInside = startPos & 1;
  bool endInside = endPos & 1;

  if (startInside && rangeSet[startPos - 1] == start) {
    startInside = false;
    startPos--;
  }

  if (endInside && rangeSet[endPos] == end) {
    endInside = false;
    endPos++;
  }

  int sizeChange = -((endPos - startPos) & ~(unsigned)1);
  if (startInside && endInside) sizeChange += 2;

  shift(sizeChange, endPos);
  endPos += sizeChange;

  if (startInside) rangeSet[startPos] = start - 1;
  if (endInside) rangeSet[endPos - 1] = end + 1;
}


void IPRangeSet::print(ostream &stream) const {
  for (unsigned i = 0; i < rangeSet.size(); i += 2) {
    if (i) stream << ' ';
    stream << IPAddress(rangeSet[i]) << '-' << IPAddress(rangeSet[i + 1]);
  }
}


void IPRangeSet::write(JSON::Sync &sync) const {
  sync.beginList();

  for (unsigned i = 0; i < rangeSet.size(); i += 2)
    if (rangeSet[i] == rangeSet[i + 1])
      sync.append(IPAddress(rangeSet[i]).toString());

    else sync.append(IPAddress(rangeSet[i]).toString() + "-" +
                     IPAddress(rangeSet[i + 1]).toString());
  sync.endList();
}


unsigned IPRangeSet::find(uint32_t ip) const {
  // An odded numbered result means the ip was within that a range.  An even
  // numbered result means the ip was outside of all ranges in the set.

  unsigned start = 0;
  unsigned end = rangeSet.size();

  while (start < end) {
    unsigned mid = (end - start) / 2 + start;

    if (rangeSet[mid] == ip) return mid | 1; // Odd number indicates match
    if (ip < rangeSet[mid]) end = mid;
    else start = mid + 1;
  }

  return start;
}


void IPRangeSet::shift(int amount, unsigned position) {
  rangeSet_t::iterator it = rangeSet.begin() + position;

  if (0 < amount) rangeSet.insert(it, amount, (uint32_t)0);  // Grow
  else if (amount < 0) rangeSet.erase(it + amount, it); // Shrink
}
