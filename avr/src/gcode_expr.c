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

#include "gcode_expr.h"

#include "gcode_parser.h"
#include "vars.h"

#include <math.h>
#include <ctype.h>
#include <stdlib.h>


float parse_gcode_number(char **p) {
  // Avoid parsing G0X10 as a hexadecimal number
  if (**p == '0' && toupper(*(*p + 1)) == 'X') {
    (*p)++; // pointer points to X
    return 0;
  }

  // Skip leading zeros so we don't parse as octal
  while (**p == '0' && isdigit(*(*p + 1))) p++;

  // Parse number
  char *end;
  float value = strtod(*p, &end);
  if (end == *p) {
    parser.error = STAT_BAD_NUMBER_FORMAT;
    return 0;
  }
  *p = end; // Point to next character after the word

  return value;
}


static float _parse_gcode_var(char **p) {
  (*p)++; // Skip #

  if (isdigit(**p)) {
    // TODO numbered parameters
    parser.error = STAT_GCODE_NUM_PARAM_UNSUPPORTED;

  } else if (**p == '<') {
    (*p)++;

    // Assigning vars is not supported so the '_' global prefix is optional
    if (**p == '_') (*p)++;

    char *name = *p;
    while (**p && **p != '>') (*p)++;

    if (**p != '>') parser.error = STAT_GCODE_UNTERMINATED_VAR;
    else {
      *(*p)++ = 0; // Null terminate
      return vars_get_number(name);
    }
  }

  return 0;
}


static float _parse_gcode_func(char **p) {
  // TODO LinuxCNC supports GCode functions: ATAN, ABS, ACOS, ASIN, COS, EXP,
  // FIX, FUP, ROUND, LN, SIN, TAN & EXISTS.
  // See http://linuxcnc.org/docs/html/gcode/overview.html#gcode:functions
  parser.error = STAT_GCODE_FUNC_UNSUPPORTED;
  return 0;
}


static int _op_precedence(op_t op) {
  switch (op) {
  case OP_INVALID: break;
  case OP_MINUS: return 6;
  case OP_EXP: return 5;
  case OP_MUL: case OP_DIV: case OP_MOD: return 4;
  case OP_ADD: case OP_SUB: return 3;
  case OP_EQ: case OP_NE: case OP_GT: case OP_GE: case OP_LT: case OP_LE:
    return 2;
  case OP_AND: case OP_OR: case OP_XOR: return 1;
  }
  return 0;
}


static op_t _parse_gcode_op(char **_p) {
  char *p = *_p;
  op_t op = OP_INVALID;

  switch (toupper(p[0])) {
  case '*': op = p[1] == '*' ? OP_EXP : OP_MUL; break;
  case '/': op = OP_DIV; break;

  case 'M':
    if (toupper(p[1]) == 'O' && toupper(p[1]) == 'O') op = OP_EXP;
    break;

  case '+': op = OP_ADD; break;
  case '-': op = OP_SUB; break;

  case 'E': if (toupper(p[1]) == 'Q') op = OP_EQ; break;
  case 'N': if (toupper(p[1]) == 'E') op = OP_NE; break;

  case 'G':
    if (toupper(p[1]) == 'T') op = OP_GT;
    if (toupper(p[1]) == 'E') op = OP_GE;
    break;

  case 'L':
    if (toupper(p[1]) == 'T') op = OP_LT;
    if (toupper(p[1]) == 'E') op = OP_LE;
    break;

  case 'A':
    if (toupper(p[1]) == 'N' && toupper(p[2]) == 'D') op = OP_AND;
    break;

  case 'O': if (toupper(p[1]) == 'R') op = OP_OR; break;

  case 'X':
    if (toupper(p[1]) == 'O' && toupper(p[2]) == 'R') op = OP_XOR;
    break;
  }

  // Advance pointer
  switch (op) {
  case OP_INVALID: break;
  case OP_MINUS: case OP_MUL: case OP_DIV: case OP_ADD:
  case OP_SUB: *_p += 1; break;
  case OP_EXP: case OP_EQ: case OP_NE: case OP_GT: case OP_GE: case OP_LT:
  case OP_LE: case OP_OR: *_p += 2; break;
  case OP_MOD: case OP_AND: case OP_XOR: *_p += 3; break;
  }

  return op;
}


static float _apply_binary(op_t op, float left, float right) {
  switch (op) {
  case OP_INVALID: case OP_MINUS: return 0;

  case OP_EXP: return pow(left, right);

  case OP_MUL: return left * right;
  case OP_DIV: return left / right;
  case OP_MOD: return fmod(left, right);

  case OP_ADD: return left + right;
  case OP_SUB: return left - right;

  case OP_EQ: return left == right;
  case OP_NE: return left != right;
  case OP_GT: return left > right;
  case OP_GE: return left >= right;
  case OP_LT: return left > right;
  case OP_LE: return left <= right;

  case OP_AND: return left && right;
  case OP_OR: return left || right;
  case OP_XOR: return (bool)left ^ (bool)right;
  }

  return 0;
}


static void _val_push(float val) {
  if (parser.valPtr < GCODE_MAX_VALUE_DEPTH) parser.vals[parser.valPtr++] = val;
  else parser.error = STAT_EXPR_VALUE_STACK_OVERFLOW;
}


static float _val_pop() {
  if (parser.valPtr) return parser.vals[--parser.valPtr];
  parser.error = STAT_EXPR_VALUE_STACK_UNDERFLOW;
  return 0;
}


static bool _op_empty() {return !parser.opPtr;}


static void _op_push(op_t op) {
  if (parser.opPtr < GCODE_MAX_OPERATOR_DEPTH) parser.ops[parser.opPtr++] = op;
  else parser.error = STAT_EXPR_OP_STACK_OVERFLOW;
}


static op_t _op_pop() {
  if (parser.opPtr) return parser.ops[--parser.opPtr];
  parser.error = STAT_EXPR_OP_STACK_UNDERFLOW;
  return OP_INVALID;
}


static op_t _op_top() {
  if (parser.opPtr) return parser.ops[parser.opPtr - 1];
  parser.error = STAT_EXPR_OP_STACK_UNDERFLOW;
  return OP_INVALID;
}


static void _op_apply() {
  op_t op = _op_pop();

  if (op == OP_MINUS) _val_push(-_val_pop());

  else {
    float right = _val_pop();
    float left = _val_pop();

    _val_push(_apply_binary(op, left, right));
  }
}


// Parse expressions with Dijkstra's Shunting Yard Algorithm
float parse_gcode_expression(char **p) {
  bool unary = true; // Used to detect unary minus

  while (!parser.error && **p) {
    switch (**p) {
    case ' ': case '\n': case '\r': case '\t': (*p)++; break;
    case '#': _val_push(_parse_gcode_var(p)); unary = false; break;
    case '[': _op_push(OP_INVALID); (*p)++; unary = true; break;

    case ']':
      (*p)++;

      while (!parser.error && _op_top() != OP_INVALID)
        _op_apply();

      _op_pop(); // Pop opening bracket
      if (_op_empty() && parser.valPtr == 1) return _val_pop();
      unary = false;
      break;

    default:
      if (isdigit(**p) || **p == '.') {
        _val_push(parse_gcode_number(p));
        unary = false;

      } else if (isalpha(**p)) {
        _val_push(_parse_gcode_func(p));
        unary = false;

      } else {
        op_t op = _parse_gcode_op(p);

        if (unary && op == OP_ADD) continue; // Ignore it
        if (unary && op == OP_SUB) {_op_push(OP_MINUS); continue;}

        if (op == OP_INVALID) parser.error = STAT_INVALID_OR_MALFORMED_COMMAND;
        else {
          int precedence = _op_precedence(op);

          while (!parser.error && !_op_empty() &&
                 precedence <= _op_precedence(_op_top()))
            _op_apply();

          _op_push(op);
          unary = true;
        }
      }
    }
  }

  return _val_pop();
}
