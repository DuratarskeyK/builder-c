#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <ctype.h>
#include "api.h"

static const char *token = NULL;
static const char *api_url = NULL;
static const char *query_string = NULL;

void init_api(const char *url, const char *tok, const char *qs) {
	curl_global_init(CURL_GLOBAL_ALL);
	token = tok;
	api_url = url;
	query_string = qs;
}

static int curl_get(const char *url, char **buf) {
	CURL *curl_handle;
	CURLcode res;
	char errbuf[CURL_ERROR_SIZE];
	errbuf[0] = 0;

	log_printf(LOG_DEBUG, "libcurl: Starting GET to %s\n", url);

	mem_t response;
	response.ptrs.write_ptr = NULL;

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_USERNAME, token);

	res = curl_easy_perform(curl_handle);
	if(res != CURLE_OK) {
		log_printf(LOG_ERROR, "libcurl: There was an error performing request:\n");
		log_printf(LOG_ERROR, "libcurl: Error code %d\n", res);
		size_t len = strlen(errbuf);
		if (len) {
			log_printf(LOG_ERROR, "libcurl: %s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
		} else {
			log_printf(LOG_ERROR, "%s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
		*buf = NULL;
		if (response.ptrs.write_ptr != NULL) {
			free(response.ptrs.write_ptr);
		}
		return 1;
	}
	else {
		log_printf(LOG_DEBUG, "libcurl: Request successful. Received %d bytes.\n", strlen(response.ptrs.write_ptr));
		log_printf(LOG_DEBUG, "Received body: %s\n", response.ptrs.write_ptr);
		*buf = response.ptrs.write_ptr;
	}

	curl_easy_cleanup(curl_handle);
	return 0;
}

static int curl_put(const char *url, const char *buf) {
	CURL *curl;
	CURLcode res;
	char errbuf[CURL_ERROR_SIZE];
	errbuf[0] = 0;

	log_printf(LOG_DEBUG, "libcurl: Starting PUT to %s\n", url);
	log_printf(LOG_DEBUG, "libcurl: Body size %d\n", strlen(buf));
	log_printf(LOG_DEBUG, "libcurl: Body contents: %s\n", buf);

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl = curl_easy_init();

	mem_t mem;
	mem.ptrs.read_ptr = buf;
	mem.offset = 0;

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(curl, CURLOPT_READDATA, &mem);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_PUT, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)strlen(buf));
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_USERNAME, token);

	res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	if(res != CURLE_OK) {
		log_printf(LOG_ERROR, "libcurl: There was an error performing request:\n");
		log_printf(LOG_ERROR, "libcurl: Error code %d\n", res);
		size_t len = strlen(errbuf);
		if (len) {
			log_printf(LOG_ERROR, "libcurl: %s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
		} else {
			log_printf(LOG_ERROR, "%s\n", curl_easy_strerror(res));
		}
		return 1;
	}
	log_printf(LOG_DEBUG, "libcurl: Request successful.\n");

	return 0;
}

int api_job_statistics(const char *payload) {
	char *path = alloc_sprintf("%s%s", api_url, API_JOBS_STATISTICS);

	if(curl_put(path, payload)) {
		free(path);
		return -1;
	}

	free(path);

	return 0;
}

int api_jobs_shift(char **buf) {
	if(api_url == NULL) {
		return -1;
	}

	char *path;
	if(query_string != NULL) {
		path = alloc_sprintf("%s%s?%s", api_url, API_JOBS_SHIFT, query_string);
	}
	else {
		path = alloc_sprintf("%s%s", api_url, API_JOBS_SHIFT);
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

	char *path = alloc_sprintf("%s%s" API_STATUS_QUERYSTRING, api_url, API_JOBS_STATUS, build_id);

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

int api_jobs_feedback(const char *buf) {
	if(api_url == NULL) {
		return -1;
	}

	char *path = alloc_sprintf("%s%s", api_url, API_JOBS_FEEDBACK);
	char *final_send = alloc_sprintf("{\"worker_queue\":\"%s\",\"worker_class\":\"%s\",\"worker_args\":[%s]}", OBSERVER_QUEUE, OBSERVER_CLASS, buf);
	curl_put(path, final_send);
	free(final_send);
	free(path);
	return 0;
}

int api_jobs_logs(const char *key, const char *buf) {
	if(api_url == NULL) {
		return -1;
	}

	char *path = alloc_sprintf("%s%s", api_url, API_JOBS_LOGS);

	int len = strlen(buf), i;
	char *escaped_buf = xmalloc(len * 2 + 1);
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
			default:
				if (isprint(c)) {
					*(p++) = c;
				}
		}
	}
	*p = '\0';

	char *payload = alloc_sprintf(API_LOGS_PAYLOAD, key, escaped_buf);
	free(escaped_buf);

	curl_put(path, payload);
	free(payload);
	free(path);

	return 0;
}
