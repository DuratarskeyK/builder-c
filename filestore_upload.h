#ifndef _FILESTORE_UPLOAD_H
#define _FILESTORE_UPLOAD_H

#include "log_levels.h"
#include "structs.h"

#define RETRIES (16)
#define MEGABYTE (1024*1024.0)

static const char file_line[]="{\"sha1\":\"%s\",\"file_name\":\"%s\",\"size\":\"%.2lf\"}";

int filestore_upload(char **results);
static void upload_file(const char *name, off_t size, const char *path, char **json);

extern void log_printf(unsigned int level, const char *message, ...);

extern builder_config_t builder_config;

extern size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

extern char *alloc_sprintf(const char *message, ...);

extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrndup(const char *s, size_t n);

#endif
