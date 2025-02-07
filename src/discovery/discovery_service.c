#include "discovery_service.h"
#include "soapClient_minimal.h"
#include "../devicemgmt/device_service.h"
#include "../common/service_common.h"
#include "clogger.h"
#include "wsddapi.h"
#include "portable_thread.h"
#include "wsdd.nsmap"

struct soap * discovery_soap = NULL;
int DISCOVERY_QUIT_FLAG = 0;
int DISCOVERY_STARTED = 0;

typedef struct {
    int init_done;
    P_COND_TYPE finish_cond;
    P_COND_TYPE cond;
    P_MUTEX_TYPE lock;
} OnvifSoapInitData;

OnvifSoapInitData * init_data = NULL;

void * 
OnvifDiscoveryService__thread(void * event){
    c_log_set_thread_color(ANSI_COLOR_YELLOW, P_THREAD_ID);
    OnvifSoapInitData ** ptrdata = (OnvifSoapInitData **) event;
    OnvifSoapInitData * data = *ptrdata;

    P_THREAD_DETACH(P_THREAD_ID); 

    data->init_done = 1;
    P_COND_BROADCAST(data->cond);

    discovery_soap = ServiceCommon__soap_new1(SOAP_IO_UDP, disco_namespaces);
    // discovery_soap->proxy_host = "0.0.0.0";
    discovery_soap->connect_flags = SO_BROADCAST;
    SOAP_SOCKET m = soap_bind(discovery_soap, NULL, 3702, 100);
    if (!soap_valid_socket(m)) {
        C_ERROR("Failed to bind discovery socket on port 3702");
        goto exit;
    }

    struct ip_mreq mcast; 
    mcast.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(discovery_soap->master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast, sizeof(mcast))<0) {
        C_ERROR("UDP group membership failed");
        goto exit;
    }

    C_DEBUG("Discovery Service Listening on port 3702");
    // TODO send Hello;
    while (!DISCOVERY_QUIT_FLAG) { //TODO Implemented threaded response?
		if (soap_begin_serve(discovery_soap) && !DISCOVERY_QUIT_FLAG){ //Parse raw HTTP request
            if (discovery_soap->error >= SOAP_STOP){
                C_ERROR("Failed to parse discovery request...");
                continue;
            }
		}

        if(OnvifDiscoveryService__serve(discovery_soap) && !DISCOVERY_QUIT_FLAG && discovery_soap->error && discovery_soap->error < SOAP_STOP){
            print_soap_fault(discovery_soap);
            C_ERROR("Failed to serve discovery request...");
            continue;
        }
    }
    // TODO send Bye;
exit:
    C_DEBUG("Discovery service terminated");
    DISCOVERY_QUIT_FLAG = 0;
    DISCOVERY_STARTED = 0;
    soap_destroy(discovery_soap); // delete managed class instances 
    soap_end(discovery_soap);     // delete managed data and temporaries 
    soap_free(discovery_soap);    // finalize and delete the context
    discovery_soap = NULL;
    *ptrdata = NULL;

    P_COND_BROADCAST(data->finish_cond); //broadcast to potentially waiting threads
    free(data);
    P_THREAD_EXIT;
    return NULL; 
}

void 
OnvifDiscoveryService__start(){
    if(DISCOVERY_STARTED){
        C_ERROR("Discovery Service already running");
        return;
    }
    if(DISCOVERY_QUIT_FLAG){
        C_ERROR("Discovery Service busy stopping");
        return;
    }
    DISCOVERY_STARTED = 1;
    P_THREAD_TYPE tid;

    init_data = malloc(sizeof(OnvifSoapInitData));
    init_data->init_done = 0;
    P_COND_SETUP(init_data->cond);
    P_COND_SETUP(init_data->finish_cond);
    P_MUTEX_SETUP(init_data->lock);

    P_THREAD_CREATE(tid, (void*(*)(void*))OnvifDiscoveryService__thread, &init_data);

    C_TRACE("Waiting for Discovery Service");
    if(!init_data->init_done) { P_COND_WAIT(init_data->cond, init_data->lock); }
    P_COND_CLEANUP(init_data->cond);
    P_MUTEX_CLEANUP(init_data->lock);
}

void 
OnvifDiscoveryService__stop(){
    if(!DISCOVERY_STARTED){
        C_ERROR("Discovery Service isn't running");
        return;
    }
    if(DISCOVERY_QUIT_FLAG){
        C_ERROR("Discovery Service already stopping");
        return;
    }
    DISCOVERY_QUIT_FLAG = 1;
    if(discovery_soap){
        C_WARN("Stopping Discovery service");
        shutdown(discovery_soap->master, SHUT_RDWR);
        if(init_data != NULL) { P_COND_WAIT(init_data->finish_cond, init_data->lock); }
    }
}

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


/*
    Server-sided logic
*/
soap_wsdd_mode 
wsdd_event_Probe(struct soap *soap, const char *MessageID, const char *ReplyTo, const char *Types, const char *Scopes, const char *MatchBy, struct wsdd__ProbeMatchesType *ProbeMatches){

    unsigned char bytes[4];
    bytes[0] = soap->ip & 0xFF;
    bytes[1] = (soap->ip >> 8) & 0xFF;
    bytes[2] = (soap->ip >> 16) & 0xFF;
    bytes[3] = (soap->ip >> 24) & 0xFF;  

    C_DEBUG("Discovery Probe from %d.%d.%d.%d", (int)bytes[3], (int)bytes[2], (int)bytes[1], (int)bytes[0]);
	C_TRACE("  MessageID: %s\n", MessageID);
	C_TRACE("  ReplyTo: %s\n", ReplyTo);
	C_TRACE("  Types: %s\n", Types);
	C_TRACE("  Scopes: %s\n", Scopes);
	C_TRACE("  MatchBy: %s\n", MatchBy);

    char hostname_struct[1024];
    hostname_struct[1023] = '\0';
    gethostname(hostname_struct, 1023);
    struct hostent* h;
    h = gethostbyname(hostname_struct);

    //TODO Add extra match with IP address instead of hostname
    int hostname_len = strlen(h->h_name);
    int port_len = sizeof(char)*(int)log10(soap->proxy_port)+1;
    int proto_len = strlen("http");
    int path_len = strlen(ONVIF_DEVICE_SERVICE_PATH);
    char * service_endpoint = soap_malloc(soap, proto_len + 3 + hostname_len + 1 + port_len + path_len+1);
    strcpy(service_endpoint,"http");
    strcat(service_endpoint,"://");
    strcat(service_endpoint,h->h_name);
    strcat(service_endpoint,":");
    char bytestr[5];
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