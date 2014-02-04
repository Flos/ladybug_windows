#pragma once
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
#include "turbojpeg.h"
#include <boost\thread.hpp>
#include <boost\circular_buffer.hpp>
#include <assert.h>
#include "protobuf/imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "zmq.h"
//#include "zmsg.hpp"
#include "time.h"
#include "configuration.h"
#include "zhelpers.h"

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


struct myThread{
	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	boost::thread* thread;
};

void ladybugThread(void* p_zmqcontext, std::string imageReciever);
void ladybugSimulator(void* p_zmqcontext );
void compresseionThread(void* p_zmqcontext, int i);
void sendingThread(void* p_zmqcontext, std::string rosmaster);

LadybugError initCamera(LadybugContext context);
void initBuffers(unsigned char** arpBuffers, unsigned int number, unsigned int width, unsigned int height, unsigned int dimensions = 4);
void initBuffersWitPicture(unsigned char** arpBuffers, long unsigned int* size);
LadybugError configureLadybugForPanoramic(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
ladybug5_network::pbMessage createMessage(std::string name, std::string camera);
void compressImageInMsg(ladybug5_network::pbMessage *message);
void addImageToMessage(ladybug5_network::pbMessage *message,  unsigned char* uncompressedBGRImageBuffer, TJPF color, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height);
void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height );