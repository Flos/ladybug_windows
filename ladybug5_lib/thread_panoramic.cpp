#include "thread_functions.h"
#include "timing.h"

int thread_panoramic(zmq::context_t* zmq_context)
{
_RESTART:
    boost::thread_group threads;
    zmq::socket_t* socket = NULL;
    zmq::socket_t* socket_watchdog = NULL;
	double t_now = clock();	
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;
    LadybugError error;
    LadybugImage image;
    LadybugContext context;
    std::string status;
	

	bool filestream = cfg_fileStream.size() > 0;
    bool done = false;
 
    bool seperatedColors = false;
    unsigned int red_offset,green_offset,blue_offset;


    //-----------------------------------------------
    //create watchdog
    //-----------------------------------------------
    int val_watchdog = 1;
    socket_watchdog = new zmq::socket_t(*zmq_context, ZMQ_PUSH);
	socket_watchdog->setsockopt(ZMQ_RCVHWM, &val_watchdog, sizeof(val_watchdog));  //prevent buffer get overfilled
	socket_watchdog->setsockopt(ZMQ_SNDHWM, &val_watchdog, sizeof(val_watchdog));
    socket_watchdog->connect("inproc://watchdog");
    zmq::message_t msg_watchdog;
    socket_watchdog->send(msg_watchdog);

    //-----------------------------------------------
    // only for filestream mode
    //-----------------------------------------------
    unsigned int sleepTime = 0;
    LadybugStreamContext streamContext;
    LadybugStreamHeadInfo streamHeadInfo;
    unsigned int stream_image_count;
    unsigned int frame_image_count;

    //-----------------------------------------------
    // start
    //-----------------------------------------------
	status = "Create Ladybug context";
	int retry = 10;

	// create ladybug context
	error = ladybugCreateContext( &context );
	_HANDLE_ERROR
	_TIME
	status = "Initialize the camera";

    if( !filestream ){ // filestream is empty start live cam
	    // Initialize  the camera
	    error = initCamera(context);
	    _HANDLE_ERROR
	    _TIME

        status = "start ladybug live";
	    error = startLadybug(context);
	    _HANDLE_ERROR
	    _TIME
	
	    // Grab an image to inspect the image size
	    status = "Grab an image to inspect the image size";
	    error = LADYBUG_FAILED;
	    while ( error != LADYBUG_OK && retry-- > 0)	{
		    error = ladybugGrabImage( context, &image ); 
	    }
	    _HANDLE_ERROR
	    _TIME
    }
    else{
        status = "start ladybug stream";
        error = ladybugCreateStreamContext( &streamContext);
        _HANDLE_ERROR

        error = ladybugInitializeStreamForReading( streamContext, cfg_fileStream.c_str() );
        _HANDLE_ERROR 

        error = ladybugGetStreamNumOfImages( streamContext, &stream_image_count);
        _HANDLE_ERROR
	    
        error = ladybugGetStreamHeader( streamContext, &streamHeadInfo);
        _HANDLE_ERROR

        error = ladybugReadImageFromStream( streamContext, &image);
        _HANDLE_ERROR
        
        frame_image_count = getImageCount(&image);
        sleepTime = (1000 - (streamHeadInfo.frameRate * 15)) / streamHeadInfo.frameRate;
        std::string stream_configfile = "filestream.cfg";
        error = ladybugGetStreamConfigFile( streamContext , stream_configfile.c_str() );
        _HANDLE_ERROR
        //
        // Load configuration file
        //
        error = ladybugLoadConfig( context, stream_configfile.c_str() );
        _HANDLE_ERROR
      }

	status = "configure for panoramic stitching";
	error = configureLadybugForPanoramic(context);
	_HANDLE_ERROR
	_TIME

    seperatedColors = isColorSeperated(&image);
	if(seperatedColors || !cfg_transfer_compressed){
        getColorOffset(&image, red_offset, green_offset, blue_offset);
    }
	
	// Set the size of the image to be processed

    status = "inspect image size";
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

	_TIME

    { 
        std::string connection;
        int socket_type = ZMQ_PUB;
        bool zmq_bind = false;

       
       connection = cfg_ros_master.c_str();
        status = "connect with zmq to " + connection;

	    socket = new zmq::socket_t(*zmq_context, socket_type);
	    int val = 6; //buffer size
	    socket->setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	    socket->setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled
        
        if(zmq_bind){
            socket->bind(connection.c_str());
        }else{
            socket->connect(connection.c_str());
        }
	    _TIME

	    ladybug5_network::pbMessage message;
        ladybug5_network::LadybugTimeStamp msg_timestamp;

        ladybug5_network::pbFloatTriblet gyro;
        ladybug5_network::pbFloatTriblet accelerometer;
        ladybug5_network::pbFloatTriblet compass;
        ladybug5_network::pbSensor sensor;
        
        ladybug5_network::pbPosition position[6];
        ladybug5_network::pbDisortion disortion[6];

        LadybugCameraInfo info;
        ladybugGetCameraInfo(context, &info);

        for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
		{
            status = "reading camera extrinics and disortion";
            double extrinsics[6];
            error = ladybugGetCameraUnitExtrinsics(context, uiCamera, extrinsics);
            _HANDLE_ERROR

            double focal_lenght; 
            error = ladybugGetCameraUnitFocalLength(context, uiCamera, &focal_lenght);
            _HANDLE_ERROR

            double centerX, centerY;
            error = ladybugGetCameraUnitImageCenter(context , uiCamera, &centerX, &centerY);
            _HANDLE_ERROR

            position[uiCamera].set_rx(extrinsics[0]);
            position[uiCamera].set_ry(extrinsics[1]);
            position[uiCamera].set_rz(extrinsics[2]);
            position[uiCamera].set_tx(extrinsics[3]);
            position[uiCamera].set_ty(extrinsics[4]);
            position[uiCamera].set_tz(extrinsics[5]);

            //not propperly scaled for different resolution
            //disortion[uiCamera].set_centerx(centerX);
            //disortion[uiCamera].set_centery(centerY);
            //disortion[uiCamera].set_focalx(focal_lenght);
            //disortion[uiCamera].set_focaly(focal_lenght);
            _TIME
        }

        unsigned int nr = 0;
        double loopstart = t_now = clock();		
       
        printf("Running...\n");

	    while(!done)
	    {
		    try{
			    loopstart = t_now = clock();
			    t_now = loopstart;


			    // Grab an image from the camera
			    std::string status = "grab image";

			    /* Get ladybugImage */
                if( filestream ){
                    error = ladybugReadImageFromStream( streamContext, &image);
                }else{
                    error = ladybugGrabImage(context, &image); 
                }
			    _HANDLE_ERROR
			    _TIME

                /* Create and fill protobuf message */
				prefill_sensordata(message, image);

                /* Add panoramic image to pb message */
                ladybug5_network::pbImage* image_msg = 0;
			    image_msg = message.add_images();
                image_msg->set_type(ladybug5_network::LADYBUG_PANORAMIC);
			    image_msg->set_name(enumToString(image_msg->type()));
                image_msg->set_height(cfg_pano_hight);
                image_msg->set_width(cfg_pano_width);
				image_msg->set_border_left(0);
                image_msg->set_border_right(0);
                image_msg->set_border_top(0);
                image_msg->set_border_bottem(0);
				image_msg->set_packages(1);
				image_msg->set_bayer_encoding("BGR8");

                pb_send(socket, &message, ZMQ_SNDMORE);

               
			    status = "Convert images to 6 BGRU buffers";
			    // Convert the image to 6 BGRU buffers
			    error = ladybugConvertImage(context, &image, NULL);
			    _HANDLE_ERROR
			    _TIME
                    
                int flag = ZMQ_SNDMORE;
				
                     
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
			
				status = "Add panoramic image to message"; 
				unsigned char* image_data = 0;
				unsigned long  image_size = 0;
				
				if(cfg_transfer_compressed){
					//encode to jpg
					tjhandle _jpegCompressor = tjInitCompress();
					tjCompress2(_jpegCompressor, 
						processedImage.pData, 
						processedImage.uiCols, 
						0, 
						processedImage.uiRows, 
						TJPF_BGR,
						&image_data, 
						&image_size, 
						TJSAMP_420, 
						99,
						TJFLAG_FASTDCT);
					tjDestroy(_jpegCompressor);	

					zmq::message_t raw_image(image_size);
					memcpy(raw_image.data(), image_data, image_size);
					socket->send(raw_image, 0 ); // panoramic is the last image

					tjFree(image_data);
				}else{
					image_data = processedImage.pData;
					image_size = processedImage.uiCols*processedImage.uiRows*3;
					zmq::message_t raw_image(image_size);
					memcpy(raw_image.data(), image_data, image_size);
					socket->send(raw_image, 0 ); // panoramic is the last image
				}
			   
                _TIME
                
                message.Clear();
                msg_timestamp.Clear();
                ++nr;
			    status = "Sum loop";
			    t_now = loopstart;
			   
                if(filestream){
					if(!cfg_panoramic && !cfg_ladybug_colorProcessing){
                        Sleep(sleepTime); /* wait for the next frame. Only usefull for real time transfer, processing is slower than the framerate */
                    }
                    if(nr == stream_image_count){
                        //done = true; /* All images in stream are processed */
                    }
                    nr=nr%stream_image_count;
                    error = ladybugGoToImage( streamContext, nr);
                }

                _TIME
                socket_watchdog->send(msg_watchdog,ZMQ_NOBLOCK); // Loop done
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
    if(filestream)
    {
        ladybugDestroyStreamContext (&streamContext);
    }

	google::protobuf::ShutdownProtobufLibrary();
    if(socket != NULL){
        socket->close();
        delete socket;
    }
	
    if(done){
       Sleep(5000);
       threads.interrupt_all();
       Sleep(2000);
       return 1;
    }
    printf("\nWating 5 sec...\n");
    Sleep(5000);
    threads.interrupt_all();
    
	goto _RESTART;
}