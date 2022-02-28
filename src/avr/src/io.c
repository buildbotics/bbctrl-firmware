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

#include "io.h"
#include "config.h"

#include <stdint.h>
#include <string.h>


#define IO_MAX_INDEX 17

// macros for finding io function given the axis number
#define MIN_INPUT(axis) ((io_function_t)(INPUT_MOTOR_0_MIN + axis))
#define MAX_INPUT(axis) ((io_function_t)(INPUT_MOTOR_0_MAX + axis))

#define IO_INVALID 0xff


static struct {
  uint16_t debounce;
  uint16_t lockout;
  uint8_t adc_ch_pin[2];
} _io = {
  .debounce = INPUT_DEBOUNCE,
  .lockout = INPUT_LOCKOUT,
};


typedef struct {
  bool state;
  uint16_t debounce;
  uint16_t lockout;
  bool initialized;
} io_input_t;


typedef struct {
  uint8_t pin;
  uint8_t types;
  uint8_t adc_ch;
  uint8_t adc_mux;
  io_function_t function;
  uint8_t mode;
  io_input_t input;
} io_pin_t;


typedef struct {
  uint8_t active;
  io_callback_t cb;
} io_func_state_t;


static io_pin_t _pins[] = {
  // Remapable pins
  {IO_01_PIN, IO_TYPE_OUTPUT},
  {IO_02_PIN, IO_TYPE_OUTPUT},
  {IO_03_PIN, IO_TYPE_INPUT},
  {IO_04_PIN, IO_TYPE_INPUT},
  {IO_05_PIN, IO_TYPE_INPUT},
  {IO_08_PIN, IO_TYPE_INPUT},
  {IO_09_PIN, IO_TYPE_INPUT},
  {IO_10_PIN, IO_TYPE_INPUT},
  {IO_11_PIN, IO_TYPE_INPUT},
  {IO_12_PIN, IO_TYPE_INPUT},
  {IO_15_PIN, IO_TYPE_OUTPUT},
  {IO_16_PIN, IO_TYPE_OUTPUT},
  {IO_18_PIN, IO_TYPE_ANALOG, 0, ADC_CH_MUXPOS_PIN7_gc},
  {IO_21_PIN, IO_TYPE_OUTPUT},
  {IO_22_PIN, IO_TYPE_INPUT},
  {IO_23_PIN, IO_TYPE_INPUT},
  {IO_24_PIN, IO_TYPE_ANALOG, 0, ADC_CH_MUXPOS_PIN6_gc},

  // Hard wired pins
  {STALL_0_PIN,     IO_TYPE_INPUT,  0, 0, INPUT_STALL_0,     NORMALLY_OPEN},
  {STALL_1_PIN,     IO_TYPE_INPUT,  0, 0, INPUT_STALL_1,     NORMALLY_OPEN},
  {STALL_2_PIN,     IO_TYPE_INPUT,  0, 0, INPUT_STALL_2,     NORMALLY_OPEN},
  {STALL_3_PIN,     IO_TYPE_INPUT,  0, 0, INPUT_STALL_3,     NORMALLY_OPEN},
  {MOTOR_FAULT_PIN, IO_TYPE_INPUT,  0, 0, INPUT_MOTOR_FAULT, NORMALLY_OPEN},
  {TEST_PIN,        IO_TYPE_OUTPUT, 0, 0, OUTPUT_TEST,       HI_LO},

  {0}, // Sentinal
};

static io_func_state_t _func_state[IO_FUNCTION_COUNT];
static float _analog_ports[ANALOGS];


static io_function_t _output_to_function(int id) {
  switch (id) {
  case 0:  return OUTPUT_0;
  case 1:  return OUTPUT_1;
  case 2:  return OUTPUT_2;
  case 3:  return OUTPUT_3;
  case 4:  return OUTPUT_MIST;
  case 5:  return OUTPUT_FLOOD;
  case 6:  return OUTPUT_FAULT;
  case 7:  return OUTPUT_TOOL_ENABLE;
  case 8:  return OUTPUT_TOOL_DIRECTION;
  case 9:  return OUTPUT_TEST;
  default: return IO_DISABLED;
  }
}


static io_function_t _input_to_function(int id) {
  switch (id) {
  case 0:  return INPUT_0;
  case 1:  return INPUT_1;
  case 2:  return INPUT_2;
  case 3:  return INPUT_3;
  case 4:  return INPUT_ESTOP;
  case 5:  return INPUT_PROBE;
  default: return IO_DISABLED;
  }
}


static uint8_t _function_to_analog_port(io_function_t function) {
  return function - ANALOG_0;
}


static bool _is_valid(io_function_t function, io_type_t type) {
  return io_get_type(function) == type;
}


static void _state_set_active(io_function_t function, bool active) {
  if (!_is_valid(function, IO_TYPE_INPUT)) return;

  io_func_state_t *state = &_func_state[function];

  if (active) {
    state->active++;
    if (state->active == 1 && state->cb) state->cb(function, true);

  } else {
    state->active--;
    if (!state->active && state->cb) state->cb(function, false);
  }
}


static uint8_t _out_mode_state(uint8_t mode, bool active) {
  switch (mode) {
  case LO_HI:  return active ? IO_HI  : IO_LO;
  case HI_LO:  return active ? IO_LO  : IO_HI;
  case TRI_LO: return active ? IO_LO  : IO_TRI;
  case TRI_HI: return active ? IO_HI  : IO_TRI;
  case LO_TRI: return active ? IO_TRI : IO_LO;
  case HI_TRI: return active ? IO_TRI : IO_HI;
  default: return IO_TRI;
  }
}


static uint8_t _get_pin_output_mode(io_pin_t *pin) {
  if (!DIR_PIN(pin->pin)) return IO_TRI;
  return OUT_PIN(pin->pin) ? IO_HI : IO_LO;
}


static bool _is_active(io_pin_t *pin) {
  switch (io_get_type(pin->function)) {
  case IO_TYPE_INPUT:
    if (!pin->input.initialized) return false;

    switch (pin->mode) {
    case NORMALLY_OPEN:   return !pin->input.state;
    case NORMALLY_CLOSED: return  pin->input.state;
    default:              return false;
    }
    break;

  case IO_TYPE_OUTPUT:
    return _get_pin_output_mode(pin) == _out_mode_state(pin->mode, true);

  default: return false;
  }
}


static void _set_output(io_pin_t *pin, bool active) {
  uint8_t state = _out_mode_state(pin->mode, active);

  switch (state) {
  case IO_LO: OUTCLR_PIN(pin->pin); DIRSET_PIN(pin->pin); break;
  case IO_HI: OUTSET_PIN(pin->pin); DIRSET_PIN(pin->pin); break;
  default:                          DIRCLR_PIN(pin->pin); break;
  }
}


static void _set_function(io_pin_t *pin, io_function_t function) {
  io_type_t oldType = io_get_type(pin->function);

  switch (oldType) {
  case IO_TYPE_INPUT:
    // Release active input
    if (_is_active(pin)) _state_set_active(pin->function, false);
    memset(&pin->input, 0, sizeof(pin->input)); // Reset input state
    break;

  case IO_TYPE_ANALOG:
    _analog_ports[_function_to_analog_port(pin->function)] = 0;
    break;

  default: break;
  }

  pin->function = function;
  io_type_t newType = io_get_type(function);
  if (!(pin->types & newType)) newType = IO_TYPE_DISABLED;

  switch (newType) {
  case IO_TYPE_INPUT:
    PINCTRL_PIN(pin->pin) = PORT_OPC_PULLUP_gc; // Pull up
    DIRCLR_PIN(pin->pin); // Input
    break;

  case IO_TYPE_OUTPUT:
    _set_output(pin, _func_state[function].active);
    break;

  case IO_TYPE_ANALOG:
    _analog_ports[_function_to_analog_port(function)] = 0;
    PINCTRL_PIN(pin->pin) = 0; // Disable pull up
    DIRCLR_PIN(pin->pin); // Input
    break;

    // Fall through if analog disabled

  case IO_TYPE_DISABLED:
    PINCTRL_PIN(pin->pin) = 0; // Disable pull up
    DIRCLR_PIN(pin->pin);      // Input
    break;
  }
}


static io_pin_t *_next_adc_pin(uint8_t ch) {
  uint8_t current = _io.adc_ch_pin[ch];

  while (true) {
    uint8_t next = ++_io.adc_ch_pin[ch];
    if (!_pins[next].pin) _io.adc_ch_pin[ch] = next = 0;
    if (_pins[next].adc_ch == ch) return &_pins[next];
    if (next == current) break; // We checked all pins
  }

  return 0;
}


static ADC_CH_t *_get_adc_ch(uint8_t ch) {return ch ? &ADCA.CH1 : &ADCA.CH0;}


static void _start_acd_conversion(io_pin_t *pin) {
  _get_adc_ch(pin->adc_ch)->MUXCTRL = pin->adc_mux;
  ADCA.CTRLA |= pin->adc_ch ? ADC_CH1START_bm : ADC_CH0START_bm;
}


static float _convert_analog_result(uint16_t result) {
  return result; // TODO convert result
}


static void _adc_callback(uint8_t ch) {
  io_pin_t *pin = &_pins[_io.adc_ch_pin[ch]];

  if (pin->adc_ch == ch) {
    uint16_t result = _get_adc_ch(ch)->RES;

    if (_is_valid(pin->function, IO_TYPE_ANALOG)) {
      unsigned port = _function_to_analog_port(pin->function);
      _analog_ports[port] = _convert_analog_result(result);
    }
  }
}


ISR(ADCA_CH0_vect) {_adc_callback(0);}
ISR(ADCA_CH1_vect) {_adc_callback(1);}


void io_init() {
  // Analog channels
  for (int i = 0; i < 2; i++) {
    ADC_CH_t *ch = _get_adc_ch(i);
    ch->CTRL    = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;
    ch->INTCTRL = ADC_CH_INTLVL_LO_gc;
  }

  // Analog module
  ADCA.REFCTRL   = ADC_REFSEL_INTVCC_gc; // 3.3V / 1.6 = 2.06V
  ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;
  ADCA.EVCTRL    = ADC_SWEEP_01_gc;
  ADCA.CTRLA     = ADC_FLUSH_bm | ADC_ENABLE_bm;

  // Init fixed functions
  for (int i = 0; _pins[i].pin; i++)
    _set_function(&_pins[i], _pins[i].function);
}


bool io_is_enabled(io_function_t function) {
  for (int i = 0; _pins[i].pin; i++)
    if (_pins[i].function == function)
      return true;

  return false;
}


io_type_t io_get_type(io_function_t function) {
  switch (function) {
  case IO_DISABLED: break;

  case INPUT_MOTOR_0_MAX: case INPUT_MOTOR_1_MAX: case INPUT_MOTOR_2_MAX:
  case INPUT_MOTOR_3_MAX: case INPUT_MOTOR_0_MIN: case INPUT_MOTOR_1_MIN:
  case INPUT_MOTOR_2_MIN: case INPUT_MOTOR_3_MIN:
  case INPUT_0: case INPUT_1: case INPUT_2: case INPUT_3:
  case INPUT_ESTOP: case INPUT_PROBE:
    return IO_TYPE_INPUT;

  case OUTPUT_0: case OUTPUT_1: case OUTPUT_2: case OUTPUT_3:
  case OUTPUT_MIST: case OUTPUT_FLOOD: case OUTPUT_FAULT:
  case OUTPUT_TOOL_ENABLE: case OUTPUT_TOOL_DIRECTION:
    return IO_TYPE_OUTPUT;

  case ANALOG_0: case ANALOG_1: case ANALOG_2: case ANALOG_3:
    return IO_TYPE_ANALOG;

  case INPUT_STALL_0: case INPUT_STALL_1: case INPUT_STALL_2:
  case INPUT_STALL_3: case INPUT_MOTOR_FAULT:
    return IO_TYPE_INPUT;

  case OUTPUT_TEST: return IO_TYPE_OUTPUT;

  case IO_FUNCTION_COUNT: break;
  }

  return IO_TYPE_DISABLED;
}


void io_set_callback(io_function_t function, io_callback_t cb) {
  if (function < IO_FUNCTION_COUNT) _func_state[function].cb = cb;
}


io_callback_t io_get_callback(io_function_t function) {
  return function < IO_FUNCTION_COUNT ? _func_state[function].cb : 0;
}


void io_set_output(io_function_t function, bool active) {
  if (!_is_valid(function, IO_TYPE_OUTPUT)) return;
  if (_func_state[function].active == active) return;

  _func_state[function].active = active;

  for (int i = 0; _pins[i].pin; i++) {
    io_pin_t *pin = &_pins[i];
    if (pin->function == function) _set_output(pin, active);
  }
}


void io_stop_outputs() {
  for (int i = 0; i < 4; i++)
    io_set_output((io_function_t)(OUTPUT_0 + i), false);
}


io_function_t io_get_port_function(bool digital, uint8_t port) {
  if ((digital ? DIGITALS : ANALOGS) <= port) return IO_DISABLED;
  return (io_function_t)((digital ? INPUT_0 : ANALOG_0) + port);
}


uint8_t io_get_input(io_function_t function) {
  if (!_is_valid(function, IO_TYPE_INPUT)) return IO_TRI;
  return !!_func_state[function].active;
}


float io_get_analog(io_function_t function) {
  if (!_is_valid(function, IO_TYPE_ANALOG)) return 0;
  return _analog_ports[_function_to_analog_port(function)];
}


/// Called from RTC on each tick
void io_rtc_callback() {
  for (int i = 0; _pins[i].pin; i++) {
    io_pin_t *pin = &_pins[i];

    // Digital input
    if (_is_valid(pin->function, IO_TYPE_INPUT)) {
      io_input_t *input = &pin->input;

      if (input->lockout && --input->lockout) continue;

      // Debounce switch
      bool state = IN_PIN(pin->pin);
      if (state == input->state && input->initialized) input->debounce = 0;
      else if (++input->debounce == _io.debounce) {
        bool first = !input->initialized;

        input->state = state;
        input->debounce = 0;
        input->initialized = true;
        input->lockout = _io.lockout;

        if (!first || _is_active(pin))
          _state_set_active(pin->function, _is_active(pin));
      }
    }
  }

  // Analog input
  static uint8_t count = 0;

  // Every 1/4 sec
  if (++count == 250) {
    count = 0;

    // Start next analog conversions
    for (int ch = 0; ch < 2; ch++) {
      io_pin_t *pin = _next_adc_pin(ch);
      if (pin) _start_acd_conversion(pin);
    }
  }
}


static uint8_t _get_state(io_function_t function) {
  if (!_is_valid(function, IO_TYPE_INPUT) || !io_is_enabled(function))
    return IO_INVALID;

  return !!_func_state[function].active;
}


// Var callbacks
uint8_t get_io_function(int index) {
  return IO_MAX_INDEX <= index ? IO_DISABLED : _pins[index].function;
}


void set_io_function(int index, uint8_t function) {
  if (IO_MAX_INDEX <= index || IO_FUNCTION_COUNT <= function ||
      _pins[index].function == function) return;

  _set_function(&_pins[index], (io_function_t)function);
}


uint8_t get_io_mode(int index) {
  return IO_MAX_INDEX <= index ? 0 : _pins[index].mode;
}


void set_io_mode(int index, uint8_t mode) {
  if (IO_MAX_INDEX <= index || NORMALLY_OPEN < mode) return;

  io_pin_t *pin = &_pins[index];
  bool wasActive = _is_active(pin);
  pin->mode = mode;
  bool isActive = _is_active(pin);

  // Changing the mode may change the pin state
  if (wasActive != isActive)
    switch (io_get_type(pin->function)) {
    case IO_TYPE_OUTPUT: _set_output(pin, wasActive);                break;
    case IO_TYPE_INPUT:  _state_set_active(pin->function, isActive); break;
    default: break;
    }
}


uint8_t get_io_state(int pin) {return _is_active(&_pins[pin]);}


void set_input_debounce(uint16_t debounce) {
  _io.debounce = INPUT_MAX_DEBOUNCE < debounce ? INPUT_DEBOUNCE : debounce;
}


uint16_t get_input_debounce() {return _io.debounce;}


void set_input_lockout(uint16_t lockout) {
  _io.lockout = INPUT_MAX_LOCKOUT < lockout ? INPUT_LOCKOUT : lockout;
}


uint16_t get_input_lockout() {return _io.lockout;}
uint8_t get_min_input(int axis) {return _get_state(MIN_INPUT(axis));}
uint8_t get_max_input(int axis) {return _get_state(MAX_INPUT(axis));}
uint8_t get_input(int index) {return _get_state(_input_to_function(index));}


uint8_t get_output_active(int index) {
  return _func_state[_output_to_function(index)].active;
}


void set_output_active(int index, uint8_t active) {
  io_set_output(_output_to_function(index), active);
}


float get_analog_input(int port) {
  return io_get_analog(io_get_port_function(false, port));
}
