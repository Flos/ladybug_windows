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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <assert.h>
#include "protobuf/imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "zmq.hpp"
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

static bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag = 0){
	// serialize the request to a string
	std::string pb_serialized;
	pb_message->SerializeToString(&pb_serialized);
    
	// create and send the zmq message
	zmq::message_t request (pb_serialized.size());
	memcpy ((void *) request.data (), pb_serialized.c_str(), pb_serialized.size());
	return socket->send(request, flag);
}

static bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message){
	// serialize the request to a string
	zmq::message_t zmq_msg;
	bool result = socket->recv(&zmq_msg);
	pb_message->ParseFromArray(zmq_msg.data(), zmq_msg.size());
	return result;
}

void ladybugThread(zmq::context_t* p_zmqcontext, std::string imageReciever);
void ladybugSimulator(zmq::context_t* p_zmqcontext );
void compresseionThread(zmq::context_t* p_zmqcontext, int i);
void sendingThread(zmq::context_t* p_zmqcontext);

LadybugError initCamera(LadybugContext context);
void initBuffers(unsigned char** arpBuffers, unsigned int number, unsigned int width, unsigned int height, unsigned int dimensions = 4);
void initBuffersWitPicture(unsigned char** arpBuffers, long unsigned int* size);
LadybugError configureLadybugForPanoramic(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
ladybug5_network::pbMessage createMessage(std::string name, std::string camera);
int singleThread();
void compressImageInMsg(ladybug5_network::pbMessage *message);
void compressImageToMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg);
void addImageToMessage(ladybug5_network::pbMessage *message,  unsigned char* uncompressedBGRImageBuffer, TJPF color, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height);
void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height );