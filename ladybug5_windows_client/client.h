#pragma once
//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>

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
#include <boost/thread.hpp>
#include <assert.h>
#include "protobuf/imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "zmq.hpp"
#include "time.h"
#include "configuration_helper.h"
#include "zhelpers.h"
#include <locale.h>

//
// Includes
//
#include "functions.h"
#include "configuration_helper.h"
#include "ladybug_stream.h"

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


/*Ladybug*/
LadybugError initCamera(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
LadybugError configureLadybugForPanoramic(LadybugContext context);

/*Protobuff*/
/* Serialize the ladybug5_network::pbMessage object and send it over the socket */
bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag = 0);
/* Recieve and deserialize the request to a ladybug5_network::pbMessage object*/
bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message);

