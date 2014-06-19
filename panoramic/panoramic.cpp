// panoramic.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "timing.h"
#include "client.h"
#include "grabSend.h"

int main(int argc, char* argv[])
{
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    initConfig(argc, argv);
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    //Sleep(5000);
    zmq::context_t* zmq_context = new zmq::context_t(2);
    zmq::socket_t* socket_watchdog = NULL;
    socket_watchdog = new zmq::socket_t(*zmq_context, ZMQ_PULL);
    int val = 1;
	socket_watchdog->setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket_watchdog->setsockopt(ZMQ_SNDHWM, &val, sizeof(val));
    socket_watchdog->bind("inproc://watchdog");
    thread_panoramic(zmq_context);


	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
}

