#include "service_common.h"
#include "onvifsoap.nsmap"
#include "logging.h"
#include "clogger.h"
#include "httpmd5.h"

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
ServiceCommon__soap_new1(int type, struct Namespace * namespace){
    struct soap * soap = soap_new1(type);
    soap_set_namespaces(soap, namespace);
    soap_register_plugin(soap, http_da);
    soap_register_plugin(soap, http_md5);
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


char * 
ServiceCommon__generate_xaddr(struct soap * soap, char * path){
    char hostname_struct[1024];
    hostname_struct[1023] = '\0';
    gethostname(hostname_struct, 1023);
    struct hostent* h;
    h = gethostbyname(hostname_struct);

    //TODO Add extra match with IP address instead of hostname
    int hostname_len = strlen(h->h_name);
    int port_len = sizeof(char)*(int)log10(soap->proxy_port)+1;
    int proto_len = strlen("http");
    int path_len = strlen(path);
    char * service_endpoint = soap_malloc(soap, proto_len + 3 + hostname_len + 1 + port_len + path_len+1);
    strcpy(service_endpoint,"http");
    strcat(service_endpoint,"://");
    strcat(service_endpoint,h->h_name);
    strcat(service_endpoint,":");
    char bytestr[5];
    itoa(bytestr, soap->proxy_port, 10);
    strcat(service_endpoint,bytestr);
    strcat(service_endpoint,path);

    return service_endpoint;
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char* 
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

struct soap * 
ServiceCommon__soap_new(){
    return ServiceCommon__soap_new1(SOAP_IO_DEFAULT, onvifsoap_namespaces);
}


