#include "timing.h"
#include "client.h"

int singleThread()
{
_RESTART:
	
	double t_now = clock();	
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;

	std::string status = "Creating zmq context and connect";
	zmq::context_t zmq_context(1);

	zmq::socket_t socket (zmq_context, ZMQ_PUB);
	int val = 6; //buffer size
	socket.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled

    printf("Sending images to %s\n", cfg_ros_master.c_str());
	socket.connect (cfg_ros_master.c_str());
	_TIME
	
	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}
	status = "Create Ladybug context";
	LadybugError error;

	int retry = 10;

	// create ladybug context
	LadybugContext context = NULL;
	error = ladybugCreateContext( &context );
	if ( error != LADYBUG_OK )
	{
		printf( "Failed creating ladybug context. Exit.\n" );
		return (1);
	}
	_TIME

	status = "Initialize the camera";

	// Initialize  the camera
	error = initCamera(context);
	_HANDLE_ERROR
	_TIME	

	if(cfg_panoramic){
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
    if (cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
		cfg_ladybug_colorProcessing == LADYBUG_MONO)
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

	while(true)
	{
		try{
			double loopstart = t_now = clock();	
			double t_now = loopstart;	
			ladybug5_network::pbMessage message;
            message.set_name("windows");
			message.set_camera("ladybug5");

			printf("\nGrab Image loop...\n");
			// Grab an image from the camera
			std::string status = "grab image";
			LadybugImage image;
			error = ladybugGrabImage(context, &image); 
			_HANDLE_ERROR
			_TIME

			ladybug5_network::LadybugTimeStamp msg_timestamp;
			msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
			msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
			msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
			msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
			msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);

			status = "Convert images to 6 RGB buffers";
			// Convert the image to 6 RGB buffers
			error = ladybugConvertImage(context, &image, arpBuffers);
			_HANDLE_ERROR
			_TIME

			if(cfg_panoramic){
				status = "Send RGB buffers to graphics card";
				// Send the RGB buffers to the graphics card
				error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
				_HANDLE_ERROR
				_TIME

				status = "create panorame in graphics card";
				// Stitch the images (inside the graphics card) and retrieve the output to the user's memory
				LadybugProcessedImage processedImage;
				error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
				_HANDLE_ERROR
				_TIME
			
				status = "Add image to message";
				addImageToMessage(&message, processedImage.pData, TJPF_BGR, &msg_timestamp, ladybug5_network::LADYBUG_PANORAMIC, processedImage.uiCols, processedImage.uiRows );
				_TIME
			}
			else{
				status = "Adding images without processing";
				for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
				{
					addImageToMessage(&message, arpBuffers[uiCamera], TJPF_BGRA, &msg_timestamp, (ladybug5_network::ImageType) ( 1 << uiCamera), uiRawCols, uiRawRows );
				}
				_TIME
			}
			printf("Sending...\n");
			status = "send img over network";
            pb_send(&socket, &message);
			_TIME

			status = "Sum loop";
			t_now = loopstart;
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

	google::protobuf::ShutdownProtobufLibrary();
	socket.close();
	
	for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ ){
		if ( arpBuffers[ uiCamera ] != NULL )
		{
			delete  [] arpBuffers[ uiCamera ];
		}
	}
	goto _RESTART;

	return 1;
}