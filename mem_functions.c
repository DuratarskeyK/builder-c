#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_functions.h"

void *xmalloc(size_t size) {
	if (size == 0) {
		size = 1;
	}
	void *res = malloc(size);
	if (res == NULL) {
		puts(no_mem_msg);
		exit(100);
	}

	return res;
}

void *xrealloc(void *ptr, size_t size) {
	if (size == 0) {
		size = 1;
	}
	void *newptr = realloc(ptr, size);
	if (newptr == NULL) {
		puts(no_mem_msg);
		exit(100);
	}

	return newptr;
}

char *xstrdup(const char *s) {
	char *res = strdup(s);
	if (res == NULL) {
		puts(no_mem_msg);
		exit(100);
	}

	return res;
}

char *xstrndup(const char *s, size_t n) {
	char *res = strndup(s, n);
	if (res == NULL) {
		puts(no_mem_msg);
		exit(100);
	}

	return res;
}
