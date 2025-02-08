#include "onvif_service.h"
#include "common/service_common.h"
#include "devicemgmt/device_service.h"
#include "media/media_service.h"
#include "portable_thread.h"
#include "clogger.h"

struct soap * onvif_soap = NULL;
int ONVIF_QUIT_FLAG = 0;
int ONVIF_STARTED = 0;

#define POOLSIZE  (64)      /* size of the thread pool */
#define QUEUESIZE (1000)    /* max number of waiting jobs in the queue */

static SOAP_SOCKET jobs[QUEUESIZE]; /* job queue with head and tail */
static int head = 0, tail = 0;
static P_MUTEX_TYPE queue_lock;    /* mutex for queue ops critical sections */
static P_COND_TYPE queue_notempty; /* condition variable when queue is empty */
static P_COND_TYPE queue_notfull;  /* condition variable when queue is full */
static P_THREAD_TYPE tids[POOLSIZE];                       /* thread pool */
static int MASTER_TID = 0;

/* add job (socket with pending request) to queue */
void 
enqueue(SOAP_SOCKET s){
  int next;
  P_MUTEX_LOCK(queue_lock);
  next = (tail + 1) % QUEUESIZE;
  while (next == head)
    P_COND_WAIT(queue_notfull, queue_lock);
  jobs[tail] = s;
  tail = next;
  P_COND_SIGNAL(queue_notempty);
  P_MUTEX_UNLOCK(queue_lock);
}

/* remove job (socket with request) from queue */
SOAP_SOCKET 
dequeue(){
  SOAP_SOCKET s;
  P_MUTEX_LOCK(queue_lock);
  while (head == tail)
    P_COND_WAIT(queue_notempty, queue_lock);
  s = jobs[head];
  head = (head + 1) % QUEUESIZE;
  P_COND_SIGNAL(queue_notfull);
  P_MUTEX_UNLOCK(queue_lock);
  return s;
}

/* worker per thread */
void *
OnvifService__thread(void *copy){
    struct soap *tsoap = (struct soap*)copy;
    int (*serv_func)(struct soap*);
    if (!tsoap)
        return NULL;
    for (;;){
        tsoap->socket = dequeue();
        if (!soap_valid_socket(tsoap->socket))
            break;

		if (soap_begin_serve(tsoap)){ //Parse raw HTTP request
            if (tsoap->error >= SOAP_STOP)
                continue;
			return NULL;
		}

        if(!strcmp(tsoap->path,ONVIF_DEVICE_SERVICE_PATH)){
            serv_func = OnvifDeviceService__serve;
        } else if(!strcmp(tsoap->path,ONVIF_MEDIA_SERVICE_PATH)){
            serv_func = OnvifMediaService__serve;
        } else {
            C_ERROR("404 service not found.");
            soap_send_empty_response (tsoap, 404);
            return NULL;
        }

		if ((serv_func(tsoap)) && !ONVIF_QUIT_FLAG && tsoap->error && tsoap->error < SOAP_STOP)
			soap_send_fault(tsoap);

        soap_destroy(tsoap);
        soap_end(tsoap);
    }
    soap_destroy(tsoap);
    soap_end(tsoap);
    soap_free(tsoap);

    P_THREAD_DETACH(P_THREAD_ID);
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
    SOAP_SOCKET m;
    P_THREAD_TYPE tids[POOLSIZE];
    onvif_soap = ServiceCommon__soap_new();

    onvif_soap->send_timeout = onvif_soap->recv_timeout = 5;
    onvif_soap->transfer_timeout = 10;

    P_MUTEX_SETUP(queue_lock);
    P_COND_SETUP(queue_notempty);
    P_COND_SETUP(queue_notfull);
    /* start workers */
    for (int i = 0; i < POOLSIZE; i++)
        P_THREAD_CREATE(tids[i], (void*(*)(void*))OnvifService__thread, (void*)soap_copy(onvif_soap));

    m = soap_bind(onvif_soap, NULL, port, 100);
    if (soap_valid_socket(m)){
        SOAP_SOCKET s;
        while (soap_valid_socket(s = soap_accept(onvif_soap))){
            enqueue(s);
        }
    }
    MASTER_TID = P_THREAD_ID;
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
        int i;
        /* stop workers */
        for (i = 0; i < POOLSIZE; i++)
            enqueue(SOAP_INVALID_SOCKET);
        for (i = 0; i < POOLSIZE; i++)
            P_THREAD_JOIN(tids[i]);
        
        //All thread are done. Closing socket
        shutdown(onvif_soap->master, SHUT_RDWR);
        P_THREAD_JOIN(MASTER_TID);
        MASTER_TID = 0;
        //TODO Wait for master thread cleanup
    }

    P_MUTEX_CLEANUP(queue_lock);
    P_COND_CLEANUP(queue_notempty);
    P_COND_CLEANUP(queue_notfull);
}