/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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

#include "report.h"
#include "config.h"
#include "usart.h"
#include "rtc.h"
#include "vars.h"


static bool _full = false;
static uint32_t _last = 0;


void report_request_full() {_full = true;}


void report_callback() {
  // Wait until output buffer is empty
  if (!usart_tx_empty()) return;

  // Limit frequency
  uint32_t now = rtc_get_time();
  if (now - _last < REPORT_RATE) return;
  _last = now;

  // Report vars
  vars_report(_full);
  _full = false;
}
