#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"
#include "parse_job_description.h"

int parse_job_description(const char *js, char **build_id, int *ttl, char **distrib_type, char ***env) {
	jsmn_parser json_parser;
	jsmntok_t *json;
	int res;

	jsmn_init(&json_parser);

	res = jsmn_parse(&json_parser, js, strlen(js), NULL, 0);

	if(res > 0) {
		json = xmalloc(res * sizeof(jsmntok_t));
		jsmn_init(&json_parser);
		res = jsmn_parse(&json_parser, js, strlen(js), json, res);
	}
	else {
		return -1;
	}

	int i;

	for(i = 0; i<res; i++) {
		if(!strncmp(&js[json[i].start], "worker_args", strlen("worker_args")) && json[i+2].type == JSMN_OBJECT) {
			break;
		}
	}

	if(i == res) {
		return -2;
	}

	i += 3;

	int j;
	for(j = 0; j<res; j++) {
		if(!strncmp(&js[json[j].start], "cmd_params", strlen("cmd_params")) && json[j+1].type == JSMN_OBJECT) {
			break;
		}
	}

	if(j == res) {
		return -2;
	}

	int cmd_params_len = json[j+1].size, cur_env = 0;
	//cmd_params + 6 necessary parameters + NULL
	*env = xmalloc((cmd_params_len + 7) * sizeof(char *));

	while(i < res) {
		int param_len = json[i].end - json[i].start;
		const char *start = &js[json[i].start];
		if(COMPARE(start, "id", param_len)) {
			*build_id = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			i += 2;
		}
		else if(COMPARE(start, "distrib_type", param_len)) {
			*distrib_type = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			i += 2;
		}
		else if(COMPARE(start, "time_living", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
			*ttl = atoi(t);
			free(t);
			i+=2;
		}
		else if(COMPARE(start, "cmd_params", param_len)) {
			//skip jsmn_object straight to the first parameter
			i += 2;

			for(j = 0; j < cmd_params_len; j++, i+=2, cur_env++) {
				char *key = strndup(&js[json[i].start], json[i].end - json[i].start);
				if(json[i+1].type == JSMN_PRIMITIVE && js[json[i+1].start] == 'n') {
					(*env)[j] = make_env(key, "");
				}
				else {
					char *value = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
					(*env)[j] = make_env(key, value);
					free(value);
				}

				free(key);
			}
		}
		else if(COMPARE(start, "include_repos", param_len)) {
			int repos_len = json[i+1].size;
			char *repo_names = xmalloc(json[i+1].end-json[i+1].start), *repo_urls = xmalloc(json[i+1].end-json[i+1].start);
			char *prn = repo_names, *pru = repo_urls;
			i += 2;

			int f = 0;
			for(j = 0; j < 2 * repos_len; j++, i++) {
				char *t = strndup(&js[json[i].start], json[i].end - json[i].start);
				if(!f) {
					prn += sprintf(prn, "%s ", t);
					f = 1;
				}
				else {
					pru += sprintf(pru, "%s ", t);
					f = 0;
				}
				free(t);
			}

			(*env)[cur_env] = make_env("REPO_NAMES", repo_names);
			(*env)[cur_env+1] = make_env("REPO_URL", repo_urls);
			free(repo_urls);
			free(repo_names);
			cur_env += 2;
		}
		else if(COMPARE(start, "uname", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
			(*env)[cur_env++] = make_env("UNAME", t);
			free(t);
			i += 2;
		}
		else if(COMPARE(start, "email", param_len)) {
			char *t = strndup(&js[json[i+1].start], json[i+1].end - json[i+1].start);
			(*env)[cur_env++] = make_env("EMAIL", t);
			free(t);
			i += 2;
		}
		else if(COMPARE(start, "platform", param_len)) {
			int platform_len = json[i+1].size;
			i += 2;
			for(j = 0; j<platform_len; i+=2, j++) {
				if(COMPARE(&js[json[i].start], "arch", json[i].end-json[i].start)) {
					char *t = strndup(&js[json[i+1].start], json[i+1].end-json[i+1].start);
					(*env)[cur_env++] = make_env("PLATFORM_ARCH", t);
					free(t);
				}
			}
		}
		else {
			i++;
		}
	}

	free(json);

	(*env)[cur_env++] = make_env("HOME", builder_config.work_dir);
	(*env)[cur_env++] = NULL;

	return cur_env;
}

static char *make_env(const char *key, const char *value) {
	char *res = xmalloc(strlen(key) + strlen(value) + 2);

	sprintf(res, "%s=%s", key, value);

	return res;
}
