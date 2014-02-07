#include "timing.h"
#include "client.h"

void sendingThread(zmq::context_t* p_zmqcontext){
    std::string status = "Sendin Thread: init";
    double t_now = clock();	
				
    printf("%s connecting to %s\n", status.c_str(), cfg_ros_master.c_str());
	
	std::string connection_in = "inproc://pb_message";
	zmq::socket_t socket_in(*p_zmqcontext, ZMQ_PULL);
	socket_in.bind(connection_in.c_str());

	zmq::socket_t socket_out(*p_zmqcontext, ZMQ_PUB);
	socket_out.connect(cfg_ros_master.c_str());
    _TIME

	while(true){
        status = "SendingThread: Recived message";
		zmq::message_t in1;
        socket_in.recv(&in1);
        _TIME
        status = "SendingThread: Send message";
        socket_out.send(in1);  
        _TIME
	}
}