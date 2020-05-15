#ifndef _CURL_CALLBACKS_H
#define _CURL_CALLBACKS_H

#include "structs.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);

#endif
