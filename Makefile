CC=cc
CFLAGS=$(CFLAGS) -c --std=gnu99 -Wall -Wpedantic -Wextra -O2
LDFLAGS=$(LDFLAGS) -lconfig -lcurl -lssl -lcrypto -pthread
SOURCES=alloc_sprintf.c api.c curl_callbacks.c dns_checker.c exec_build.c filestore_upload.c init.c jsmn.c live_inspector.c live_logger.c logger.c main.c mem_functions.c openssl_threaded.c parse_job_description.c statistics.c system_with_output.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=builder

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o $(EXECUTABLE)
