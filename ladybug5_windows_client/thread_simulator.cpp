#include "timing.h"
#include "client.h"

void ladybugSimulator(zmq::context_t* p_zmqcontext ){
	std::string connection = "inproc://uncompressed";
	std::string status = "Simulator Thread: init";
	printf("%s connecting to: %s\n", status.c_str(), connection.c_str());
	////connection = "inproc://workers_";

    zmq::socket_t socket(*p_zmqcontext, ZMQ_PUSH);
	socket.bind(connection.c_str());

	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}
	
	double t_now = clock();	
	unsigned int uiRawCols = 2448;
	unsigned int uiRawRows = 2048;
	unsigned long size[LADYBUG_NUM_CAMERAS];

	initBuffersWitPicture(arpBuffers, size);
	ladybug5_network::LadybugTimeStamp msg_timestamp;
	ladybug5_network::pbMessage message;
    
	while(true)
	{
			status = "simulator loop sending";
			double loopstart = t_now = clock();	
			double t_now = loopstart;
            int flag = 1;
			
			msg_timestamp.set_ulcyclecount(4);
			msg_timestamp.set_ulcycleoffset(2);
			msg_timestamp.set_ulcycleseconds(5);
			msg_timestamp.set_ulmicroseconds(3);
			msg_timestamp.set_ulseconds(7);

			for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
			{
				message.set_name("windows");
				message.set_camera("ladybug5");
				ladybug5_network::pbImage* image_msg = 0;
				image_msg = message.add_images();
				//image_msg->set_image(arpBuffers[uiCamera],  size[uiCamera]);
				image_msg->set_size( size[uiCamera]);
				image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
				image_msg->set_name(enumToString(image_msg->type()));
				image_msg->set_hight(uiRawCols);
				image_msg->set_width(uiRawRows);
				image_msg->set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));

				assert(image_msg->size()!=0);
                
                zmq::message_t image(size[uiCamera]);
                memcpy(image.data(), arpBuffers[uiCamera], size[uiCamera]);
                if( !cfg_full_img_msg || uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                    socket.send(image, ZMQ_SNDMORE);
                    pb_send(&socket, &message, 0);
                    message.Clear();
                }else{
                    socket.send(image, ZMQ_SNDMORE);
                }
			}
            _TIME
			Sleep(100);
            status = "simulator pause";
			_TIME
		}
}