#include "service_common.h"
#include "onvifsoap.nsmap"
#include "logging.h"
#include "clogger.h"

SOAP_FMAC5 int SOAP_FMAC6 
SOAP_ENV__Fault(struct soap* soap, char *faultcode, char *faultstring, char *faultactor, struct SOAP_ENV__Detail *detail, struct SOAP_ENV__Code *SOAP_ENV__Code, struct SOAP_ENV__Reason *SOAP_ENV__Reason, char *SOAP_ENV__Node, char *SOAP_ENV__Role, struct SOAP_ENV__Detail *SOAP_ENV__Detail){
    C_DEBUG("SOAP_ENV__Fault - Not implemented");
    return SOAP_NO_METHOD;
}

int 
ServiceCommon__delete_pointer(struct soap *soap, struct soap_clist *p){
    free(p->ptr);
    return SOAP_OK;
}

struct soap * 
ServiceCommon__soap_new1(int type){
    struct soap * soap = soap_new1(type);
    soap_set_namespaces(soap, onvifsoap_namespaces);
    soap->bind_flags         = SO_REUSEADDR;
    char *debug_flag = NULL;

    if (( debug_flag =getenv( "ONVIF_DEBUG" )) != NULL )
        C_INFO("ONVIF_DEBUG variable set. '%s'",debug_flag) ;

    if(debug_flag){
        soap_register_plugin(soap, logging);
        soap_set_logging_outbound(soap,stdout);
        soap_set_logging_inbound(soap,stdout);
    }

    return soap;
}

struct soap * 
ServiceCommon__soap_new(){
    return ServiceCommon__soap_new1(SOAP_IO_DEFAULT);
}


