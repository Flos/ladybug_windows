#include "thread_functions.h"
#include "timing.h"

void compresseionThread(zmq::context_t* p_zmqcontext, int i)
{	
    std::string status = "CompressionThread";
	std::string connection_in = "inproc://uncompressed";
	std::string connection_out = "inproc://pb_message";
    double t_now = clock();
	printf("Compression Thread%i: in: %s out: %s\n", i, connection_in.c_str(), connection_out.c_str());

    zmq::socket_t socket_in(*p_zmqcontext, ZMQ_PULL);
	socket_in.connect(connection_in.c_str());

    zmq::socket_t socket_out(*p_zmqcontext, ZMQ_PUB);
    socket_out.connect(cfg_ros_master.c_str());

	zmq::message_t zmq_ready;
	ladybug5_network::pbMessage pb_msg;

	while(true)
	{
		status = "compresseionThread recieving image";
        zmq::message_t arpBuffer[LADYBUG_NUM_CAMERAS];
        int numImages = 0;

        int more;
        size_t more_size = sizeof (more);
        
        pb_recv(&socket_in, &pb_msg);
        do{
            socket_in.recv(&arpBuffer[numImages]);
            socket_in.getsockopt(ZMQ_RCVMORE, &more, &more_size);
             ++numImages;
            printf("compression recieved image: %i more: %i\n", numImages, more, more_size);
        }
        while(more);
         _TIME

	    status = "compresseionThread compress jpg";
         zmq::message_t* buffer[LADYBUG_NUM_CAMERAS];
        for(int i=0 ; i < numImages; ++i){
            buffer[i] = &compressImageToZmqMsg(&pb_msg, &arpBuffer[i], i);
        }
		
        _TIME
        status = "compresseionThread serialise and send";

        pb_send(&socket_out, &pb_msg, ZMQ_SNDMORE);

        for(int i=0; i < numImages; ++i){
            socket_out.send(buffer[i], i == LADYBUG_NUM_CAMERAS-1 ? 0 : ZMQ_SNDMORE);
        }

		pb_msg.Clear();
        _TIME
	}
}

