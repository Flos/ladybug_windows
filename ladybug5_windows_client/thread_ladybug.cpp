#include "timing.h"
#include "client.h"

void ladybugThread(zmq::context_t* p_zmqcontext, std::string imageReciever)
{
    printf("Start ladybug5_windows_client.exe\nSending images to: %s\n", imageReciever.c_str());

_RESTART:
	zmq::socket_t socket(*p_zmqcontext, ZMQ_PUSH);
    socket.bind(imageReciever.c_str());
	
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
	{
        unsigned int image_size = initBuffers(arpBuffers, LADYBUG_NUM_CAMERAS, uiRawCols, uiRawRows, 4);
		ladybug5_network::LadybugTimeStamp msg_timestamp;
		ladybug5_network::pbMessage message;
		while(true)
		{
			try{
				status = "Grabbing loop... CAM";
				double loopstart = t_now = clock();	
				double t_now = loopstart;	
				LadybugImage image;
				error = ladybugGrabImage(context, &image); 
				_HANDLE_ERROR
	
				msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
				msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
				msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
				msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
				msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);

				error = ladybugConvertImage(context, &image, arpBuffers);
				_HANDLE_ERROR

				for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
				{
					message.set_name("windows");
				    message.set_camera("ladybug5");
					ladybug5_network::pbImage* image_msg = 0;
					image_msg = message.add_images();
					//image_msg->set_image(arpBuffers[uiCamera], image_size);
					image_msg->set_size(image_size);
					image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
					image_msg->set_name(enumToString(image_msg->type()));
					image_msg->set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));
					image_msg->set_hight(uiRawRows);
					image_msg->set_width(uiRawCols);

					zmq::message_t image(image_size);
                    memcpy(image.data(), arpBuffers[uiCamera], image_size);
                    if( !cfg_full_img_msg || uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                        socket.send(image, ZMQ_SNDMORE);
                        pb_send(&socket, &message, 0);
                        message.Clear();
                        printf("Ladybug thread send msg");
                    }else{
                        socket.send(image, ZMQ_SNDMORE);
                    }
				}
                if(cfg_panoramic){
                    printf("Panoramic output not supported while multithreading\n");
                }
				_TIME
			}
			catch(std::exception e){
				printf("Exception, trying to recover\n");
				goto _EXIT; 
			};
		}
	
		printf("Done.\n");
	}
	
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
