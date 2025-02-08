#include "onvif_rtsp_service.h"
#include "onvif_rtsp_media_factory.h"
#include "clogger.h"
#include "portable_thread.h"

/*
    Very ugly and rudimentary rtsp service implementation
    Will be impolementation once ONVIF Profiles feature is implemented
    1 RTSP server for each profile configuration
*/
typedef struct {
    int started;
    char * mount;
    GMainLoop * loop;
    P_THREAD_TYPE tid;
    GstRTSPServer * server;
    OnvifRtspMediaFactory * factory;
} OnvifRtspServicePrivate;

typedef struct {
    OnvifRtspService * service;
    int done;
    P_COND_TYPE cond;
    P_MUTEX_TYPE lock;
} OnvifRtspServiceInitData;

G_DEFINE_TYPE_WITH_PRIVATE (OnvifRtspService, OnvifRtspService_, G_TYPE_OBJECT)

static void
OnvifRtspService__dispose (GObject *object){
    OnvifRtspServicePrivate *priv = OnvifRtspService__get_instance_private (ONVIF_RTSP_SERVICE(object));
    if(priv->mount){
        free(priv->mount);
        priv->mount = NULL;
    }

    if(priv->server){
        gst_object_unref(priv->server);
        priv->server = NULL;
    }

    G_OBJECT_CLASS (OnvifRtspService__parent_class)->dispose (object);
}

static void
OnvifRtspService__class_init (OnvifRtspServiceClass *klass){
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = OnvifRtspService__dispose;

}

static void
OnvifRtspService__init (OnvifRtspService *self){
    OnvifRtspServicePrivate *priv = OnvifRtspService__get_instance_private (self);

    priv->mount = NULL;
    priv->started = FALSE;
    priv->server = gst_rtsp_onvif_server_new ();
    priv->factory = OnvifRtspMediaFactory__new ();

    /*
    * Securing the stream
    */
    char * user = "admin"; //Temporarely hardcoded value
    char * pass = "admin";
    C_DEBUG("Setting authentication for user : %s", user);
    gst_rtsp_media_factory_add_role (GST_RTSP_MEDIA_FACTORY(priv->factory), "admin",
        GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
        GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);

    /* make a new authentication manager */
    GstRTSPAuth * auth = gst_rtsp_auth_new ();
    gst_rtsp_auth_set_supported_methods (auth, GST_RTSP_AUTH_DIGEST);

    /* make admin token */
    GstRTSPToken * token = gst_rtsp_token_new (GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "admin", NULL);
    gst_rtsp_auth_add_digest (auth, user, pass, token);
    gst_rtsp_token_unref (token);

    /* set as the server authentication manager */
    gst_rtsp_server_set_auth (priv->server, auth);
    g_object_unref (auth);

    /*
    * Configure pipeline.
    */
    gst_rtsp_media_factory_set_shared (GST_RTSP_MEDIA_FACTORY(priv->factory), FALSE);
    gst_rtsp_media_factory_set_media_gtype (GST_RTSP_MEDIA_FACTORY(priv->factory), GST_TYPE_RTSP_ONVIF_MEDIA);
    gst_rtsp_media_factory_set_launch (GST_RTSP_MEDIA_FACTORY(priv->factory), "( "
        "videotestsrc ! video/x-raw,width=640,height=480,framerate=30/1 ! "
        "openh264enc ! h264parse ! rtph264pay name=pay0 pt=96 "
        "audiotestsrc ! audio/x-raw,rate=8000 ! "
        "alawenc ! rtppcmapay name=pay1 pt=97 " 
        ")");
}

OnvifRtspService * 
OnvifRtspService__new (){
    return g_object_new (ONVIF_TYPE_RTSP_SERVICE, NULL);
}

void * OnvirRtspService__thread(void * event){
    c_log_set_thread_color(ANSI_COLOR_GREEN, P_THREAD_ID);
    OnvifRtspServiceInitData * data = (OnvifRtspServiceInitData *) event;
    OnvifRtspServicePrivate *priv = OnvifRtspService__get_instance_private (data->service);
    
    //Keeping location pointer because data won't be value after cond broadcast
    priv->loop = g_main_loop_new (NULL, FALSE);
    data->done = 1;          //Flag that the context and loop are ready
    P_COND_BROADCAST(data->cond);
    
    char * service = gst_rtsp_server_get_service(priv->server);
    char * address = gst_rtsp_server_get_address(priv->server);
    C_DEBUG ("RTSP Stream ready at rtsp://%s:%s%s",address, service,priv->mount);
    free(service);
    free(address);
    g_main_loop_run (priv->loop);

    P_THREAD_EXIT;
    return NULL;

}

void OnvirRtspService__start(OnvifRtspService *self){
    OnvifRtspServicePrivate *priv = OnvifRtspService__get_instance_private (self);
    priv->started = TRUE;
    /*
    * Setting mount point
    */
    char * mount = "h264";
    priv->mount = malloc(strlen(mount)+2);
    priv->mount[0] = '/';
    strcpy(&priv->mount[1],mount);

    char * bind_addr = "0.0.0.0";
    char * bind_port = "8554";
    GstRTSPMountPoints * mounts = gst_rtsp_server_get_mount_points (priv->server);
    gst_rtsp_mount_points_add_factory (mounts, priv->mount, GST_RTSP_MEDIA_FACTORY(priv->factory));
    g_object_unref (mounts);
    gst_rtsp_server_set_address(priv->server,bind_addr);
    gst_rtsp_server_set_service(priv->server,bind_port);

    /*
    *   Start loop
    */
    gst_rtsp_server_attach (priv->server, NULL);
    // priv->loop = g_main_loop_new (NULL, FALSE);

    OnvifRtspServiceInitData data;
    data.done = 0;
    data.service = self;
    P_COND_SETUP(data.cond);
    P_MUTEX_SETUP(data.lock);

    P_THREAD_CREATE(priv->tid, OnvirRtspService__thread, &data);

    C_TRACE("Waiting for Gstreamer loop initialization");
    if(!data.done) { P_COND_WAIT(data.cond, data.lock); }
    P_COND_CLEANUP(data.cond);
    P_MUTEX_CLEANUP(data.lock);
}

void OnvirRtspService__stop(OnvifRtspService *self){
    OnvifRtspServicePrivate *priv = OnvifRtspService__get_instance_private (self);
    if(priv->loop){
        g_main_loop_quit(priv->loop);
        g_main_loop_unref(priv->loop);
        priv->loop = NULL;
        //Making sure the loop is finished so that session isn't destroyed while an event is running
        P_THREAD_JOIN(priv->tid);
        g_main_context_unref(g_main_context_default());
    }

    char * service = gst_rtsp_server_get_service(priv->server);
    char * address = gst_rtsp_server_get_address(priv->server);
    C_DEBUG ("RTSP Stream stopped at rtsp://%s:%s%s",address,service,priv->mount);
    free(service);
    free(address);
}

