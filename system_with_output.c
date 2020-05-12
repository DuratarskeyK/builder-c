#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system_with_output.h"

int system_with_output(const char *cmd, char **output) {
  char *final_cmd = xmalloc(strlen(cmd) + 6);
  sprintf(final_cmd, "%s 2>&1", cmd);
  FILE *proc = popen(final_cmd, "r");
  if (proc == NULL) {
    return -1;
  }
  int cnt = 0, max = 1024;
  *output = xmalloc(1024);
  while(1) {
    int read_size = fread(*output + cnt, 1, 1024, proc);
    cnt += read_size;
    if (ferror(proc) || feof(proc)) {
      break;
    }
    if (cnt + 1024 >= max) {
      max += 1024;
      *output = realloc(*output, max);
      if (*output == NULL) {
        return -1;
      }
    }
  }
  (*output)[cnt - 1] = '\0';
  int exit_code = pclose(proc);
  if (exit_code < 0) {
    return -1;
  }
  return exit_code;
}
