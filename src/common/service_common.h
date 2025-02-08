#ifndef ONVIF_SERVICE_COMMON_H_ 
#define ONVIF_SERVICE_COMMON_H_

#include "stdsoap2.h"
#include SOAP_XSTRINGIFY(SOAP_H_FILE)

#define CONCAT(a,b) a ## b
#define BUILD_NO_METH_FUNC(a)  __##a
#define BUILD_NO_METH_REQUEST(a)  _##a 
#define BUILD_NO_METH_RESPONSE(a) CONCAT(_##a, Response)

#define ONVIF_DEFINE_METHOD(onvif_method) \
    SOAP_FMAC5 int SOAP_FMAC6  \
    BUILD_NO_METH_FUNC(onvif_method)(struct soap* soap, struct BUILD_NO_METH_REQUEST(onvif_method) * request, struct BUILD_NO_METH_RESPONSE(onvif_method) * response)

#define ONVIF_DEFINE_NO_METHOD(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method){ \
        C_WARN(#onvif_method" onvif_method not implemented"); \
        return SOAP_NO_METHOD; \
    }

#define ONVIF_DEFINE_SECURE_METHOD(onvif_method) \
    ONVIF_DEFINE_METHOD(onvif_method) { \
    C_DEBUG(#onvif_method); \
    const char *username = soap_wsse_get_Username(soap); \
    const char *password; \
    if (!username || !strlen(username)){ \
        C_WARN(#onvif_method" Anonymous authentication attempt"); \
        soap_wsse_delete_Security(soap); \
        return SOAP_FAULT; \
    } \
    password = "admin"; /* TODO Fixed hardcoded value with credentials database */ \
    if (soap_wsse_verify_Password(soap, password)){ \
        C_TRACE("Failed authentication attempt for user '%s'",username); \
        soap_wsse_delete_Security(soap); \
        /* TODO Optionally sign error */ \
        return SOAP_FAULT; \
    }

#define ONVIF_DEFINE_SECURE_METHOD_RETURNVAL(val) \
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

struct soap * ServiceCommon__soap_new();
struct soap * ServiceCommon__soap_new1(int type, struct Namespace * namespace);
char * ServiceCommon__generate_xaddr(struct soap * soap, char * path);
char* itoa(char* result, int value, int base);

#endif