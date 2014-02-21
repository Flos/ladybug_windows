#include "thread_functions.h"
#include "timing.h"

int singleThread()
{
_RESTART:
	
	double t_now = clock();	
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;
    bool seperatedColors = false;
    unsigned int red_offset,green_offset,blue_offset;

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
	while ( error != LADYBUG_OK && retry-- > 0)	{
		error = ladybugGrabImage( context, &image ); 
	}
	_HANDLE_ERROR
	_TIME

   
	status = "inspect image size";
	// Set the size of the image to be processed
    if(cfg_postprocessing){
        if (cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
	        cfg_ladybug_colorProcessing == LADYBUG_MONO)
        {
	        uiRawCols = image.uiCols / 2;
	        uiRawRows = image.uiRows / 2;
        }else{
	        uiRawCols = image.uiCols;
	        uiRawRows = image.uiRows;
        }
        // Initialize alpha mask size - this can take a long time if the
	    // masks are not present in the current directory.
	    status = "Initializing alpha masks (this may take some time)...";
	    error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
	    _HANDLE_ERROR

	    initBuffers(arpBuffers, LADYBUG_NUM_CAMERAS, uiRawCols, uiRawRows, 4);

    }else{
        uiRawCols = image.uiFullCols;
        uiRawRows = image.uiFullRows;
        seperatedColors = isColorSeperated(&image);
        getColorOffset(&image, red_offset, green_offset, blue_offset);
    }
	_TIME

    {
	    ladybug5_network::pbMessage message;
        ladybug5_network::LadybugTimeStamp msg_timestamp;
        unsigned int nr = 0;
        double loopstart = t_now = clock();		
        printf("Running...\n");
 
	    while(true)
	    {
		    try{
			    loopstart = t_now = clock();
			    t_now = loopstart;

			    // Grab an image from the camera
			    std::string status = "grab image";
			    //LadybugImage image;
			    error = ladybugGrabImage(context, &image); 
			    _HANDLE_ERROR
			    _TIME

                message.set_name("windows");
			    message.set_camera("ladybug5");
                message.set_id(nr);
		
			    msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
			    msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
			    msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
			    msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
			    msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);
                message.set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));


                if(cfg_postprocessing)
                {
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
                }else{ //No post processing, no panoramic picture
                    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
		            {
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
                    status = "Stream: send image " + std::to_string(nr);
                    pb_send(&socket, &message, ZMQ_SNDMORE);

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
                        socket.send(R, ZMQ_SNDMORE ); //R = Index + 3

                        zmq::message_t G(g_size);
                        memcpy(G.data(), g_data, g_size);
                        socket.send(G, ZMQ_SNDMORE ); //G = Index + 1 || 2

                        zmq::message_t B(b_size);
                        memcpy(B.data(), b_data, b_size);

                        int flag = ZMQ_SNDMORE;

                        if( nr == LADYBUG_NUM_CAMERAS-1 ){
                           flag = 0;
                        }
                        socket.send(B, flag ); //B = index
		            }
                }

                message.Clear();
                msg_timestamp.Clear();
                ++nr;
			    status = "Sum loop";
			    t_now = loopstart;
			    _TIME
		    }
		
		    catch(std::exception e){
			    printf("Exception, trying to recover\n");
			    goto _EXIT; 
		    };
	    }
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