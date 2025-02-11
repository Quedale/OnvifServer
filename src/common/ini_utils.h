#ifndef ONVIF_INIT_UTIL_H_ 
#define ONVIF_INIT_UTIL_H_

#include "stdsoap2.h"

#ifndef INI_BUFFER_LENGTH
#   define INI_BUFFER_LENGTH 64
#endif

// typedef struct {
//     char * key;
//     char * value;
// } IniKeyPair;

// typedef struct {
//     char * name;
//     int count;
//     IniKeyPair ** entries;
// } IniSection;

// typedef struct {
//     int count;
//     IniSection ** sections;
// } IniFile;


typedef void (key_pair_callback(struct soap * soap, char * category, char * key, int key_length, char * val, int val_length, void * user_data));

void IniUtils__parse_file(struct soap * soap, char * path, key_pair_callback, void * user_data);

#endif