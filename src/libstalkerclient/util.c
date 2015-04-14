#include "util.h"

#include <string.h>
#include <stdlib.h>

char* sc_util_strcpy(char *str) {
  size_t len;

  len = strlen(str) + 1;
  char *buffer = (char *) malloc(len * sizeof (char));
  strcpy(buffer, str);

  return buffer;
}
