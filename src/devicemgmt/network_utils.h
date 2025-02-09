#ifndef ONVIF_NETWORK_UTILS_H_ 
#define ONVIF_NETWORK_UTILS_H_

#define IP_MAX_LEN 40 //IPv6 spec max + 1

int NetworkUtils__lookup_hostname (const char *host, char * hostname);
int NetworkUtils__lookup_ip (const char *host, char *result);

#endif