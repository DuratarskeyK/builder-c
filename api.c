#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include "api.h"

static const char *token = NULL;
static const char *api_url = NULL;

typedef struct {
	const char *ptr;
	int offset;
} mem_t;

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
	mem_t *c = (mem_t *)stream;
	size_t offset = c->offset;
	size_t to_read, retcode;

	to_read = size * nmemb;
	if(to_read > strlen((char *)c->ptr) - offset) {
		to_read = strlen((char *)c->ptr) - offset;
	}

	memcpy(ptr, c->ptr + c->offset, to_read);

	offset += to_read;
	if(offset > strlen((char *)c->ptr)) {
		offset = strlen((char *)c->ptr);
	}

	c->offset = offset;
	retcode = to_read;

	return retcode;
}

static int curl_get(const char *url, char **buf) {
	CURL *curl_handle;
	CURLcode res;
	FILE *tmpf;
	long pos;

	tmpf = tmpfile();

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)tmpf);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	if(token != NULL) {
		curl_easy_setopt(curl_handle, CURLOPT_USERNAME, token);
	}

	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK) {
		fclose(tmpf);
		curl_easy_cleanup(curl_handle);
		return 1;
	}
	else {
		fflush(tmpf);
		pos = ftell(tmpf);
		fseek(tmpf, 0, SEEK_SET);
		*buf = malloc((size_t)(pos + 1));
		fread(*buf, pos, 1, tmpf);
		(*buf)[pos] = '\0';
	}

	fclose(tmpf);
	curl_easy_cleanup(curl_handle);
	return 0;
}

static int curl_put(const char *url, const char *buf) {
	CURL *curl;
	CURLcode res;
	mem_t mem;

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl = curl_easy_init();

	mem.ptr = buf;
	mem.offset = 0;

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 	curl_easy_setopt(curl, CURLOPT_PUT, 1L);
 	curl_easy_setopt(curl, CURLOPT_URL, url);
 	curl_easy_setopt(curl, CURLOPT_READDATA, &mem);
 	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
 	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
 	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)strlen(buf));
 	if(token != NULL) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, token);
	}

 	res = curl_easy_perform(curl);
 	if(res != CURLE_OK) {
 		curl_slist_free_all(headers);
 		curl_easy_cleanup(curl);
 		return 1;
 	}

 	curl_slist_free_all(headers);
 	curl_easy_cleanup(curl);
 	return 0;
}

void api_set_token(const char *t) {
	token = t;
}

void api_set_api_url(const char *url) {
	api_url = url;
}

int api_job_statistics(const char *payload) {
	char *path;
	if(api_url == NULL) {
		return -1;
	}

	path = malloc(strlen(api_url) + strlen(API_JOBS_STATISTICS) + 1);
	sprintf(path, "%s%s", api_url, API_JOBS_STATISTICS);

	if(curl_put(path, payload)) {
		free(path);
		return -1;
	}

	free(path);

	return 0;
}

int api_jobs_shift(char **buf, const char *query_string) {
	char *path;
	if(api_url == NULL) {
		return -1;
	}

	if(query_string != NULL) {
		path = malloc(strlen(api_url) + strlen(API_JOBS_SHIFT) + strlen(query_string) + 2);
		sprintf(path, "%s%s?%s", api_url, API_JOBS_SHIFT, query_string);
	}
	else {
		path = malloc(strlen(api_url) + strlen(API_JOBS_SHIFT) + 1);
		sprintf(path, "%s%s", api_url, API_JOBS_SHIFT);
	}

	if(curl_get(path, buf)) {
		free(path);
		return -1;
	}

	free(path);
	return 0;
}

int api_jobs_status(const char *build_id) {
	if(api_url == NULL) {
		return -1;
	}

	char *path = malloc(strlen(api_url) + strlen(API_JOBS_STATUS) + strlen(build_id) + strlen(API_STATUS_QUERYSTRING) + 1);

	sprintf(path, "%s%s" API_STATUS_QUERYSTRING, api_url, API_JOBS_STATUS, build_id);

	char *ret;

	if(curl_get(path, &ret)) {
		return 0;
	}

	int f;
	if(strstr(ret, "USR1")) {
		f = 1;
	}
	else {
		f = 0;
	}

	free(ret);
	free(path);

	return f;
}

int api_jobs_feedback(const char *build_id, int status, const char *args) {
	char *path;

	if(api_url == NULL) {
		return -1;
	}

	path = malloc(strlen(api_url) + strlen(API_JOBS_FEEDBACK) + 1);
	sprintf(path, "%s%s", api_url, API_JOBS_FEEDBACK);
	char *final_send, *final_args;
	if(args) {
		int len = strlen(args);
		if(args[0] == '{' && args[len-1] == '}') {
			final_args = malloc(strlen(args) + strlen(",\"id\":xxxxxxxxxxxx,\"status\":xxx}") + 1);
			sprintf(final_args, "%s,\"id\":%s,\"status\":%d}", args, build_id, status);
			final_args[len-1] = ' ';
			final_send = malloc(strlen(args) + strlen(",\"id\":xxxxxxxxxxxx,\"status\":xxx}")\
						 + strlen("{\"worker_queue\":\"\",\"worker_class\":\"\",\"worker_args\":[]}") +\
						 strlen(OBSERVER_QUEUE) + strlen(OBSERVER_CLASS) + 1);
			sprintf(final_send, "{\"worker_queue\":\"%s\",\"worker_class\":\"%s\",\"worker_args\":[%s]}\
								", OBSERVER_QUEUE, OBSERVER_CLASS, final_args);
			free(final_args);
			curl_put(path, final_send);
			free(final_send);
		}
	}

	free(path);
	return 0;
}

int api_jobs_logs(const char *key, const char *buf) {
	char *path;

	if(api_url == NULL) {
		return -1;
	}

	path = malloc(strlen(api_url) + strlen(API_JOBS_LOGS) + 1);
	sprintf(path, "%s%s", api_url, API_JOBS_LOGS);

	int len = strlen(buf), i;
	char *escaped_buf = malloc(len * 2);
	char *p = escaped_buf;

	for(i = 0; i<len; i++) {
		char c = buf[i];
		switch(c) {
			case '"': p += sprintf(p, "\\\""); break;
			case '\\': p += sprintf(p, "\\\\"); break;
			case '\b': p += sprintf(p, "\\b"); break;
			case '\f': p += sprintf(p, "\\f"); break;
			case '\n': p += sprintf(p, "\\n"); break;
			case '\r': p += sprintf(p, "\\r"); break;
			case '\t': p += sprintf(p, "\\t"); break;
			default: *(p++) = c; break;
		}
	}
	*p = '\0';

	char *payload = malloc(strlen(escaped_buf) + strlen(API_LOGS_PAYLOAD) + strlen(key) + 1);
	sprintf(payload, API_LOGS_PAYLOAD, key, escaped_buf);
	free(escaped_buf);

	curl_put(path, payload);
	free(payload);
	free(path);

	return 0;
}

/*int api_jobs_shift(char **buf) {
	FILE *a;
	*buf = malloc(809);
	a = fopen("test.api", "r");
	fread(*buf, 808, 1, a);
	(*buf)[808] = '\0';
	fclose(a);
	return 0;
}*/
