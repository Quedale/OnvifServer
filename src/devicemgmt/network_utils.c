#include "network_utils.h"
#include <stdlib.h>
#include <netdb.h>
#include "clogger.h"
#include <arpa/inet.h>

int
NetworkUtils__lookup_ip (const char *host, char * retval){
	struct addrinfo hints, *res, *result;
	void *ptr;
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	if (getaddrinfo (host, NULL, &hints, &result) != 0){
		C_ERROR ("Failed to lookup address info");
		return EXIT_FAILURE;
	}

	res = result;
	while (res){
		inet_ntop (res->ai_family, res->ai_addr->sa_data, retval, 100);
		switch (res->ai_family){
			case AF_INET:
				ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
				break;
			case AF_INET6:
				ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
				break;
		}
		inet_ntop (res->ai_family, ptr, retval, 100);
		// C_FATAL ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4, retval, res->ai_canonname);
		res = res->ai_next;
	}
  
	freeaddrinfo(result);
	return EXIT_SUCCESS;
}

int
NetworkUtils__lookup_hostname (const char *host, char * hostname){
    struct addrinfo *result, *res;
 
    /* resolve the domain name into a list of addresses */
	int error = getaddrinfo(host, NULL, NULL, &result);
    if (error != 0){
        C_ERROR("error in getaddrinfo: %s", gai_strerror(error));
        return EXIT_FAILURE;
    }   
 
    /* loop over all returned results and do inverse lookup */
    for (res = result; res != NULL; res = res->ai_next){
		error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error != 0){
            C_ERROR("error in getnameinfo: %s", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0'){
			//TODO Make sure its the right one
			freeaddrinfo(result);
			return EXIT_SUCCESS;
		}
    }
 
    freeaddrinfo(result);

	return EXIT_FAILURE;
}
