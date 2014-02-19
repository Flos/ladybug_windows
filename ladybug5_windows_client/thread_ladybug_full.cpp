#include "thread_functions.h"
#include "timing.h"

int thread_ladybug_full()
{
_RESTART:
	zmq::context_t zmq_context(2);
    boost::thread_group threads;
    zmq::socket_t* socket = NULL;
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
    // only on for if processing is enabled
    //-----------------------------------------------
    unsigned int arpBufferSize = 0;
	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}

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
	
	// Set the size of the image to be processed
    if(cfg_postprocessing || cfg_panoramic){
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

	    arpBufferSize = initBuffers(arpBuffers, LADYBUG_NUM_CAMERAS, uiRawCols, uiRawRows, 4);

    }else{
        uiRawCols = image.uiFullCols;
        uiRawRows = image.uiFullRows;
    }

     seperatedColors = isColorSeperated(&image);
     if(seperatedColors){
         getColorOffset(&image, red_offset, green_offset, blue_offset);
     }
	_TIME

    { 
        std::string connection;
        int socket_type = ZMQ_PUB;
        bool zmq_bind = false;

        if( cfg_transfer_compressed && (cfg_postprocessing || cfg_panoramic)){
            connection = zmq_uncompressed;
            socket_type = ZMQ_PUSH;
            zmq_bind = true;
            
            for(unsigned int i=0; i < boost::thread::hardware_concurrency(); ++i){
        	   threads.create_thread(std::bind(compressionThread, &zmq_context, i)); //worker thread (jpg-compression)
            }
            threads.create_thread(std::bind(sendingThread, &zmq_context));
        }else{
            connection = cfg_ros_master.c_str();
        }

        status = "connect with zmq to " + connection;

	    socket = new zmq::socket_t(zmq_context, socket_type);
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

            double centerX,centerY;
            error = ladybugGetCameraUnitImageCenter(context , uiCamera, &centerX, &centerY);
            _HANDLE_ERROR

            position[uiCamera].set_rx(extrinsics[0]);
            position[uiCamera].set_ry(extrinsics[1]);
            position[uiCamera].set_rz(extrinsics[2]);
            position[uiCamera].set_tx(extrinsics[3]);
            position[uiCamera].set_ty(extrinsics[4]);
            position[uiCamera].set_tz(extrinsics[5]);

            disortion[uiCamera].set_centerx(centerX);
            disortion[uiCamera].set_centery(centerY);
            disortion[uiCamera].set_focalx(focal_lenght);
            disortion[uiCamera].set_focaly(focal_lenght);
            _TIME
        }

        unsigned int nr = 0;
        double loopstart = t_now = clock();		
       
	    while(!done)
	    {
		    try{
			    loopstart = t_now = clock();
			    t_now = loopstart;

			    printf("\nGrab Image loop...\n");
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
                message.set_name("windows");
			    message.set_camera("ladybug5");
                message.set_id(nr);
                message.set_serial_number(std::to_string(info.serialBase).c_str);

                /* read the sensor data */        
                accelerometer.set_x(image.imageHeader.accelerometer.x);
                accelerometer.set_y(image.imageHeader.accelerometer.y);
                accelerometer.set_z(image.imageHeader.accelerometer.z);
                sensor.set_allocated_accelerometer(new ladybug5_network::pbFloatTriblet(accelerometer));
 
                compass.set_x(image.imageHeader.compass.x);
                compass.set_y(image.imageHeader.compass.y);
                compass.set_z(image.imageHeader.compass.z);
                sensor.set_allocated_compass(new ladybug5_network::pbFloatTriblet(compass));

                gyro.set_x(image.imageHeader.gyroscope.x);
                gyro.set_y(image.imageHeader.gyroscope.y);
                gyro.set_z(image.imageHeader.gyroscope.z);
                sensor.set_allocated_gyroscope(new ladybug5_network::pbFloatTriblet(gyro));

                sensor.set_humidity(image.imageHeader.uiHumidity);
                sensor.set_barometer(image.imageHeader.uiAirPressure);
                sensor.set_temperature(image.imageHeader.uiTemperature);
                message.set_allocated_sensors(new ladybug5_network::pbSensor(sensor));
                
		
			    msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
			    msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
			    msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
			    msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
			    msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);
                message.set_allocated_time(new ladybug5_network::LadybugTimeStamp(msg_timestamp));

                for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
		        {
			        ladybug5_network::pbImage* image_msg = 0;
			        image_msg = message.add_images();
			        image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
			        image_msg->set_name(enumToString(image_msg->type()));
			        image_msg->set_height(uiRawRows);
                    image_msg->set_width(uiRawCols);
                    image_msg->set_allocated_distortion(new ladybug5_network::pbDisortion(disortion[uiCamera]));
                    image_msg->set_allocated_position(new ladybug5_network::pbPosition(position[uiCamera]));
                    if(seperatedColors && !cfg_postprocessing && !cfg_panoramic){
                        image_msg->set_packages(3);
                    }else{
                        image_msg->set_packages(1);
                    }
                }

                /* Add panoramic image to pb message */
                if(cfg_panoramic){  
                    ladybug5_network::pbImage* image_msg = 0;
			        image_msg = message.add_images();
                    image_msg->set_type(ladybug5_network::LADYBUG_PANORAMIC);
			        image_msg->set_name(enumToString(image_msg->type()));
                    image_msg->set_height(cfg_pano_hight);
                    image_msg->set_width(cfg_pano_width);
                    image_msg->set_packages(1);
                }

                pb_send(socket, &message, ZMQ_SNDMORE);

                if(cfg_postprocessing || cfg_panoramic)
                {
			        status = "Convert images to 6 BGRU buffers";
			        // Convert the image to 6 BGRU buffers
			        error = ladybugConvertImage(context, &image, arpBuffers);
			        _HANDLE_ERROR
			        _TIME
                    
                    int flag = ZMQ_SNDMORE;
				    status = "Adding images with processing";
				    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
				    {
					    zmq::message_t raw_image(arpBufferSize);
                        memcpy(raw_image.data(), arpBuffers[uiCamera], arpBufferSize);
                           
                        if( !cfg_panoramic && uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                            flag = 0;
                        }
                        socket->send(raw_image, flag ); // send BGRU images
				    }
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
                        unsigned int size = processedImage.uiCols*processedImage.uiRows*3;
                        zmq::message_t raw_image(size);
                        memcpy(raw_image.data(), processedImage.pData, size);                    
                        socket->send(raw_image, 0 ); // panoramic is the last image
                        _TIME
			        }
			        status = "send img over network";
                    
                    _TIME
                }else{ //No post processing, no panoramic picture
                    
                    status = "send image " + std::to_string(nr);
                    int flag = ZMQ_SNDMORE;

                    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
                    {
                        unsigned int index = uiCamera*4;
                        //send images 
                     
                        unsigned int r_size, g_size, b_size;
                        char* r_data = NULL;
                        char* g_data = NULL;
                        char* b_data = NULL;

                        extractImageToMsg(&image, index+blue_offset,    &b_data, b_size);
                        extractImageToMsg(&image, index+green_offset,   &g_data, g_size);
                        extractImageToMsg(&image, index+red_offset,     &r_data, r_size);
                            
                        if( uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                            flag = 0;
                        }

                        if(seperatedColors){

                            //RGB expected at reciever
                            zmq::message_t R(r_size);
                            memcpy(R.data(), r_data, r_size);
                            socket->send(R, ZMQ_SNDMORE ); // Red = Index + 3

                            zmq::message_t G(g_size);
                            memcpy(G.data(), g_data, g_size);
                            socket->send(G, ZMQ_SNDMORE ); // Green = Index + 1 || 2

                            zmq::message_t B(b_size);     // Blue = Index 0
                            memcpy(B.data(), b_data, b_size);
                            socket->send(B, flag );

                        }else{ /* RGGB RAW */
                            assert(r_size == g_size);
                            assert(r_size == b_size);
                            zmq::message_t image(r_size*3);
                            memcpy(image.data(), r_data, r_size);
                            memcpy((char*)image.data()+r_size, g_data, g_size);
                            memcpy((char*)image.data()+r_size*2, b_data, b_size);
                            socket->send(image, flag );
                        }          
                    }
                }
                message.Clear();
                msg_timestamp.Clear();
                ++nr;
			    status = "Sum loop";
			    t_now = loopstart;
			   
                if(filestream){
                    if(!cfg_panoramic && !cfg_postprocessing){
                        Sleep(sleepTime); /* wait for the next frame. Only usefull for real time transfer, processing is slower than the framerate */
                    }
                    if(nr == stream_image_count){
                        //done = true; /* All images in stream are processed */
                    }
                    nr=nr%stream_image_count;
                    error = ladybugGoToImage( streamContext, nr);
                }
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
    if(filestream)
    {
        ladybugDestroyStreamContext (&streamContext);
    }

	google::protobuf::ShutdownProtobufLibrary();
    if(socket != NULL){
        socket->close();
        delete socket;
    }
	
	for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ ){
		if ( arpBuffers[ uiCamera ] != NULL )
		{
			delete  [] arpBuffers[ uiCamera ];
		}
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