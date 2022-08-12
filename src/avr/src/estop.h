/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#pragma once

#include "status.h"

#include <stdbool.h>


/*
 * Estop cases:
 *   - Estop switch triggered
 *   - User trigged software estop
 *   - Unexpected limit switch
 *   - Motor fault
 *   - Shutdown command
 *   - Code assert
*/

void estop_init();
bool estop_triggered();
void estop_trigger(stat_t reason);
void estop_clear();


#define ESTOP_ASSERT(COND, CODE)                    \
  do {if (!(COND)) estop_trigger(CODE);} while (0)
