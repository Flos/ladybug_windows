#ifndef TIMING_H_
#define TIMING_H_

#include <iostream>
#include <time.h>
#include <string>
#include <ladybug.h>
#include "imageMessage.pb.h"

#ifndef _TIME
#define _TIME \
	t_now = time_diff(status, t_now, __FUNCTION__);\

#endif

double time_diff(std::string status, double start, std::string name);
std::string enumToString(ladybug5_network::ImageType type);
#endif

//=============================================================================
// Macro Definitions
//=============================================================================
#define _HANDLE_ERROR \
	if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! While %s. Ladybug library reported %s\n", status.c_str(), \
	::ladybugErrorToString( error ) ); \
	printf( "\n\n\nRestarting...\n\n\n");\
	Sleep(500);\
	goto _EXIT; \
	}

#define _HANDLE_ERROR_LADY \
	if( lady->error != LADYBUG_OK ) \
	{ \
	printf( "Error! While %s. Ladybug library reported %s\n", status.c_str(), \
	::ladybugErrorToString( lady->error ) ); \
	printf( "\n\n\nRestarting...\n\n\n");\
	Sleep(500);\
	goto _EXIT; \
	}