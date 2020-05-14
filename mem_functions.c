#include <stdio.h>
#include <stdlib.h>
#include "mem_functions.h"

void *xmalloc(size_t size) {
	if (size == 0) {
		size = 1;
	}
	void *res = malloc(size);
	if (res == NULL) {
		printf("FATAL: Can't allocate enough memory for operation, aborting.\n");
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
		printf("FATAL: Can't allocate enough memory for operation, aborting.\n");
		exit(100);
	}

	return newptr;
}