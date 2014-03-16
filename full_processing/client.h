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
// Other Includes
//=============================================================================
#include <boost/thread.hpp>
#include <assert.h>
#include "imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "zmq.hpp"
#include "time.h"
#include "zhelpers.h"
#include <locale.h>
#include <WinBase.h>

//
// Includes
//
#include "helper.h"
#include "configuration_helper.h"
#include "thread_functions.h"

