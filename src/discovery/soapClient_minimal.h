#ifndef ONVIF_SOAP_CLIENT_MIN_H_ 
#define ONVIF_SOAP_CLIENT_MIN_H_

#include "stdsoap2.h"
#include SOAP_XSTRINGIFY(SOAP_H_FILE)

SOAP_FMAC5 int SOAP_FMAC6 soap_send___wsdd__ProbeMatches(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct wsdd__ProbeMatchesType *wsdd__ProbeMatches);

#endif

