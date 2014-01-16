//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <iostream>

//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>

//=============================================================================
// Other Includes
//=============================================================================
#include "socket.h"
#include "timing.h"
#include "turbojpeg.h"

//=============================================================================
// Macro Definitions
//=============================================================================
#define _HANDLE_ERROR \
	if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! Ladybug library reported %s\n", \
	::ladybugErrorToString( error ) ); \
	printf( "\n\n\nRestarting");\
	Sleep(500);\
	goto _EXIT; \
	}	

LadybugError initCamera(LadybugContext context);
LadybugError configureLadybugForPanoramic(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
ladybug5_network::ImageMessage createMessage();
void addImageToMessage(ladybug5_network::ImageMessage &message,  unsigned char* uncompressedBGRImageBuffer, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height);
void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height );