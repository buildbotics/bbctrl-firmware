#include "devices.h"

#include <string.h>


device_t devices[] = {
#include "devices.dat"
  {0} // Sentinel
};


device_t *devices_find(char *name) {
  for (int i = 0; devices[i].name; i++)
    if (strcmp(name, devices[i].name) == 0)
      return &devices[i];

  return 0;
}


device_t *devices_find_by_sig(uint32_t sig) {
  for (int i = 0; devices[i].name; i++)
    if (devices[i].sig == sig)
      return &devices[i];

  return 0;
}
