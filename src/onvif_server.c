#include "clogger.h"
#include "portable_thread.h"
#include "common/service_common.h"
#include "devicemgmt/device_service.h"
#include "media/media_service.h"
#include "discovery/discovery_service.h"
#include <argp.h>
#include <signal.h>

const char *argp_program_version = "0.0";
const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "Onvif Linux Server.";

struct soap * discovery_soap = NULL;
struct soap * onvif_soap = NULL;
int QUIT_FLAG = 0;

static struct argp_option options[2] = {
    { "port",   'p',    "PORT", 0,      "Network port to listen. (Default: 80)", 1},
    { 0 } 
};

//Define argument struct
struct arguments {
    int port;
};

//main arguments processing function
static error_t 
parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p': arguments->port = atoi(arg); break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

//Initialize argp context
static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };

SOAP_FMAC5 int SOAP_FMAC6 
serve_request(struct soap *soap){
    int (*serv_func)(struct soap*);

	soap->keep_alive = soap->max_keep_alive + 1;
	do{
		if (soap->keep_alive > 0 && soap->max_keep_alive > 0)
			soap->keep_alive--;
		if (soap_begin_serve(soap)){ //Parse raw HTTP request
            if (soap->error >= SOAP_STOP)
            continue;
			return soap->error;
		}

        if(!strcmp(soap->path,ONVIF_DEVICE_SERVICE_PATH)){
            serv_func = OnvifDeviceService__serve;
        } else if(!strcmp(soap->path,ONVIF_MEDIA_SERVICE_PATH)){
            serv_func = OnvifMediaService__serve;
        } else {
            C_ERROR("404 service not found.");
            soap_send_empty_response (soap, 404);
            return SOAP_OK;
        }

		if ((serv_func(soap)) && soap->error && soap->error < SOAP_STOP)
			return soap_send_fault(soap);
	} while (soap->keep_alive);
	return SOAP_OK;
}

void *
thread_process_request(void* tsoap) {
    struct soap *soap = (struct soap*)tsoap;
    P_THREAD_DETACH(P_THREAD_ID); 

    serve_request(soap);

    soap_destroy(soap); // delete managed class instances 
    soap_end(soap);     // delete managed data and temporaries 
    soap_free(soap);    // finalize and delete the context
    return NULL; 
}

void *
thread_discovery_serve(void* nul) {
    P_THREAD_DETACH(P_THREAD_ID); 

    discovery_soap = ServiceCommon__soap_new1(SOAP_IO_UDP);

    discovery_soap->connect_flags = SO_BROADCAST;
    SOAP_SOCKET m = soap_bind(discovery_soap, NULL, 3702, 100);
    if (!soap_valid_socket(m)) {
        C_ERROR("Failed to bind discovery socket on port 3702");
        goto exit;
    }

    struct ip_mreq mcast; 
    mcast.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(discovery_soap->master, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast, sizeof(mcast))<0) {
        C_ERROR("UDP group membership failed");
        goto exit;
    }

    C_DEBUG("Discovery Listening on port 3702");
    // TODO send Hello;
    while (!QUIT_FLAG) { //TODO Implemented threaded response?
		if (soap_begin_serve(discovery_soap) && !QUIT_FLAG){ //Parse raw HTTP request
            if (discovery_soap->error >= SOAP_STOP){
                C_ERROR("Failed to parse discovery request...");
                continue;
            }
		}

        if(OnvifDiscoveryService__serve(discovery_soap) && !QUIT_FLAG){
            print_soap_fault(discovery_soap);
            C_ERROR("Failed to serve discovery request...");
            continue;
        }
    }
    // TODO send Bye;
exit:
    C_WARN("Discovery service terminated");
    soap_destroy(discovery_soap); // delete managed class instances 
    soap_end(discovery_soap);     // delete managed data and temporaries 
    soap_free(discovery_soap);    // finalize and delete the context
    return NULL; 
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGKILL:
    case SIGQUIT:
        if(QUIT_FLAG){
            C_FATAL("Forcing shutdown!");
            exit(1);
        }
        // CSI <n> G: Cursor horizontal absolute
        printf("\033[1G"); //Hide ^C
        C_WARN("Graceful shutdown...\n");
        QUIT_FLAG = 1;
        if(discovery_soap){
            if(discovery_soap){
                C_WARN("Stopping discovery service %p", (void*)discovery_soap);
                shutdown(discovery_soap->master, SHUT_RDWR);
            }
            if(onvif_soap){
                C_WARN("Stopping onvif service %p", (void*)discovery_soap);
                shutdown(onvif_soap->master, SHUT_RDWR);
            }
        }
        break;
    default:
        C_FATAL("Unexpected process signal\n");
        exit(1);
    }
}

int 
main(int argc, char *argv[]) {
    c_log_set_level(C_TRACE_E);
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGQUIT, sig_handler);

    struct arguments arguments;

    C_INFO("Onvif Device Server");

    /* Default values. */
    arguments.port = 80;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(arguments.port < 1 || arguments.port > 65535){
        C_FATAL("Invalid port number provider. Must be >0 & <=65535.\n\tCurrent value : '%d'",arguments.port);
        return 1;
    }

    onvif_soap = ServiceCommon__soap_new();

    onvif_soap->accept_timeout = 24*60*60;             /* quit after 24h of inactivity (optional) */
    onvif_soap->send_timeout = onvif_soap->recv_timeout = 5; /* max send and receive socket inactivity time (sec) */
    onvif_soap->transfer_timeout = 10;
    SOAP_SOCKET m, s;
    struct soap *tsoap;
    P_THREAD_TYPE tid;

    m = soap_bind(onvif_soap, NULL, arguments.port, 2);          /* small backlog queue of 2 to 10 for iterative servers */
    
    if (!soap_valid_socket(m)) {
        C_FATAL("Failed to bind socket on port %d",arguments.port);
        exit(EXIT_FAILURE);
    }
    
    P_THREAD_CREATE(tid, (void*(*)(void*))thread_discovery_serve, NULL);

    C_DEBUG("Listening on port %d",arguments.port);
    while (!QUIT_FLAG){
        s = soap_accept(onvif_soap); 
        if (soap_valid_socket(s)){
            tsoap = soap_copy(onvif_soap);
            if (!tsoap) 
                soap_force_closesock(onvif_soap);
            else
                while (P_THREAD_CREATE(tid, (void*(*)(void*))thread_process_request, (void*)tsoap))
                    sleep(1); // failed, try again

        } else if (onvif_soap->errnum && !QUIT_FLAG){
            print_soap_fault(onvif_soap); 
            sleep(1);
        } else if (!QUIT_FLAG) {
            C_ERROR("Server timed out"); 
            break; 
        } 
    }

    soap_destroy(onvif_soap); /* delete deserialized objects */
    soap_end(onvif_soap);     /* delete heap and temp data */
    soap_free(onvif_soap);    /* we're done with the context */

    C_INFO("Onvif Service terminated");
    return 0;
}