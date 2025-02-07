#ifndef ONVIF_RTSP_MEDIA_FACTORY_H
#define ONVIF_RTSP_MEDIA_FACTORY_H

#include <gst/rtsp-server/rtsp-onvif-server.h>

G_BEGIN_DECLS

#define ONVIF_TYPE_RTSP_MEDIA_FACTORY OnvifRtspMediaFactory__get_type()
G_DECLARE_FINAL_TYPE (OnvifRtspMediaFactory, OnvifRtspMediaFactory_, ONVIF, RTSP_MEDIA_FACTORY, GstRTSPOnvifMediaFactory)

struct _OnvifRtspMediaFactory { GstRTSPOnvifMediaFactory parent_instance; };
struct _OnvifRtspMediaFactoryClass { GstRTSPOnvifMediaFactoryClass parent_class; };

OnvifRtspMediaFactory * OnvifRtspMediaFactory__new();

G_END_DECLS

#endif