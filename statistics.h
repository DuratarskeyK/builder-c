#ifndef _STATISTICS_H
#define _STATISTICS_H

#include "structs.h"
#include "api_data.h"

static void *statistics(void *);
static char char2hex(unsigned char);
void set_busy_status(int s);
int start_statistics_thread(const char *);

#endif
