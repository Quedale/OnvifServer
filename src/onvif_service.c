#include "onvif_service.h"
#include "common/service_common.h"
#include "devicemgmt/device_service.h"
#include "media/media_service.h"
#include "portable_thread.h"
#include "clogger.h"

struct soap * onvif_soap = NULL;
int ONVIF_QUIT_FLAG = 0;
int ONVIF_STARTED = 0;

void *
OnvifService__thread(void* tsoap) {
    c_log_set_thread_color(ANSI_COLOR_CYAN, P_THREAD_ID);
    struct soap *soap = (struct soap*)tsoap;
    P_THREAD_DETACH(P_THREAD_ID); 

    int (*serv_func)(struct soap*);

	soap->keep_alive = soap->max_keep_alive + 1;
	do{
		if (soap->keep_alive > 0 && soap->max_keep_alive > 0)
			soap->keep_alive--;
		if (soap_begin_serve(soap)){ //Parse raw HTTP request
            if (soap->error >= SOAP_STOP)
                continue;
			return NULL;
		}

        if(!strcmp(soap->path,ONVIF_DEVICE_SERVICE_PATH)){
            serv_func = OnvifDeviceService__serve;
        } else if(!strcmp(soap->path,ONVIF_MEDIA_SERVICE_PATH)){
            serv_func = OnvifMediaService__serve;
        } else {
            C_ERROR("404 service not found.");
            soap_send_empty_response (soap, 404);
            return NULL;
        }

		if ((serv_func(soap)) && !ONVIF_QUIT_FLAG && soap->error && soap->error < SOAP_STOP)
			soap_send_fault(soap);
	} while (soap->keep_alive);

    soap_destroy(soap); // delete managed class instances 
    soap_end(soap);     // delete managed data and temporaries 
    soap_free(soap);    // finalize and delete the context
    P_THREAD_EXIT;
    return NULL; 
}


void OnvifService__start(int port){
    if(ONVIF_STARTED){
        C_ERROR("HTTP Service already running");
        return;
    }
    if(ONVIF_QUIT_FLAG){
        C_ERROR("HTTP Service busy stopping");
        return;
    }
    ONVIF_STARTED = 1;

    onvif_soap = ServiceCommon__soap_new();

    onvif_soap->accept_timeout = 24*60*60;             /* quit after 24h of inactivity (optional) */
    onvif_soap->send_timeout = onvif_soap->recv_timeout = 5; /* max send and receive socket inactivity time (sec) */
    onvif_soap->transfer_timeout = 10;
    SOAP_SOCKET m, s;
    struct soap *tsoap;
    P_THREAD_TYPE tid;
    
    // onvif_soap->proxy_host = "0.0.0.0";
    m = soap_bind(onvif_soap, NULL, port, 2);          /* small backlog queue of 2 to 10 for iterative servers */
    
    if (!soap_valid_socket(m)) {
        print_soap_fault(onvif_soap); 
        C_FATAL("Failed to bind socket on port %d",port);
        exit(EXIT_FAILURE);
    }

    C_DEBUG("HTTP Service Listening on port %d",port);
    while (!ONVIF_QUIT_FLAG){
        s = soap_accept(onvif_soap); 
        if (soap_valid_socket(s)){
            tsoap = soap_copy(onvif_soap);
            if (!tsoap) 
                soap_force_closesock(onvif_soap);
            else
                while (P_THREAD_CREATE(tid, (void*(*)(void*))OnvifService__thread, (void*)tsoap))
                    sleep(1); // failed, try again

        } else if (onvif_soap->errnum && !ONVIF_QUIT_FLAG){
            print_soap_fault(onvif_soap); 
            sleep(1);
        } else if (!ONVIF_QUIT_FLAG) {
            C_ERROR("HTTP Server timed out"); 
            break; 
        } 
    }

    soap_destroy(onvif_soap); /* delete deserialized objects */
    soap_end(onvif_soap);     /* delete heap and temp data */
    soap_free(onvif_soap);    /* we're done with the context */
    onvif_soap = NULL;
    ONVIF_STARTED = 0;
    ONVIF_QUIT_FLAG = 0;
    C_INFO("HTTP Service terminated");
}

void OnvifService__stop(){
    if(!ONVIF_STARTED){
        C_ERROR("HTTP Service isn't running");
        return;
    }
    if(ONVIF_QUIT_FLAG){
        C_ERROR("HTTP Service already stopping");
        return;
    }
    ONVIF_QUIT_FLAG = 1;
    if(onvif_soap){
        C_WARN("Stopping HTTP Service");
        shutdown(onvif_soap->master, SHUT_RDWR);
    }
}