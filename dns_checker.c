#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "dns_checker.h"

static struct addrinfo hints, *infoptr;

int check_dns() {
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	for (int try = 0; try < DNS_RETRIES; try++) {
	    log_printf(LOG_INFO, "Try #%d: Resolving DNS.\n", try + 1);
	
	    int result = getaddrinfo("github.com", NULL, &hints, &infoptr);
	    if (result) {
		    if ( try < DNS_RETRIES ) {
			log_printf(LOG_INFO, "getaddrinfo: %s\n Retrying DNS after 5 seconds.\n", gai_strerror(result));
			sleep(5);
		    } else {
			    log_printf(LOG_ERROR, "getaddrinfo: %s\n", gai_strerror(result));
			    return -1;
			}
	    } else {
		    try = DNS_RETRIES;
		}
	}

	log_printf(LOG_DEBUG, "github.com was resolved to:\n");

	struct addrinfo *p;
	char host[256];

	for (p = infoptr; p != NULL; p = p->ai_next) {
		getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof (host), NULL, 0, NI_NUMERICHOST);
		log_printf(LOG_DEBUG, "%s\n", host);
	}

	freeaddrinfo(infoptr);

	return 0;
}
