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
P_THREAD_TYPE MASTER_TID = 0;

#define ONVIF_NVT_TYPE "\"http://www.onvif.org/ver10/network/wsdl\":NetworkVideoTransmitter"

struct soap *
OnvifDiscoveryService__new_soap(){
    struct soap * soap = ServiceCommon__soap_new1(SOAP_IO_UDP, disco_namespaces);
    // discovery_soap->proxy_host = "0.0.0.0";
    soap->connect_flags = SO_BROADCAST;
    SOAP_SOCKET m = soap_bind(soap, NULL, 3702, 100);
    if (!soap_valid_socket(m)) {
        C_ERROR("Failed to bind discovery socket on port 3702");
        return NULL;
    }

    struct ip_mreq mcast; 
    mcast.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(soap->master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast, sizeof(mcast))<0) {
        C_ERROR("UDP group membership failed");
        return NULL;
    }
    
    return soap;
}

void 
OnvifDiscoveryService__Hello(){
    C_DEBUG("Sending Discovery Hello");
    struct soap * soap = OnvifDiscoveryService__new_soap();
    if(!soap) goto exit;

    if(soap_wsdd_Hello(soap,
        SOAP_WSDD_ADHOC,      //Mode
        NULL, //Endpoint
        soap_wsa_rand_uuid(soap),   //message id
        NULL, //RelatesTo
        NULL, //EndpointReference
        ONVIF_NVT_TYPE, //Type
        NULL, // Scopes TODO Fill scope
        NULL,        // MatchBy
        ServiceCommon__generate_xaddr(soap,ONVIF_DEVICE_SERVICE_PATH),
        10u) != SOAP_OK){
            print_soap_fault(soap);
            C_ERROR("Failed to send bye.");
    }
exit:
    soap_destroy(soap); // delete managed class instances 
    soap_end(soap);     // delete managed data and temporaries 
    soap_free(soap);    // finalize and delete the context
}

/*
    Bye has its own soap since the original soap closes the socket on shutdown
*/
void 
OnvifDiscoveryService__Bye(){
    C_DEBUG("Sending Discovery Bye");
    struct soap * soap = OnvifDiscoveryService__new_soap();
    if(!soap) goto exit;

    if(soap_wsdd_Bye(soap,
        SOAP_WSDD_ADHOC,      //Mode
        NULL, //Endpoint
        soap_wsa_rand_uuid(soap),   //message id
        NULL, //ReplyTo
        ONVIF_NVT_TYPE, //Type
        NULL, // Scopes TODO Fill scope
        NULL,        // MatchBy
        ServiceCommon__generate_xaddr(soap,ONVIF_DEVICE_SERVICE_PATH),
        10u) != SOAP_OK){
            print_soap_fault(soap);
            C_ERROR("Failed to send bye.");
    }
exit:
    soap_destroy(soap); // delete managed class instances 
    soap_end(soap);     // delete managed data and temporaries 
    soap_free(soap);    // finalize and delete the context
}

void * 
OnvifDiscoveryService__thread(void * nul){
    c_log_set_thread_color(ANSI_COLOR_YELLOW, P_THREAD_ID);

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

exit:
    C_DEBUG("Discovery service terminated");
    DISCOVERY_QUIT_FLAG = 0;
    DISCOVERY_STARTED = 0;
    soap_destroy(discovery_soap); // delete managed class instances 
    soap_end(discovery_soap);     // delete managed data and temporaries 
    soap_free(discovery_soap);    // finalize and delete the context
    discovery_soap = NULL;
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
    P_THREAD_CREATE(MASTER_TID, (void*(*)(void*))OnvifDiscoveryService__thread, NULL);
    OnvifDiscoveryService__Hello();
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
        OnvifDiscoveryService__Bye();
        shutdown(discovery_soap->master, SHUT_RDWR);
        P_THREAD_JOIN(MASTER_TID);
        MASTER_TID = 0;
    }
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

    soap_wsdd_add_ProbeMatch(soap, ProbeMatches,
        soap_wsa_rand_uuid(soap), //EndpointReference
        "tdn:NetworkVideoTransmitter", //Types
        NULL, //Scopes
        NULL, //MatchBy
        ServiceCommon__generate_xaddr(soap,ONVIF_DEVICE_SERVICE_PATH), //XAddrs
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