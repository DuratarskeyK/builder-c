#include <string.h>
#include "curl_callbacks.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t sz = nmemb * size;
	mem_t *mem = (mem_t *)userdata;

	if (mem->ptrs.write_ptr == NULL) {
		mem->offset = 0;
	}
	mem->ptrs.write_ptr = xrealloc(mem->ptrs.write_ptr, mem->offset + sz + 1);

	memcpy(mem->ptrs.write_ptr + mem->offset, ptr, sz);
	mem->offset += sz;
	mem->ptrs.write_ptr[mem->offset] = '\0';

	return sz;
}

size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
	mem_t *c = (mem_t *)stream;
	size_t offset = c->offset;
	size_t to_read, retcode;

	to_read = size * nmemb;
	if(to_read > strlen((char *)c->ptrs.read_ptr) - offset) {
		to_read = strlen((char *)c->ptrs.read_ptr) - offset;
	}

	memcpy(ptr, c->ptrs.read_ptr + c->offset, to_read);

	offset += to_read;
	if(offset > strlen((char *)c->ptrs.read_ptr)) {
		offset = strlen((char *)c->ptrs.read_ptr);
	}

	c->offset = offset;
	retcode = to_read;

	return retcode;
}
