#ifndef ONVIF_MEDIA_SERVICE_H
#define ONVIF_MEDIA_SERVICE_H

#include "stdsoap2.h"

#define ONVIF_MEDIA_SERVICE_VERSION_MAJOR 21
#define ONVIF_MEDIA_SERVICE_VERSION_MINOR 06

#define ONVIF_MEDIA_SERVICE_NAMESPACE "http://www.onvif.org/ver10/media/wsdl"
#define ONVIF_MEDIA_SERVICE_PATH "/onvif/media_service"

struct trt__Capabilities * OnvifMediaService__createCapabilities(struct soap * soap);

int OnvifMediaService__serve(struct soap *soap);

#endif