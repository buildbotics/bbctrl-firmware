#pragma once

#include "config.h"

#include <stdbool.h>

void mp_jog(float velocity[AXES]);
bool mp_jog_busy();
