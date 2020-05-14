#ifndef _API_H
#define _API_H

#include "api_data.h"
#include "log_levels.h"
#include "structs.h"

static int curl_get(const char *, char **);
static int curl_put(const char *, const char *);

void init_api(const char *url, const char *tok, const char *qs);
int api_job_statistics(const char *);
int api_jobs_logs(const char *, const char *);
int api_jobs_shift(char **);
int api_jobs_feedback(const char *, int, const char *);
int api_jobs_status(const char *);

extern size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
extern size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

extern void log_printf(unsigned int level, const char *message, ...);

extern void *xmalloc(size_t size);

#endif
