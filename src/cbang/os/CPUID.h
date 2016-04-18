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

#ifndef CPUID_H
#define CPUID_H

#include <cbang/StdTypes.h>

#include <cbang/enum/CPUFeature.h>
#include <cbang/enum/CPUExtendedFeature.h>

#include <string>

namespace cb {
  class CPUID {
    uint32_t regs[4];

  public:
    CPUID &cpuID(uint32_t eax, uint32_t ebx = 0, uint32_t ecx = 0,
                 uint32_t edx = 0);

    const uint32_t *getRegs() const {return regs;}

    static uint32_t getBits(uint32_t x, unsigned start = 31, unsigned end = 0);
    uint32_t EAX(unsigned start = 31, unsigned end = 0) const
      {return getBits(regs[0], start, end);}
    uint32_t EBX(unsigned start = 31, unsigned end = 0) const
      {return getBits(regs[1], start, end);}
    uint32_t ECX(unsigned start = 31, unsigned end = 0) const
      {return getBits(regs[2], start, end);}
    uint32_t EDX(unsigned start = 31, unsigned end = 0) const
      {return getBits(regs[3], start, end);}

    std::string getCPUBrand();
    std::string getCPUVendor();
    uint32_t getCPUSignature();
    uint64_t getCPUFeatures();
    uint64_t getCPUExtendedFeatures();
    bool cpuHasFeature(CPUFeature feature);
    bool cpuHasExtendedFeature(CPUExtendedFeature feature);
    unsigned getCPUFamily();
    unsigned getCPUModel();
    unsigned getCPUStepping();
    void getCPUCounts(uint32_t &logical, uint32_t &cores, uint32_t &threads);
  };
}

#endif // CPUID_H
