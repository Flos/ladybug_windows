#pragma once
#include <ladybug.h>
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