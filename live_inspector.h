#ifndef _LIVE_INSPECTOR_H
#define _LIVE_INSPECTOR_H

#include "structs.h"

static void *live_inspector(void *);
int start_live_inspector(int, pid_t, const char *);
void stop_live_inspector();

#endif