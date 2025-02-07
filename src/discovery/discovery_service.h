#ifndef ONVIF_DISCOVERY_SERVICE_H_ 
#define ONVIF_DISCOVERY_SERVICE_H_

#include "stdsoap2.h"

int OnvifDiscoveryService__serve(struct soap *soap);

void OnvifDiscoveryService__start();
void OnvifDiscoveryService__stop();

#endif