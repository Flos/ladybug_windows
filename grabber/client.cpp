#include "timing.h"
#include "client.h"
#include "grabSend.h"

void main( int argc, char* argv[] ){
	/* Prevent windows to go to sleep */
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); 
    GOOGLE_PROTOBUF_VERIFY_VERSION;

	/*Load config file*/
    initConfig(argc, argv); 

    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    zmq::context_t* zmq_context = new zmq::context_t(2);
    zmq::socket_t* socket_watchdog = NULL;
    socket_watchdog = new zmq::socket_t(*zmq_context, ZMQ_PULL);
    int val = 1;
	socket_watchdog->setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket_watchdog->setsockopt(ZMQ_SNDHWM, &val, sizeof(val));
    socket_watchdog->bind("inproc://watchdog");
    
    //Test
    GrabSend* grabSend = NULL;
    boost::thread* ladybug_grabber = NULL;

    zmq::message_t msg;
    while(true){
        try{
            if(!socket_watchdog->recv(&msg, ZMQ_NOBLOCK) || grabSend == NULL || grabSend->stop){
                if( grabSend != NULL ){
                    delete grabSend;
                    Sleep(2000);
                }
                if( ladybug_grabber != NULL){
                    ladybug_grabber->interrupt();
                    delete ladybug_grabber;
                    Sleep(2000);
                }
                grabSend = new GrabSend(zmq_context);
                ladybug_grabber = new boost::thread(&GrabSend::loop, grabSend);
            }
        }catch(std::exception e){
            std::printf("Exception, restarting!\n");
        }
        Sleep(5000);
    }

	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
}


