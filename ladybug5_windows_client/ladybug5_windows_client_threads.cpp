#pragma once
#include "timing.h"
#include "myHelper.h"
#include "ladybug5_windows_client.h"

void ladybugThread(void* p_zmqcontext, std::string imageReciever)
{
    printf("Start ladybug5_windows_client.exe\nPano w: %i h: %i\n\n", PANORAMIC_IMAGE_WIDTH, 
		PANORAMIC_IMAGE_HEIGHT);
	std::string connection = "inproc://" + imageReciever;

_RESTART:
	//zmq::context_t *context = p_zmqcontext;

	void *socket = zmq_socket (p_zmqcontext, ZMQ_PUSH);
	//TODO setsockopt
    zmq_connect (socket, "inproc://uncompressed");
	
	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}
	
	double t_now = clock();	
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;

	LadybugError error;

	int retry = 10;

	// create ladybug context
	std::string status = "Create ladybug context";
	LadybugContext context = NULL;
	error = ladybugCreateContext( &context );
	if ( error != LADYBUG_OK )
	{
		printf( "Failed creating ladybug context. Exit.\n" );
	}
	_HANDLE_ERROR
	_TIME

	status = "Initialize the camera";

	// Initialize  the camera
	error = initCamera(context);
	_HANDLE_ERROR
	_TIME	

	if(PANORAMIC){
		status = "configure for panoramic stitching";
		error = configureLadybugForPanoramic(context);
		_HANDLE_ERROR
		_TIME
	}else{
		printf("Skipped configuration for panoramic pictures\n");
		 
	}

	status = "start ladybug";
	error = startLadybug(context);
	_HANDLE_ERROR
	_TIME
	
	// Grab an image to inspect the image size
	status = "Grab an image to inspect the image size";
	error = LADYBUG_FAILED;
	LadybugImage image;
	while ( error != LADYBUG_OK && retry-- > 0)
	{
		error = ladybugGrabImage( context, &image ); 
	}
	_HANDLE_ERROR
	_TIME


	status = "inspect image size";
	// Set the size of the image to be processed
	if (COLOR_PROCESSING_METHOD == LADYBUG_DOWNSAMPLE4 || 
		COLOR_PROCESSING_METHOD == LADYBUG_MONO)
	{
		uiRawCols = image.uiCols / 2;
		uiRawRows = image.uiRows / 2;
	}
	else
	{
		uiRawCols = image.uiCols;
		uiRawRows = image.uiRows;
	}
	_TIME

	// Initialize alpha mask size - this can take a long time if the
	// masks are not present in the current directory.
	status = "Initializing alpha masks (this may take some time)...";
	error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
	_HANDLE_ERROR
	_TIME

	initBuffers(arpBuffers, LADYBUG_NUM_CAMERAS, uiRawCols, uiRawRows, 4);
	zmq_msg_t msg;
	while(true)
	{
		try{
			status = "Grabbing loop...";
			double loopstart = t_now = clock();	
			double t_now = loopstart;	
			LadybugImage image;
			error = ladybugGrabImage(context, &image); 
			_HANDLE_ERROR
	
			ladybug5_network::LadybugTimeStamp msg_timestamp;
			msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
			msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
			msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
			msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
			msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);

			error = ladybugConvertImage(context, &image, arpBuffers);
			_HANDLE_ERROR
			

			for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
			{
				zmq_msg_init(&msg);
				ladybug5_network::pbMessage message = createMessage("windows","ladybug5");
				// send message to queue insted
				unsigned int image_size = uiRawCols*uiRawRows*4;
				ladybug5_network::pbImage* image_msg = 0;
				image_msg = message.add_images();
				image_msg->set_image(arpBuffers[uiCamera], image_size);
				image_msg->set_size(image_size);
				image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
				image_msg->set_name(enumToString(image_msg->type()));
				image_msg->set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));
				image_msg->set_hight(uiRawCols);
				image_msg->set_width(uiRawRows);

				s_send(socket, (char*) message.SerializeAsString().c_str());
				std::cout << "send message with size: " << message.ByteSize() << std::endl;
			}
			_TIME
		}
		catch(std::exception e){
			printf("Exception, trying to recover\n");
			goto _EXIT; 
		};
	}
	
	printf("Done.\n");
	
_EXIT:
	//
	// clean up
	//
	ladybugStop( context );
	ladybugDestroyContext( &context );
	for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ ){
		if ( arpBuffers[ uiCamera ] != NULL )
		{
			delete  [] arpBuffers[ uiCamera ];
		}
	}
	goto _RESTART;
}

void ladybugSimulator(void* p_zmqcontext ){
	std::string status = "Simulator Thread: init";
	printf("%s\n", status.c_str());
	
	std::string connection = "inproc://uncompressed";
	void *socket = zmq_socket(p_zmqcontext, ZMQ_REQ);
	zmq_connect(socket, connection.c_str());

	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}
	
	double t_now = clock();	
	unsigned int uiRawCols = 2048;
	unsigned int uiRawRows = 2448;
	unsigned long size[LADYBUG_NUM_CAMERAS];

	initBuffersWitPicture(arpBuffers, size);
	ladybug5_network::LadybugTimeStamp msg_timestamp;
	ladybug5_network::pbMessage message = createMessage("windows","ladybug5");
	zmq_msg_t reply;
	while(true)
	{
			status = "Grabbing loop...";
			double loopstart = t_now = clock();	
			double t_now = loopstart;	
	
			
			msg_timestamp.set_ulcyclecount(4);
			msg_timestamp.set_ulcycleoffset(2);
			msg_timestamp.set_ulcycleseconds(5);
			msg_timestamp.set_ulmicroseconds(3);
			msg_timestamp.set_ulseconds(7);

			for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
			{
				// send message to queue insted
				message.Clear();
				message.set_name("windows");
				message.set_camera("ladybug5");
				ladybug5_network::pbImage* image_msg = 0;
				image_msg = message.add_images();
				image_msg->set_image(arpBuffers[uiCamera],  size[uiCamera]);
				image_msg->set_size( size[uiCamera]);
				image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
				image_msg->set_name(enumToString(image_msg->type()));
				image_msg->set_hight(uiRawCols);
				image_msg->set_width(uiRawRows);
				image_msg->set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));

				assert(image_msg->image().size()!=0);

				s_send(socket, (char*) message.SerializeAsString().c_str());
				//s_recv(socket); 
				std::cout << "Simulator Thread: send message with size: " << message.ByteSize() << std::endl;
				Sleep(1000);
			}
			
			_TIME
		}
}


void compresseionThread(void* p_zmqcontext, int i)
{
	printf("Compression Thread%i: init \n", i);

	void *uncompressed = zmq_socket (p_zmqcontext, ZMQ_REP);
    zmq_connect (uncompressed, "inproc://workers");

	void *compressed = zmq_socket (p_zmqcontext, ZMQ_DEALER);
    zmq_connect (compressed, "inproc://pb_message");

	std::string ready = "ready";
	ladybug5_network::pbMessage msg;
	zmq_msg_t msg_uncompressed;
	int more;
	size_t more_size = sizeof (more);
	while(true)
	{
		msg.Clear();
		int rc = zmq_msg_init(&msg_uncompressed);
		assert (rc == 0);
		/* Block until a message is available to be received from socket */
		rc = zmq_recvmsg (uncompressed, &msg_uncompressed, 0);
		assert (rc == 0);
		zmq_getsockopt(&msg_uncompressed, ZMQ_MORE, &more, &more_size);
		assert (rc == 0);

		msg.ParseFromArray(&msg_uncompressed, more_size);

		assert(msg.images(0).image().length()!=0);
		assert(msg.images(0).image().size()!=0);
	
		compressImageInMsg(&msg);

		std::cout << "Compression Thread"<< i <<": sending image: " << msg.ByteSize() << std::endl;
		s_send(compressed, (char*)msg.SerializePartialAsString().c_str());
		s_send(uncompressed, (char*)ready.c_str());
		zmq_msg_close(&msg_uncompressed);
	}
}

void sendingThread(void* p_zmqcontext, std::string rosmaster){
	printf("Sending Thread: init connecting to %s\n", rosmaster.c_str());

	void *sink = zmq_socket (p_zmqcontext, ZMQ_DEALER);
    zmq_bind (sink, "inproc://pb_message");

	void* publisher= zmq_socket(p_zmqcontext, ZMQ_PUB);
	zmq_connect(publisher, rosmaster.c_str());

	zmq_msg_t part;
	int more;
	size_t more_size = sizeof (more);

	while(true){
		int rc = zmq_msg_init (&part);
		assert (rc == 0);
		/* Block until a message is available to be received from socket */
		rc = zmq_recvmsg (sink, &part, 0);
		assert (rc != -1);
		rc = zmq_getsockopt(&part, ZMQ_MORE, &more, &more_size);
		assert (rc == 0);
		if (more) {
		  fprintf (stderr, "more\n");
		}
		else {
		  fprintf (stderr, "end\n");
		  break;
		}
		zmq_msg_send(&part, publisher, 0);
		printf("Sending Thread: send message\n %i\n",more_size );
		zmq_msg_close(&part); 
	}
}
