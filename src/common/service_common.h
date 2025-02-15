#ifndef ONVIF_SERVICE_COMMON_H_ 
#define ONVIF_SERVICE_COMMON_H_

#include "httpda.h"
#include "stdsoap2.h"
#include SOAP_XSTRINGIFY(SOAP_H_FILE)

#define CONCAT(a,b) a ## b
#define BUILD_NO_METH_FUNC(a)  __##a
#define BUILD_NO_METH_REQUEST(a)  _##a 
#define BUILD_NO_METH_RESPONSE(a) CONCAT(_##a, Response)

#define FAULT_UNAUTHORIZED "\"http://www.onvif.org/ver10/error\":NotAuthorized"
#define FAULT_ACTIONNOTSUPPORTED "\"http://www.onvif.org/ver10/error\":ActionNotSupported"
#define ONVIF_NOT_SUPPORTED_FAULT soap_receiver_fault_subcode(soap, FAULT_ACTIONNOTSUPPORTED, NULL, NULL)
#define ONVIF_NOT_AUTHORIZED_FAULT ((!soap_receiver_fault_subcode(soap, FAULT_UNAUTHORIZED, NULL, NULL)) ? 401 : 401) //I dont know any better way to return a value from a macro

/*
*
* Simple macro to reallocate the size of a complex gsoap type
* This macro takes care of setting default value by calculating count by factoring pointer size and struct size
*
* Here's an example :
*     soap_instance_resize(soap,
*       NetworkInterface->IPv4->Config->Manual,
*       NetworkInterface->IPv4->Config->__sizeManual,
*       tt__PrefixedIPv4Address);
*/
#define soap_instance_resize(soap,ptr,newsize, type) \
        size_t og_count = *(size_t*)(((void**)&ptr[newsize-1]) - sizeof(void*)) / sizeof(struct type); \
        void * nptr = soap_realloc(soap, ptr,sizeof(struct type) * newsize); \
        if(nptr){ \
          ptr = nptr; \
          size_t new_count = sizeof(struct type) * newsize / sizeof(struct type); \
          for(size_t i=new_count;i>og_count;i--) \
              soap_default_##type(soap, (struct type *) &ptr[i-1]); \
        } else { \
          C_ERROR("Failed to realloc instance. Out-of-Memory?"); \
        }
            
#define ONVIF_DEFINE_METHOD(onvif_method) \
    SOAP_FMAC5 int SOAP_FMAC6  \
    BUILD_NO_METH_FUNC(onvif_method)(struct soap* soap, struct BUILD_NO_METH_REQUEST(onvif_method) * request, struct BUILD_NO_METH_RESPONSE(onvif_method) * response)

#define ONVIF_DEFINE_UNSECURE_METHOD(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method) {

#define ONVIF_DEFINE_NO_METHOD(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method){ \
      C_WARN(#onvif_method" method not implemented"); \
      return SOAP_NO_METHOD; \
    }

#define ONVIF_DEFINE_NOT_SUPPORTED(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method){ \
      C_DEBUG(#onvif_method" action not supported"); \
      return ONVIF_NOT_SUPPORTED_FAULT; \
    }

#define ONVIF_REALM "OnvifServerRealm"
#define ONVIF_DEFINE_SECURE_METHOD(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method) { \
    C_DEBUG(#onvif_method); \
    const char *username = soap_wsse_get_Username(soap); \
    const char *password = "admin"; /* TODO Fixed hardcoded value with credentials database */ \
    if (!soap->authrealm){ /*Initiating HTTP handshake*/ \
      C_TRACE(#onvif_method" No realm set"); \
      soap->authrealm = ONVIF_REALM; \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    } \
    if (!username || !strlen(username)){ \
      C_WARN(#onvif_method" Anonymous authentication attempt"); \
      soap_wsse_delete_Security(soap); \
      soap->authrealm = ONVIF_REALM; \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    } \
    if (strcmp(soap->authrealm, ONVIF_REALM)){ \
      C_WARN(#onvif_method" Invalid HTTP realm supplied '%s'",username); \
      soap->authrealm = ONVIF_REALM; \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    } \
    if (strcmp(soap->userid, username)){ \
      C_WARN(#onvif_method" Invalid HTTP userid supplied '%s'",username); \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    } \
    if (http_da_verify_post(soap, password)){ \
      C_WARN(#onvif_method" Invalid HTTP credentials supplied '%s'",username); \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    } \
    if (soap_wsse_verify_Password(soap, password)){ \
      C_WARN(#onvif_method" Invalid soap credentials supplied '%s'",username); /* This should technically not happen since http was successful */ \
      soap_wsse_delete_Security(soap); \
      /* TODO Optionally sign error */ \
      return ONVIF_NOT_AUTHORIZED_FAULT; \
    }

#define ONVIF_METHOD_RETURNVAL(val) \
      /* TODO Optionally sign message */ \
      return val; \
    }

int ServiceCommon__delete_pointer(struct soap *soap, struct soap_clist *p);

#define ServiceCommon__serialize_data(parent_soap, serializer_func, inret, data) \
    {struct soap *serializer_soap = ServiceCommon__soap_new(); \
    const char *inner_ret = NULL; \
    serializer_soap->os = &inner_ret; \
    int r = serializer_func(serializer_soap, data); \
    if (r) C_ERROR("Failed to created DeviceServiceCapabilities"); \
    serializer_soap->os = NULL; \
    soap_unlink(serializer_soap,inner_ret); \
    soap_destroy(serializer_soap); \
    soap_end(serializer_soap); \
    soap_free(serializer_soap); \
    size_t i = strcspn ((char*)inner_ret,"\n"); \
    inret = (char*) &inner_ret[i+1]; \
	struct soap_clist *cp = soap_link(parent_soap, 0, 1, ServiceCommon__delete_pointer); \
	if (!cp && parent_soap && 1 != SOAP_NO_LINK_TO_DELETE){ \
        SOAP_FREE(parent_soap,ret); \
		return SOAP_FATAL_ERROR; \
    } else if (cp){ \
		cp->ptr = (void*)inner_ret; \
    }}


#define print_soap_fault(soap) \
  if (soap_check_state(soap)){ \
    C_ERROR("Error: soap struct state not initialized"); \
  } else if (soap->error) { \
    const char **c, *v = NULL, *s, *d; \
    c = soap_faultcode(soap); \
    if (!*c) \
    { \
      soap_set_fault(soap); \
      c = soap_faultcode(soap); \
    } \
    if (soap->version == 2) \
      v = soap_fault_subcode(soap); \
    s = soap_fault_string(soap); \
    d = soap_fault_detail(soap); \
    C_ERROR("%s%d fault %s [%s]\n\"%s\"\nDetail: %s\n", soap->version ? "SOAP 1." : "Error ", soap->version ? (int)soap->version : soap->error, *c, v ? v : "no subcode", s ? s : "[no reason]", d ? d : "[no detail]"); \
  }

typedef struct {
    int rtsp_port;
} OnvifUserConfig;

struct soap * ServiceCommon__soap_new();
struct soap * ServiceCommon__soap_new1(int type, struct Namespace * namespace);
char * ServiceCommon__generate_xaddr(struct soap * soap, char * path);
char* itoa(char* result, int value, int base);
SOAP_FMAC1 void* SOAP_FMAC2 soap_realloc(struct soap *soap, void * ptr, size_t n);

#endif