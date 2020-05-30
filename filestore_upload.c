#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <curl/curl.h>
#include "filestore_upload.h"

int filestore_upload(char **results) {
	DIR *output_dir = opendir(builder_config.output_dir);
	if (output_dir == NULL) {
		log_printf(LOG_ERROR, "Error openning output directory %s: %s\n", builder_config.output_dir, strerror(errno));
		return -1;
	}

	*results = NULL;
	char *res = xmalloc(3);
	int sz = 1;
	res[0] = '[';
	while(1) {
		errno = 0;
		struct dirent *entry = readdir(output_dir);
		if (entry == NULL) {
			if (errno != 0) {
				free(res);
				log_printf(LOG_ERROR, "Error reading directory %s: %s\n", builder_config.output_dir, strerror(errno));
				closedir(output_dir);
				return -1;
			}
			break;
		}
		char *filename = alloc_sprintf("%s/%s", builder_config.output_dir, entry->d_name);
		struct stat fileinfo;
		if(stat(filename, &fileinfo) < 0) {
			free(res);
			log_printf(LOG_ERROR, "stat: Path %s: %s\n", filename, strerror(errno));
			free(filename);
			return -1;
		}
		if (!S_ISREG(fileinfo.st_mode)) {
			free(filename);
			continue;
		}
		char *json = NULL;
		upload_file(entry->d_name, fileinfo.st_size, filename, &json);
		if (json != NULL) {
			int len = strlen(json);
			res = xrealloc(res, sz + len + 2);
			sprintf(res + sz, "%s,", json);
			sz += len + 1;
			res[sz] = '\0';
			free(json);
		}
		unlink(filename);
		free(filename);
	}
	if (sz == 1) {
		res[1] = ']';
		res[2] = '\0';
	} else {
		res[sz - 1] = ']';
	}
	*results = res;
	log_printf(LOG_DEBUG, "Results: %s\n", res);
	return 0;
}

static void upload_file(const char *name, off_t size, const char *path, char **json) {
	CURL *curl;
	curl_mime *form = NULL;
  curl_mimepart *field = NULL;
	char errbuf[CURL_ERROR_SIZE];
	int sleep_time = 1;
	errbuf[0] = '\0';

	for (int try = 0; try < RETRIES; try++) {
		log_printf(LOG_INFO, "Try %d: Uploading file %s\n", try + 1, path);
		curl = curl_easy_init();

		form = curl_mime_init(curl);

		field = curl_mime_addpart(form);
		curl_mime_name(field, "file_store[file]");
		curl_mime_filedata(field, path);
		char *escaped_name = curl_easy_escape(curl, name, 0);
		curl_mime_filename(field, escaped_name);

		mem_t response;
		response.ptrs.write_ptr = NULL;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_USERNAME, builder_config.api_token);
		curl_easy_setopt(curl, CURLOPT_URL, builder_config.file_store_url);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 128L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60L);

		CURLcode res = curl_easy_perform(curl);
		long code;
		int error = 0;
		if(res != CURLE_OK) {
			log_printf(LOG_ERROR, "libcurl: There was an error performing request:\n");
			log_printf(LOG_ERROR, "libcurl: Error code %d.\n", res);
			size_t len = strlen(errbuf);
			if (len) {
				log_printf(LOG_ERROR, "libcurl: %s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
			} else {
				log_printf(LOG_ERROR, "libcurl: %s\n", curl_easy_strerror(res));
			}
			error = 1;
		} else {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
			if (code != 201 && code != 422) {
				log_printf(LOG_ERROR, "libcurl: Received code %ld on request.\n", code);
				log_printf(LOG_ERROR, "libcurl: Response from server is %s.\n", response.ptrs.write_ptr);
				error = 1;
			}
		}
		curl_easy_cleanup(curl);
		curl_mime_free(form);
		curl_free(escaped_name);

		if (error == 1) {
			if (response.ptrs.write_ptr != NULL) {
				free(response.ptrs.write_ptr);
			}
			log_printf(LOG_ERROR, "Sleeping for %ds.\n", sleep_time);
			sleep(sleep_time);
			if (sleep_time < 16) {
				sleep_time *= 2;
			}
		} else {
			char *resp = response.ptrs.write_ptr;
			if (strlen(name) >= 4 && !strcmp(name + strlen(name) - 4, ".rpm")) {
				log_printf(LOG_INFO, "Try %d: File uploaded.\n", try + 1);
				free(resp);
				return;
			}
			int to_add = 0;
			char *start, *end;
			if (code == 201) {
				start = strstr(resp, ":\"");
				to_add = 2;
				end = strchr(start + 2, '"');
			} else {
				start = strchr(resp, '\'');
				to_add = 1;
				end = strchr(start + 1, '\'');
			}
			if (start == NULL || end == NULL) {
				log_printf(LOG_ERROR, "Should never happen but happened anyway, wtf: %s\n", resp);
				free(resp);
				continue;
			}
			*end = '\0';
			*json = alloc_sprintf(file_line, start + to_add, name, size / MEGABYTE);
			log_printf(LOG_INFO, "Try %d: File uploaded.\n", try + 1);
			log_printf(LOG_DEBUG, "Resulting json: %s\n", *json);
			return;
		}
	}
	log_printf(LOG_ERROR, "File upload failed.");
}
