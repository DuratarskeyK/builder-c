#ifndef _API_H
#define _API_H

#include "structs.h"
#include "api_data.h"

static int curl_get(const char *, char **);
static int curl_put(const char *, const char *);
void api_set_token(const char *);
void api_set_api_url(const char *);
int api_job_statistics(const char *);
int api_jobs_logs(const char *, const char *);
int api_jobs_shift(char **, const char *);
int api_jobs_feedback(const char *, int, const char *);
int api_jobs_status(const char *);

#endif