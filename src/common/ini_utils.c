#include "ini_utils.h"
#include <pwd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "clogger.h"
#include <stdio.h>

int 
IniUtils__get_config_path(char * file_path, char * ret_val){
    const char * configdir;
    if((configdir = getenv("XDG_CONFIG_HOME")) != NULL){
        strcpy(ret_val,configdir);
        strcat(ret_val,"/onvifserver/");
        strcat(ret_val,file_path);

        C_TRACE("Using XDG_CONFIG_HOME config directory : %s",configdir);
        return EXIT_SUCCESS;
    }

    const char *homedir;
    char *buf = NULL;
    if ((homedir = getenv("HOME")) == NULL) {
        struct passwd pwd;
        struct passwd *result;
        size_t bufsize;
        int s;
        bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == (unsigned int) -1)
            bufsize = 0x4000; // = all zeroes with the 14th bit set (1 << 14)
        buf = malloc(bufsize);
        if (buf == NULL) {
            C_ERROR("IniUtils__get_config_path malloc failed.");
            exit(EXIT_FAILURE);
        }
        s = getpwuid_r(getuid(), &pwd, buf, bufsize, &result);
        if (result == NULL) {
            if (s == 0)
                C_WARN("Not found\n");
            else {
                errno = s;
                C_ERROR("IniUtils__get_config_path failed getpwnam_r");
            }
            exit(EXIT_FAILURE);
        }
        homedir = result->pw_dir;
    }

    C_TRACE("Generating default config path from HOME directory : %s\n",homedir);

    strcpy(ret_val,homedir);
    strcat(ret_val,"/.config/onvifserver/");
    strcat(ret_val,file_path);

    if(buf) free(buf);

    return EXIT_SUCCESS;
}

void 
IniUtils__extract_data(struct soap * soap, char * path, key_pair_callback callback, void * user_data){
    FILE * fptr = fopen(path, "r");
    int buffer_size = INI_BUFFER_LENGTH;
    char buffer[buffer_size];
    char ch;
    int comment_ignore = 0; //Flag used to support comment markers
    int max_buffer_reached = 0; //Flag used to establish a buffer limit to avoid large memory usage
    int index = 0;              //Current buffer length
    int parsing_section_name = -1; //Start index of the category title
    int current_line = 1;

    char * current_category = NULL;
    while((ch = (char) fgetc(fptr)) != EOF){
        if(ch == '#'){
            comment_ignore = 1;
            continue;
        } else if (ch == '\n'){
            if(index){
                if(current_category){
                    buffer[index] = '\0';
                    char *c = strchr (buffer, '=');
                    int pos = c ? c - buffer : -1;
                    if(pos > 1){
                        if(callback)
                            callback(soap, current_category, buffer,pos, &buffer[pos+1],index-pos,user_data);
                    } else {
                        C_ERROR("Invalid keypair syntax on line #%d [%s]",current_line,buffer);
                    }
                } else {
                    C_ERROR("KeyPair defined without category on line #%d",current_line);
                }
            }
            current_line++;
            comment_ignore = 0;
            max_buffer_reached = 0;
            parsing_section_name = -1;
            index = 0;
            continue;
        } else if (ch == '[' && index == 0){
            parsing_section_name = index;
        } else if (parsing_section_name > -1 && ch == ']'){
            //allocate or resize current category buffer if too small
            if(!current_category) {
                current_category = malloc(index-1-parsing_section_name);
            } else if ( (int)strlen(current_category) < index-1-parsing_section_name){
                current_category = realloc(current_category,index-1-parsing_section_name);
            }
            //Copy category name buffer so that it can be used in later keypair dispatch
            strncpy(current_category, &buffer[parsing_section_name+1], index-1-parsing_section_name);
            current_category[index-1-parsing_section_name] = '\0';
            parsing_section_name = -1; //Reset category name flag
            index = 0;
            continue;
        }

        if(comment_ignore || max_buffer_reached){
            continue;
        }

        //process ch
        if(index >= buffer_size){
            // buffer_size = buffer_size*2;
            // buffer = realloc(buffer,buffer_size);
            max_buffer_reached = 1;
            C_ERROR("Reached max buffer. Ignoring the rest of the line.");
            continue;
        }

        buffer[index] = ch;
        index++;
    }

    if(current_category){
        free(current_category);
        current_category = NULL;
    }
}

void 
IniUtils__parse_file(struct soap * soap, char * path, key_pair_callback callback, void * user_data){
    char file_path[PATH_MAX];
    if(IniUtils__get_config_path(path, file_path)){
        C_ERROR("Failed to determine config files location [%s]", path);
        return;
    }

    C_TRACE("Reading INI file : %s",file_path);
    IniUtils__extract_data(soap, file_path, callback,user_data);
}