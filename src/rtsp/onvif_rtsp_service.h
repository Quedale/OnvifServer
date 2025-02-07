#ifndef ONVIF_RTSP_SERVICE_H
#define ONVIF_RTSP_SERVICE_H

#include <glib-object.h>
#include "onvif_static_plugins.h"

G_BEGIN_DECLS

#define ONVIF_TYPE_RTSP_SERVICE OnvifRtspService__get_type()
G_DECLARE_FINAL_TYPE (OnvifRtspService, OnvifRtspService_, ONVIF, RTSP_SERVICE, GObject)

struct _OnvifRtspService { GObject parent_instance; };
struct _OnvifRtspServiceClass { GObjectClass parent_class; };

OnvifRtspService * OnvifRtspService__new();
void OnvirRtspService__start(OnvifRtspService *self);
void OnvirRtspService__stop(OnvifRtspService *self);

G_END_DECLS

#endif