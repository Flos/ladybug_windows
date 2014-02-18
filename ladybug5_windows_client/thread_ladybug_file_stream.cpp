#include "thread_functions.h"
#include "timing.h"

void ladybugFileStreamThread(zmq::context_t* p_zmqcontext, char* filename ){
    double t_now = clock();	
    //inspect stream context
    LadybugError error;
    LadybugImage image;
    LadybugContext context;
    LadybugStreamContext streamContext;
    LadybugStreamHeadInfo streamHeadInfo;
    unsigned int stream_image_count;
    unsigned int frame_image_count;
    bool seperatedColors;
    unsigned int red_offset,green_offset,blue_offset;

    //
    // Create contexts and prepare stream for reading
    //
    error = ladybugCreateContext( &context);

    error = ladybugCreateStreamContext( &streamContext);

    error = ladybugInitializeStreamForReading( streamContext, filename );

    error = ladybugGetStreamNumOfImages( streamContext, &stream_image_count);

    error = ladybugGetStreamHeader( streamContext, &streamHeadInfo);

    error = ladybugReadImageFromStream( streamContext, &image); 

    unsigned int sleepTime = (1000 - (streamHeadInfo.frameRate * 15)) / streamHeadInfo.frameRate;
    std::string connection;
    seperatedColors = isColorSeperated(&image);
    frame_image_count = getImageCount(&image);

    getColorOffset(&image, red_offset, green_offset, blue_offset);

    zmq::socket_t* socket;
    int i = 24;
    if(seperatedColors){
        socket = new zmq::socket_t(*p_zmqcontext, ZMQ_PUB);
        socket->setsockopt( ZMQ_RCVHWM, &i, sizeof(i));
        socket->setsockopt( ZMQ_SNDHWM, &i, sizeof(i));
        connection = cfg_ros_master.c_str();
        socket->connect(connection.c_str());
    }
    else{
        socket = new zmq::socket_t(*p_zmqcontext, ZMQ_PUSH);
        socket->setsockopt( ZMQ_RCVHWM, &i, sizeof(i));
        socket->setsockopt( ZMQ_SNDHWM, &i, sizeof(i));
        connection = "inproc://uncompressed";
        socket->bind(connection.c_str());
    }

	std::string status = "ladybug filestream Thread: init";
    printf("%s reading stream: %s with %i images, connecting to: %s\n", status.c_str(), filename, stream_image_count, connection.c_str());

	ladybug5_network::LadybugTimeStamp msg_timestamp;
	ladybug5_network::pbMessage message;
    int imageNr = 0;
	while(true)
	{
        ladybugReadImageFromStream( streamContext, &image);

		status = "Process image loop sending";
		double loopstart = t_now = clock();	
		t_now = loopstart;
			
		msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
		msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
		msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
		msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
		msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);
        message.set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));

		for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
		{
            message.set_id(imageNr);
			message.set_name("windows");
			message.set_camera("ladybug5");
			ladybug5_network::pbImage* image_msg = 0;
			image_msg = message.add_images();
			image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
			image_msg->set_name(enumToString(image_msg->type()));
			image_msg->set_height(image.uiRows);
            image_msg->set_width(image.uiCols);
            if(seperatedColors){
                image_msg->set_packages(3);
            }
        }
        status = "Stream: send image " + std::to_string(imageNr);

        pb_send(socket, &message, ZMQ_SNDMORE);

        for( unsigned int nr = 0; nr < LADYBUG_NUM_CAMERAS; nr++ )
        {
            unsigned int index = nr*4;
            //send images 
                     
            unsigned int r_size, g_size, b_size;
            char* r_data = NULL;
            char* g_data = NULL;
            char* b_data = NULL;

            extractImageToMsg(&image, index+blue_offset,    &b_data, b_size);
            extractImageToMsg(&image, index+green_offset,   &g_data, g_size);
            extractImageToMsg(&image, index+red_offset,     &r_data, r_size);

            //RGB expected at reciever
            zmq::message_t R(r_size);
            memcpy(R.data(), r_data, r_size);
            socket->send(R, ZMQ_SNDMORE ); //R = Index + 3

            zmq::message_t G(g_size);
            memcpy(G.data(), g_data, g_size);
            socket->send(G, ZMQ_SNDMORE ); //G = Index + 1 || 2

            zmq::message_t B(b_size);
            memcpy(B.data(), b_data, b_size);

            int flag = ZMQ_SNDMORE;

            if( nr == LADYBUG_NUM_CAMERAS-1 ){
               flag = 0;
            }
            socket->send(B, flag ); //B = index
		}
        message.Clear();
        Sleep(sleepTime);
        imageNr=(imageNr+1)%stream_image_count;
        error = ladybugGoToImage( streamContext, imageNr);
        _TIME
	}
}