// stdafx.h : Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>

#include <locale.h>
#include <time.h>

#include <tchar.h>
#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#include <Shlwapi.h>
#include <string>

//=============================================================================
// Other Includes
//=============================================================================
#include <zmq.hpp>
#include <imageMessage.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "config.h"
#include "MyProcess.h"
#include "zmq_service.h"
#include "watchdog_service.h"