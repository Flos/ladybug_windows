#include "thread_functions.h"
#include "timing.h"

void compressionThread(zmq::context_t* p_zmqcontext, int i)
{	
    std::string status = "CompressionThread";
    double t_now = clock();
	printf("Compression Thread%i: in: %s out: %s\n", i, zmq_uncompressed, zmq_compressed);

    zmq::socket_t socket_in(*p_zmqcontext, ZMQ_PULL);
    int val = 2; //buffer size
	socket_in.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket_in.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket_in.connect(zmq_uncompressed);

    zmq::socket_t socket_out(*p_zmqcontext, ZMQ_PUSH);
    socket_out.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket_out.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled
    socket_out.connect(zmq_compressed);

	zmq::message_t zmq_ready;
	ladybug5_network::pbMessage pb_msg;
    const unsigned int max_nr_images = LADYBUG_NUM_CAMERAS + 1; /*6x raw + panoramic*/

	while(true)
	{
		status = "compresseionThread recieving image";
        zmq::message_t arpBuffer[max_nr_images];
        int numImages = 0;

        int more;
        size_t more_size = sizeof (more);
        
        pb_recv(&socket_in, &pb_msg);
        assert(pb_msg.images_size() > 0);
        do{
            socket_in.recv(&arpBuffer[numImages]);
            socket_in.getsockopt(ZMQ_RCVMORE, &more, &more_size);
             ++numImages;
            printf("compression recieved image: %i more: %i\n", numImages, more, more_size);
        }
        while(more);
        //_TIME

	    status = "compresseionThread compress jpg";
        zmq::message_t buffer[max_nr_images];
        for(int i=0 ; i < numImages; ++i){
            if( i < max_nr_images-1){
                buffer[i] = compressImageToZmqMsg(&pb_msg, &arpBuffer[i], i, TJPF_RGBA); // TJPF_BGRA
            }
            else{
                 /* panramic image is BGR not BGRA, last image is the panoramic*/
                buffer[i] = compressImageToZmqMsg(&pb_msg, &arpBuffer[i], i, TJPF_RGB);
            }  
        }
		
        _TIME
        status = "compresseionThread serialise and send";

        pb_send(&socket_out, &pb_msg, ZMQ_SNDMORE);

        for(int i=0; i < numImages; ++i){
            socket_out.send(buffer[i], i == numImages-1 ? 0 : ZMQ_SNDMORE);
        }

		pb_msg.Clear();
        _TIME
	}
}

