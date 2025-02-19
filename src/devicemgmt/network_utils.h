#ifndef ONVIF_NETWORK_UTILS_H_ 
#define ONVIF_NETWORK_UTILS_H_

#define IP_MAX_LEN 40 //IPv6 spec max + 1

#include <linux/rtnetlink.h>
#include "stdsoap2.h"

typedef enum {
	NETUTILS_NETLINK				= 1<<0,
	NETUTILS_NETADDRS				= 1<<1,
	NETUTILS_DUMP					= NETUTILS_NETLINK | NETUTILS_NETADDRS 
} NetworkUtilsQueryType;

typedef int (*NetworkUtilsQueryCallback) (struct soap * soap, void * user_data, struct nlmsghdr * msg);
#define NETWORK_UTILS_QUERY_CALLBACK(f) ((NetworkUtilsQueryCallback) (void (*)(void)) (f))

int NetworkUtils__query(struct soap * soap, void * user_data, NetworkUtilsQueryType qtype,  NetworkUtilsQueryCallback callback);
int NetworkUtils__lookup_hostname (const char *host, char * hostname);
int NetworkUtils__lookup_ip (const char *host, char *result);

#endif