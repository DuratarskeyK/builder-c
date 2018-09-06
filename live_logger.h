#ifndef _LIVE_LOGGER_H
#define _LIVE_LOGGER_H

static void *buffer_dump(void *);
static void *read_log(void *);
int start_live_logger(const char *, int);
void stop_live_logger();

#endif