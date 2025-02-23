#include "service_common.h"
#include "onvifsoap.nsmap"
#include "logging.h"
#include "clogger.h"
#include "httpmd5.h"

#ifndef SOAP_CANARY
# define SOAP_CANARY (0xC0DE)
#endif

/*
*
* Reallocation implementation for gsoap. It is important to invoke soap_default_NS_Type(soap,instance) 
* after reallocation to avoid clean up problems associated to mandatory fields.
* The orignalsize paramter is useful to determine the newly allocated space to initialize new soap data
*
* The macro "soap_instance_resize" in service_common.h is available for cleaner code
*
* Here's a simple example for NetworkInterfaces incrementing allocation count by 1
* 	response->__sizeNetworkInterfaces++;
*	struct tt__NetworkInterface * NetworkInterface;
*	if(!response->NetworkInterfaces){
*		response->NetworkInterfaces = soap_new_tt__NetworkInterface(soap, 1);
*		NetworkInterface = response->NetworkInterfaces;
*	} else {
*		response->NetworkInterfaces = soap_realloc(soap, response->NetworkInterfaces,sizeof(struct tt__NetworkInterface) * response->__sizeNetworkInterfaces);
*		NetworkInterface = &response->NetworkInterfaces[response->__sizeNetworkInterfaces-1];
*       soap_default_tt__NetworkInterface(soap, NetworkInterface);  <----- This is the important part otherwise you will have to populate the entire object manually
*   }
*
* Here's a simple example for NetworkInterfaces incrementing allocation count by n amount
*       size_t original_size;
*       int newsize = n;
*		response->NetworkInterfaces = soap_realloc(soap, response->NetworkInterfaces,sizeof(struct tt__NetworkInterface) * newsize, &original_size);
*       for(size_t i=newsize;i>original_size;i--) \
*           soap_default_tt__NetworkInterface(soap, (struct tt__NetworkInterface *) &response->NetworkInterfaces[i-1]); \
*/
SOAP_FMAC1 void* SOAP_FMAC2
soap_realloc(struct soap *soap, void * ptr, size_t n, size_t * orignalsize){
    char *p;
    size_t k = n;
    if (SOAP_MAXALLOCSIZE > 0 && n > SOAP_MAXALLOCSIZE){
        if (soap)
            soap->error = SOAP_EOM;
        return NULL;
    }
    if (!soap)
        return realloc(ptr, n);

    //Add mandatory extra padding
    n += sizeof(short);
    n += (~n+1) & (sizeof(void*)-1); /* align at 4-, 8- or 16-byte boundary by rounding up */
    if (n + sizeof(void*) + sizeof(size_t) < k){
        soap->error = SOAP_EOM;
        return NULL;
    }

    //Search pointer if memory alloaction list
    char **q;
    for (q = (char**)(void*)&soap->alist; *q; q = *(char***)q){
        if (*(unsigned short*)(char*)(*q - sizeof(unsigned short)) != (unsigned short)SOAP_CANARY){
            C_ERROR("Data corruption in dynamic allocation");
            soap->error = SOAP_MOE;
            return NULL;
        }
        if (ptr == (void*)(*q - *(size_t*)(*q + sizeof(void*)))){
            break;
        }
    }

    if (*q){
        //Extract original allocation size
        size_t og_size = *(size_t*)(*q + sizeof(void *));

        //Reattach broken pointer chain
        if(*(char***)q)
            *q = **(char***)q;

        //Shift original size to exclude original size and canary value
        og_size -= sizeof(void*) - sizeof(size_t) - sizeof(unsigned short); //Handle round up?
        if(orignalsize){
            *orignalsize = og_size;
        }

        //Rellocation (define macro?)
        p = (char*) realloc(ptr,n + sizeof(void*) + sizeof(size_t));
        if(!p){
            C_ERROR("Data corruption in dynamic allocation");
            soap->error = SOAP_MOE;
            return NULL;
        }

        /*
        *   Initialize new memory allocation while leaving original data intact
        *   This is the equivalent to soap_default_ns__Type(...)
        */
        memset(p + og_size, 0, n-og_size);

       /* set a canary word to detect memory overruns and data corruption */
       *(unsigned short*)((char*)p + n - sizeof(unsigned short)) = (unsigned short)SOAP_CANARY;

       /* keep chain of alloced cells for destruction */
       *(void**)(p + n) = soap->alist;
       *(size_t*)(p + n + sizeof(void*)) = n;
       soap->alist = p + n;

       return (void*)p;
   }
   
   C_ERROR("Pointer not found in memory allocation cache");
   soap->error = SOAP_SVR_FAULT;
   return NULL;
}

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


