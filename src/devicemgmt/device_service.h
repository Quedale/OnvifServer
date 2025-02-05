#ifndef ONVIF_DEVICE_SERVICE_H_ 
#define ONVIF_DEVICE_SERVICE_H_

#include "stdsoap2.h"

#define ONVIF_DEVICE_SERVICE_VERSION_MAJOR 23
#define ONVIF_DEVICE_SERVICE_VERSION_MINOR 12

#define ONVIF_DEVICE_SERVICE_NAMESPACE "http://www.onvif.org/ver10/device/wsdl"
#define ONVIF_DEVICE_SERVICE_PATH "/onvif/device_service"

int OnvifDeviceService__serve(struct soap *soap);

#endif