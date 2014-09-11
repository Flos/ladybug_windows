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
	
	zmq.send(&pb_msg);
	
	service.handle_zmq();

	ladybug5_network::pb_reply pb_reply;
	zmq.receive(&pb_reply);

	printf("Received: %s\n", pb_reply.SerializeAsString());
}

void create_config(){
	Config config;
	//config.load("watchdog.ini");
	config.config_file="watchdog.ini";
	config.put("watchdog.socket","tcp://*:28883");
	config.put("watchdog.autostart","jpg_raw");	

	config.put_path("jpg_raw","..\\bin\\grabber_x64_Release.exe");
	config.put_path("panoramic","..\\bin\\panoramic_x64_Release.exe");
	config.put_path("calibration","..\\bin\\export_distCoeffs_x64_Release.exe");
	config.put_path("full_processing","..\\bin\\full_processing_x64_Release.exe");

	config.put_path("debug_jpg_raw","..\\bin\\grabber_x64_Debug.exe");
	config.put_path("debug_panoramic","..\\bin\\panoramic_x64_Debug.exe");
	config.put_path("debug_calibration","..\\bin\\export_distCoeffs_x64_Debug.exe");
	config.put_path("debug_full_processing","..\\bin\\full_processing_x64_Debug.exe");

	config.put_path("shutdown","C:\\Windows\\System32\\shutdown.exe");
	config.put_args("shutdown","/s");
	config.put_path("restart","C:\\Windows\\System32\\shutdown.exe");
	config.put_args("restart","/r");
	config.save();
}

int main(array<System::String ^> ^args)
{
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

    System::Console::WriteLine(L"Service Started");

	//create_config();
	Watchdog_service service;
	service.loop();

	//std::cin.ignore();

    return 0;
}