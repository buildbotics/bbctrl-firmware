/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                            All rights reserved.

     This file ("the software") is free software: you can redistribute it
     and/or modify it under the terms of the GNU General Public License,
      version 2 as published by the Free Software Foundation. You should
      have received a copy of the GNU General Public License, version 2
     along with the software. If not, see <http://www.gnu.org/licenses/>.

     The software is distributed in the hope that it will be useful, but
          WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
               Lesser General Public License for more details.

       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#include "outputs.h"
#include "config.h"


typedef struct {
  uint8_t pin;
  bool active;
  output_state_t state;
  output_mode_t mode;
} output_t;


output_t outputs[OUTS] = {
  {TOOL_ENABLE_PIN},
  {TOOL_DIR_PIN},
  {SWITCH_1_PIN},
  {SWITCH_2_PIN},
  {FAULT_PIN},
};


static output_t *_get_output(uint8_t pin) {
  switch (pin) {
  case TOOL_ENABLE_PIN: return &outputs[0];
  case TOOL_DIR_PIN:    return &outputs[1];
  case SWITCH_1_PIN:    return &outputs[2];
  case SWITCH_2_PIN:    return &outputs[3];
  case FAULT_PIN:       return &outputs[4];
  }

  return 0;
}


static void _update_state(output_t *output) {
  switch (output->mode) {
  case OUT_DISABLED: output->state = OUT_TRI; break;
  case OUT_LO_HI: output->state = output->active ? OUT_HI : OUT_LO; break;
  case OUT_HI_LO: output->state = output->active ? OUT_LO : OUT_HI; break;
  case OUT_TRI_LO: output->state = output->active ? OUT_LO : OUT_TRI; break;
  case OUT_TRI_HI: output->state = output->active ? OUT_HI : OUT_TRI; break;
  case OUT_LO_TRI: output->state = output->active ? OUT_TRI : OUT_LO; break;
  case OUT_HI_TRI: output->state = output->active ? OUT_TRI : OUT_HI; break;
  }

  switch (output->state) {
  case OUT_TRI: DIRCLR_PIN(output->pin); break;
  case OUT_HI: OUTSET_PIN(output->pin); DIRSET_PIN(output->pin); break;
  case OUT_LO: OUTCLR_PIN(output->pin); DIRSET_PIN(output->pin); break;
  }
}


void outputs_init() {
  for (int i = 0; i < OUTS; i++) _update_state(&outputs[i]);
}


bool outputs_is_active(uint8_t pin) {
  output_t *output = _get_output(pin);
  return output ? output->active : false;
}


void outputs_set_active(uint8_t pin, bool active) {
  output_t *output = _get_output(pin);
  if (!output) return;

  output->active = active;
  _update_state(output);
}


void outputs_set_mode(uint8_t pin, output_mode_t mode) {
  output_t *output = _get_output(pin);
  if (!output) return;
  output->mode = mode;
}


output_state_t outputs_get_state(uint8_t pin) {
  output_t *output = _get_output(pin);
  if (output) return OUT_TRI;
  return output->state;
}


uint8_t get_output_state(uint8_t id) {
  return OUTS <= id ? OUT_TRI : outputs[id].state;
}


uint8_t get_output_mode(uint8_t id) {
  return OUTS <= id ? OUT_DISABLED : outputs[id].mode;
}


void set_output_mode(uint8_t id, uint8_t mode) {
  if (OUTS <= id) return;
  outputs[id].mode = mode;
  _update_state(&outputs[id]);
}
