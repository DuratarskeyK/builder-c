#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system_with_output.h"

int system_with_output(const char *cmd, char **output) {
  FILE *temp = tmpfile();
  int fd = fileno(temp);
  char *final_cmd = malloc(strlen(cmd) + 100);
  sprintf(final_cmd, "(%s) 1>&%d 2>&%d", cmd, fd, fd);
  int exit_code = system(final_cmd);
  fflush(temp);
  free(final_cmd);
  long pos = ftell(temp);
  fseek(temp, 0, SEEK_SET);
  *output = malloc((size_t)(pos + 1));
  fread(*output, pos, 1, temp);
  (*output)[pos] = '\0';
  fclose(temp);
  return exit_code;
}
