CC=cc
$CC -g --std=gnu99 -Wall -Wpedantic -Wextra -lconfig -lcurl -lssl -lcrypto -pthread -O2 alloc_sprintf.c mem_functions.c curl_callbacks.c filestore_upload.c parse_job_description.c system_with_output.c init.c dns_checker.c logger.c jsmn.c statistics.c live_inspector.c live_logger.c exec_build.c api.c openssl_threaded.c main.c -o builder
