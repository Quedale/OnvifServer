#include "soapClient_minimal.h"
#include "clogger.h"

/* 
    Verbatim copy from the soapClient.c generated source code, which is needed by the wsdd plugin.
    This is the only part we need from the client so we dont bother generating the extra overhead
*/
SOAP_FMAC1
const char*
SOAP_FMAC2
soap_url(struct soap *soap, const char *s, const char *t){
    if (!t || (*t != '/' && *t != '?') || strlen(s) + strlen(t) >= sizeof(soap->msgbuf))
        return s;
    strcpy(soap->msgbuf, s);
    strcat(soap->msgbuf, t);
    return soap->msgbuf;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__ProbeMatches(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ProbeMatchesType *wsdd__ProbeMatches){
    struct __wsdd__ProbeMatches soap_tmp___wsdd__ProbeMatches;
	if (soap_action == NULL)
		soap_action = "http://docs.oasis-open.org/ws-dd/ns/discovery/2009/01/ProbeMatches";
	soap_begin(soap);
	soap->encodingStyle = NULL;
	soap_tmp___wsdd__ProbeMatches.wsdd__ProbeMatches = wsdd__ProbeMatches;
	soap_set_version(soap, 2); /* SOAP1.2 */
	soap_serializeheader(soap);
	soap_serialize___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH){
    	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches, "-wsdd:ProbeMatches", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}

	if (soap_end_count(soap))
		return soap->error;

	if (soap_connect(soap, soap_url(soap, soap_endpoint, NULL), soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___wsdd__ProbeMatches(soap, &soap_tmp___wsdd__ProbeMatches, "-wsdd:ProbeMatches", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Hello(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__HelloType *wsdd__Hello){
	struct __wsdd__Hello soap_tmp___wsdd__Hello;
	if (soap_action == NULL)
		soap_action = "http://docs.oasis-open.org/ws-dd/ns/discovery/2009/01/Hello";
	soap_tmp___wsdd__Hello.wsdd__Hello = wsdd__Hello;
	soap_begin(soap);
	soap->encodingStyle = NULL; /* use SOAP literal style */
	soap_serializeheader(soap);
	soap_serialize___wsdd__Hello(soap, &soap_tmp___wsdd__Hello);
	if (soap_begin_count(soap))
		return soap->error;
	if ((soap->mode & SOAP_IO_LENGTH))
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___wsdd__Hello(soap, &soap_tmp___wsdd__Hello, "-wsdd:Hello", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___wsdd__Hello(soap, &soap_tmp___wsdd__Hello, "-wsdd:Hello", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 
soap_send___wsdd__Bye(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ByeType *wsdd__Bye){
	struct __wsdd__Bye soap_tmp___wsdd__Bye;
	if (soap_action == NULL)
		soap_action = "http://docs.oasis-open.org/ws-dd/ns/discovery/2009/01/Bye";
	soap_tmp___wsdd__Bye.wsdd__Bye = wsdd__Bye;
	soap_begin(soap);
	soap->encodingStyle = NULL; /* use SOAP literal style */
	soap_serializeheader(soap);
	soap_serialize___wsdd__Bye(soap, &soap_tmp___wsdd__Bye);
	if (soap_begin_count(soap))
		return soap->error;
	if ((soap->mode & SOAP_IO_LENGTH))
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put___wsdd__Bye(soap, &soap_tmp___wsdd__Bye, "-wsdd:Bye", "")
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	}
	if (soap_end_count(soap))
		return soap->error;
	if (soap_connect(soap, soap_endpoint, soap_action)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put___wsdd__Bye(soap, &soap_tmp___wsdd__Bye, "-wsdd:Bye", "")
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap_closesock(soap);
	return SOAP_OK;
}

/*
    Mandatory client-side stub for the wsdd plugin
*/
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