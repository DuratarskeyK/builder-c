#ifndef _API_H
#define _API_H

#include "api_data.h"
#include "log_levels.h"

static int curl_get(const char *, char **);
static int curl_put(const char *, const char *);

void init_api(const char *url, const char *api_token);
int api_job_statistics(const char *);
int api_jobs_logs(const char *, const char *);
int api_jobs_shift(char **, const char *);
int api_jobs_feedback(const char *, int, const char *);
int api_jobs_status(const char *);

typedef struct {
	const char *ptr;
	int offset;
} mem_t;

extern void log_printf(unsigned int level, const char *message, ...);

#endif
