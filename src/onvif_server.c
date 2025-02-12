#include "clogger.h"
#include "portable_thread.h"
#include "discovery/discovery_service.h"
#include "rtsp/onvif_rtsp_service.h"
#include "onvif_service.h"
#include <argp.h>
#include <signal.h>

const char *argp_program_version = "0.0";
const char *argp_program_bug_address = "<your@email.address>";
static char doc[] = "Onvif Linux Server.";
int QUIT_FLAG = 0; //Used to force quit on double sigterm signal
OnvifRtspService * rtsp_service = NULL;

static struct argp_option options[] = {
    { "port",       'p',    "PORT",     0,  "Network port to listen. (Default: 8080)", 1},
    { "rtspport",   'r',    "RTSPPORT", 0,  "RTSP port used. (Default: 8554)",         1},
    { 0 }
};

//Define argument struct
struct arguments {
    int port;
    int rtsp_port;
};

//main arguments processing function
static error_t 
parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p': arguments->port = atoi(arg); break;
    case 'r': arguments->rtsp_port = atoi(arg); break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

//Initialize argp context
static struct argp argp = { options, parse_opt, NULL, doc, 0, 0, 0 };

void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
        if(QUIT_FLAG){
            C_FATAL("Forcing shutdown!");
            exit(1);
        }
        // CSI <n> G: Cursor horizontal absolute
        printf("\033[1G"); //Hide ^C
        C_WARN("Graceful shutdown...\n");
        QUIT_FLAG = 1;
        OnvifDiscoveryService__stop();
        OnvifService__stop();
        
        if(rtsp_service){
            C_WARN("Stopping RTSP service");
            OnvirRtspService__stop(rtsp_service);
            g_object_unref(rtsp_service);
            rtsp_service = NULL;
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
    c_log_set_thread_color(ANSI_COLOR_BLUE, P_THREAD_ID);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGQUIT, sig_handler);

    gst_plugin_init_static();
    
    struct arguments arguments;

    /* Default values. */
    arguments.port = 8080;
    arguments.rtsp_port = 8554;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(arguments.port < 1 || arguments.port > 65535){
        C_FATAL("Invalid port number provider. Must be >0 & <=65535.\n\tCurrent value : '%d'",arguments.port);
        return 1;
    }
    if(arguments.rtsp_port < 1 || arguments.rtsp_port > 65535){
        C_FATAL("Invalid RTSP port number provider. Must be >0 & <=65535.\n\tCurrent value : '%d'",arguments.rtsp_port);
        return 1;
    }
    if(arguments.port == arguments.rtsp_port){
        C_FATAL("HTTP port and RTSP port can't be identical. '%d' == '%d'",arguments.port,arguments.rtsp_port);
        return 1;
    }

    rtsp_service = OnvifRtspService__new(arguments.rtsp_port); //This is a GObject since there will be multiple instances in the future (1 for each profile created)
    OnvirRtspService__start(rtsp_service);
    OnvifDiscoveryService__start();
    OnvifService__start(arguments.port, arguments.rtsp_port); //This call doesn't invoke a thread and hangs

    return 0;
}