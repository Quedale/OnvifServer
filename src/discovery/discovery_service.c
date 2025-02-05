#include "discovery_service.h"
#include "soapClient_minimal.h"
#include "../devicemgmt/device_service.h"
#include "clogger.h"
#include "wsddapi.h"
#include "wsaapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <byteswap.h>

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
static char* 
itoa(char* result, int value, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

static char *
get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen){
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),s, maxlen);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),s, maxlen);
            break;
        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }
    return s;
}

/*
    Server-sided logic
*/
soap_wsdd_mode 
wsdd_event_Probe(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *Types, const char *Scopes, const char *MatchBy, struct wsdd__ProbeMatchesType *ProbeMatches){
    int ipv4_max_len = 15;
    char client_ip[ipv4_max_len];
    get_ip_str(&soap->peer.addr,client_ip,ipv4_max_len);
    C_DEBUG("ONVIF Discovery Probe from %s", client_ip);
	C_TRACE("  MessageID: %s\n", MessageID);
	C_TRACE("  ReplyTo: %s\n", ReplyTo);
	C_TRACE("  Types: %s\n", Types);
	C_TRACE("  Scopes: %s\n", Scopes);
	C_TRACE("  MatchBy: %s\n", MatchBy);

    unsigned char bytes[4];
    bytes[0] = soap->ip & 0xFF;
    bytes[1] = (soap->ip >> 8) & 0xFF;
    bytes[2] = (soap->ip >> 16) & 0xFF;
    bytes[3] = (soap->ip >> 24) & 0xFF;  

    int port_len = sizeof(char)*(int)log10(soap->proxy_port)+1;
    int proto_len = strlen("http");
    int path_len = strlen(ONVIF_DEVICE_SERVICE_PATH);
    char * service_endpoint = soap_malloc(soap, proto_len + 3 + ipv4_max_len + 1 + port_len + path_len+1);
    strcpy(service_endpoint,"http");
    strcat(service_endpoint,"://");

    char bytestr[5];
    itoa(bytestr, (int)bytes[3], 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,".");

    itoa(bytestr, (int)bytes[2], 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,".");

    itoa(bytestr, (int)bytes[1], 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,".");

    itoa(bytestr, (int)bytes[0], 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,":");

    itoa(bytestr, soap->proxy_port, 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,ONVIF_DEVICE_SERVICE_PATH);

    soap_wsdd_add_ProbeMatch(soap, ProbeMatches,
        soap_wsa_rand_uuid(soap), //EndpointReference
        "tdn:NetworkVideoTransmitter", //Types
        NULL, //Scopes
        NULL, //MatchBy
        service_endpoint, //XAddrs
        10);  //MetadataVersion
                    
    //TODO Fill is client manual probe request
    return SOAP_WSDD_MANAGED;
}

void wsdd_event_Hello(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int MetadataVersion)
{ C_WARN("wsdd_event_Hello"); }

void wsdd_event_Bye(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, const char *EndpointReference, const char *Types, const char *Scopes, const char *MatchBy, const char *XAddrs, unsigned int *MetadataVersion)
{ C_WARN("wsdd_event_Bye"); }

void wsdd_event_ProbeMatches(struct soap *soap, unsigned int InstanceId, const char *SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ProbeMatchesType *ProbeMatches)
{C_WARN("wsdd_event_ProbeMatches");}

void wsdd_event_ResolveMatches(struct soap *soap, unsigned int InstanceId, const char * SequenceId, unsigned int MessageNumber, const char *MessageID, const char *RelatesTo, struct wsdd__ResolveMatchType *match)
{ C_WARN("wsdd_event_ResolveMatches"); }

soap_wsdd_mode 
wsdd_event_Resolve(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *EndpointReference, struct wsdd__ResolveMatchType *match){
    C_WARN("wsdd_event_Resolve");
    return SOAP_WSDD_ADHOC;
}

/*
    Pretty sure this is a legacy wsdl definition (2010?)
*/
SOAP_FMAC5 int SOAP_FMAC6 
__tdn__Hello(struct soap* soap, struct wsdd__HelloType tdn__Hello, struct wsdd__ResolveType *tdn__HelloResponse){
    C_WARN("__tdn__Hello");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
__tdn__Bye(struct soap* soap, struct wsdd__ByeType tdn__Bye, struct wsdd__ResolveType *tdn__ByeResponse){
    C_WARN("__tdn__Bye");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
__tdn__Probe(struct soap* soap, struct wsdd__ProbeType tdn__Probe, struct wsdd__ProbeMatchesType *tdn__ProbeResponse){
    C_WARN("__tdn__Probe");
    return 0;
}

/*
    Mandatory client-side stub for the wsdd plugin
*/
SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Hello(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__HelloType *wsdd__Hello){
    C_WARN("soap_send___wsdd__Hello");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Bye(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ByeType *wsdd__Bye){
    C_WARN("soap_send___wsdd__Bye");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Probe(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ProbeType *wsdd__Probe){
    C_WARN("soap_send___wsdd__Probe");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Resolve(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ResolveType *wsdd__Resolve){
    C_WARN("soap_send___wsdd__Resolve");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_recv___wsdd__ProbeMatches(struct soap *soap, struct __wsdd__ProbeMatches * probe_matches){
    C_WARN("soap_recv___wsdd__ProbeMatches");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_recv___wsdd__ResolveMatches(struct soap *soap, struct __wsdd__ResolveMatches * resolve_matches){
    C_WARN("soap_recv___wsdd__ResolveMatches");
    return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__ResolveMatches(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ResolveMatchesType *wsdd__ResolveMatches){
    C_WARN("soap_send___wsdd__ResolveMatches");
    return 0;
}

int  
OnvifDiscoveryService__serve(struct soap *soap){
	(void)soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:Hello"))
		return soap_serve___wsdd__Hello(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:Bye"))
		return soap_serve___wsdd__Bye(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:Probe"))
		return soap_serve___wsdd__Probe(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:ProbeMatches"))
		return soap_serve___wsdd__ProbeMatches(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:Resolve"))
		return soap_serve___wsdd__Resolve(soap);
	if (!soap_match_tag(soap, soap->tag, "wsdd:ResolveMatches"))
		return soap_serve___wsdd__ResolveMatches(soap);
	if (!soap_match_tag(soap, soap->tag, "tdn:Hello"))
		return soap_serve___tdn__Hello(soap);
	if (!soap_match_tag(soap, soap->tag, "tdn:Bye"))
		return soap_serve___tdn__Bye(soap);
	if (!soap_match_tag(soap, soap->tag, "tdn:Probe"))
		return soap_serve___tdn__Probe(soap);
	return soap->error = SOAP_NO_METHOD;
}