// service.cpp: Hauptprojektdatei.

#include "stdafx.h"

void test_zmq(){
	Watchdog_service service;
	//test zmq message
	//ZMQ
	
	service.handle_zmq();

	Zmq_service zmq;
	//std::string socket_connection = "tcp://127.0.0.1:28883";
	zmq.init("tcp://127.0.0.1:28883", ZMQ_REQ);

	//define message
	ladybug5_network::pb_start_msg pb_msg;
	pb_msg.set_name("pano");	
	
	zmq.send(pb_msg);
	
	service.handle_zmq();

	ladybug5_network::pb_reply pb_reply;
	zmq.receive(pb_reply);

	printf("Received: %s\n", pb_reply.SerializeAsString());
}


int main(array<System::String ^> ^args)
{
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

    System::Console::WriteLine(L"Service Started");

	Watchdog_service service;
	service.loop();

	//std::cin.ignore();

    return 0;
}