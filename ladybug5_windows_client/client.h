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
#include <boost/thread.hpp>
//#include <boost\>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/assign.hpp>
#include <assert.h>
#include "protobuf/imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "zmq.hpp"
//#include "zmsg.hpp"
#include "time.h"
#include "configuration.h"
#include "zhelpers.h"
#include <locale.h>

typedef boost::bimap< LadybugDataFormat, std::string > ldf_type;
typedef boost::bimap< LadybugColorProcessingMethod, std::string > lcpm_type;

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

/*Threads*/
void ladybugThread(zmq::context_t* p_zmqcontext, std::string imageReciever);
void ladybugSimulator(zmq::context_t* p_zmqcontext );
void compresseionThread(zmq::context_t* p_zmqcontext, int i);
void sendingThread(zmq::context_t* p_zmqcontext);
int singleThread();

/*Config*/
void printTree (boost::property_tree::ptree &pt, int level);
void createDefaultIni(boost::property_tree::ptree *pt);
void loadConfigsFromPtree(boost::property_tree::ptree *pt);
extern const ldf_type ladybugDataFormatMap;
extern const lcpm_type ladybugColorProcessingMap;

/*Ladybug*/
LadybugError initCamera(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
LadybugError configureLadybugForPanoramic(LadybugContext context);

/*Protobuff*/
/* Serialize the ladybug5_network::pbMessage object and send it over the socket */
bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag = 0);
/* Recieve and deserialize the request to a ladybug5_network::pbMessage object*/
bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message);

/*Helper*/
unsigned int initBuffers(unsigned char** arpBuffers, unsigned int number, unsigned int width, unsigned int height, unsigned int dimensions = 4);
void initBuffersWitPicture(unsigned char** arpBuffers, long unsigned int* size);
void compressImageToMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg, int i, TJPF color = TJPF_BGRA);
void addImageToMessage(ladybug5_network::pbMessage *message,  unsigned char* uncompressedBGRImageBuffer, TJPF color, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height);
void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height );