#pragma once
//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>
//=============================================================================
// Other includes
//=============================================================================
#include <string.h>
#include "zmq.hpp"
#include "ladybug_stream.h"
#include "ladybug_helper.h"
#include "helper.h"
#include "configuration_helper.h"
#include "protobuf_helper.h"
#include <boost/thread.hpp>
#include "myLadybug.h"
#include "error.h"

/*Threads*/
void ladybugThread(zmq::context_t* p_zmqcontext, std::string imageReciever);
void ladybugSimulator(zmq::context_t* p_zmqcontext );
void compressionThread(zmq::context_t* p_zmqcontext, int i);
void sendingThread(zmq::context_t* p_zmqcontext);
void ladybugFileStreamThread(zmq::context_t* p_zmqcontext, char* filename);
int thread_ladybug_full(zmq::context_t* zmq_context);
int thread_ladybug();
int singleThread();

