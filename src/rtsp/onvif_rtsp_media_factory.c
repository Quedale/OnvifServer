#include "onvif_rtsp_media_factory.h"
#include "clogger.h"

/*
    Placeholder until ONVIF VideoSource feature is implemented
*/
typedef struct {
    char * emtpy;
} OnvifRtspMediaFactoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (OnvifRtspMediaFactory, OnvifRtspMediaFactory_, GST_TYPE_RTSP_ONVIF_MEDIA_FACTORY)

static void
OnvifRtspMediaFactory__init (OnvifRtspMediaFactory * self){

}

static void
OnvifRtspMediaFactory__class_init (OnvifRtspMediaFactoryClass * klass){

}

OnvifRtspMediaFactory *
OnvifRtspMediaFactory__new (void){
    return g_object_new (ONVIF_TYPE_RTSP_MEDIA_FACTORY, NULL);
}