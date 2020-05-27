/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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
  {TEST_PIN},
};


static output_t *_get_output(uint8_t pin) {
  switch (pin) {
  case TOOL_ENABLE_PIN: return &outputs[0];
  case TOOL_DIR_PIN:    return &outputs[1];
  case SWITCH_1_PIN:    return &outputs[2];
  case SWITCH_2_PIN:    return &outputs[3];
  case FAULT_PIN:       return &outputs[4];
  case TEST_PIN:        return &outputs[5];
  }

  return 0;
}


static void _update_state(output_t *output) {
  switch (output->mode) {
  case OUT_DISABLED: output->state = OUT_TRI; break;
  case OUT_LO_HI:  output->state = output->active ? OUT_HI  : OUT_LO;  break;
  case OUT_HI_LO:  output->state = output->active ? OUT_LO  : OUT_HI;  break;
  case OUT_TRI_LO: output->state = output->active ? OUT_LO  : OUT_TRI; break;
  case OUT_TRI_HI: output->state = output->active ? OUT_HI  : OUT_TRI; break;
  case OUT_LO_TRI: output->state = output->active ? OUT_TRI : OUT_LO;  break;
  case OUT_HI_TRI: output->state = output->active ? OUT_TRI : OUT_HI;  break;
  }

  switch (output->state) {
  case OUT_TRI: DIRCLR_PIN(output->pin); break;
  case OUT_HI:  OUTSET_PIN(output->pin); DIRSET_PIN(output->pin); break;
  case OUT_LO:  OUTCLR_PIN(output->pin); DIRSET_PIN(output->pin); break;
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


bool outputs_toggle(uint8_t pin) {
  output_t *output = _get_output(pin);
  if (!output) return false;

  output->active = !output->active;
  _update_state(output);
  return output->active;
}


void outputs_set_mode(uint8_t pin, output_mode_t mode) {
  output_t *output = _get_output(pin);
  if (!output) return;
  output->mode = mode;
  _update_state(output);
}


output_state_t outputs_get_state(uint8_t pin) {
  output_t *output = _get_output(pin);
  if (output) return OUT_TRI;
  return output->state;
}


void outputs_stop() {
  outputs_set_active(SWITCH_1_PIN, false);
  outputs_set_active(SWITCH_2_PIN, false);
}


// Var callbacks
uint8_t get_output_state(int id) {
  return OUTS <= id ? OUT_TRI : outputs[id].state;
}


bool get_output_active(int id) {
  return OUTS <= id ? false : outputs[id].active;
}


void set_output_active(int id, bool active) {
  if (OUTS <= id) return;
  outputs[id].active = active;
  _update_state(&outputs[id]);
}


uint8_t get_output_mode(int id) {
  return OUTS <= id ? OUT_DISABLED : outputs[id].mode;
}


void set_output_mode(int id, uint8_t mode) {
  if (OUTS <= id) return;
  outputs[id].mode = (output_mode_t)mode;
  _update_state(&outputs[id]);
}
