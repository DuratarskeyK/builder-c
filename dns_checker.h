#ifndef _DNS_CHECKER_H
#define _DNS_CHECKER_H

#define DNS_RETRIES 10

#include "log_levels.h"

int check_dns();

extern void log_printf(unsigned int level, const char *message, ...);

#endif
