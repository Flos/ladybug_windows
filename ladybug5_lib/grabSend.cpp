#include "grabSend.h"

GrabSend::GrabSend(zmq::context_t* zmq_context){
    socket = NULL;
    socket_watchdog = NULL;
    uiRawCols = 0;
    uiRawRows = 0;
    seperatedColors = false;
    stop = false;

    Configuration config;

	if(config.cfg_panoramic){
		config.cfg_panoramic=false;
		printf("panoramic image creation not supported in grabber...\n");
	}

	if(config.cfg_transfer_compressed){
		config.cfg_transfer_compressed=false;
		printf("compressed image transfer is not supported in grabber...\n");
	}

    lady = new Ladybug(&config);
   
     //-----------------------------------------------
    //create watchdog
    //-----------------------------------------------
    int val_watchdog = 1;
    socket_watchdog = new zmq::socket_t(*zmq_context, ZMQ_PUSH);
	socket_watchdog->setsockopt(ZMQ_RCVHWM, &val_watchdog, sizeof(val_watchdog));  //prevent buffer get overfilled
	socket_watchdog->setsockopt(ZMQ_SNDHWM, &val_watchdog, sizeof(val_watchdog));
    socket_watchdog->connect("inproc://watchdog");
    socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK);

    lady->grabImage(&image);
   
    seperatedColors = isColorSeperated(&image);
    if(seperatedColors || !lady->config->cfg_transfer_compressed){
        getColorOffset(&image, red_offset, green_offset, blue_offset);
    }
	
	// Set the size of the image to be processed
    if(lady->config->cfg_rectification || lady->config->cfg_panoramic){
        status = "inspect image size";
        if (lady->config->cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
	        lady->config->cfg_ladybug_colorProcessing == LADYBUG_MONO)
        {
	        uiRawCols = image.uiCols / 2;
	        uiRawRows = image.uiRows / 2;
        }else{
	        uiRawCols = image.uiCols;
	        uiRawRows = image.uiRows;
        }
    }else if(seperatedColors){
        uiRawCols = image.uiFullCols / 2;
	    uiRawRows = image.uiFullRows / 2;
    }
    else{
        uiRawCols = image.uiFullCols;
        uiRawRows = image.uiFullRows;
    }     
	_TIME
    
    std::string connection;
    int socket_type = ZMQ_PUB;
    bool zmq_bind = false;

    if( lady->config->cfg_transfer_compressed && (lady->config->cfg_rectification || cfg_panoramic)){
        connection = zmq_uncompressed;
        socket_type = ZMQ_PUSH;
        zmq_bind = true;
            
        for(unsigned int i=0; i < boost::thread::hardware_concurrency(); ++i){
        	threads.create_thread(std::bind(compressionThread, zmq_context, i)); //worker thread (jpg-compression)
        }
        threads.create_thread(std::bind(sendingThread, zmq_context));
    }else{
        connection = cfg_ros_master.c_str();
    }

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
    
    socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK);
    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ ){
        status = "reading camera extrinics and disortion";
           
        CameraCalibration calib;
        lady->getCameraCalibration(uiCamera, &calib);
            
        position[uiCamera].set_rx(calib.rotationX);
        position[uiCamera].set_ry(calib.rotationY);
        position[uiCamera].set_rz(calib.rotationZ);
        position[uiCamera].set_tx(calib.translationX);
        position[uiCamera].set_ty(calib.translationY);
        position[uiCamera].set_tz(calib.translationZ);
            
        disortion[uiCamera].set_centerx(calib.centerX);
        disortion[uiCamera].set_centery(calib.centerY);
        disortion[uiCamera].set_focalx(calib.focal_lenght);
        disortion[uiCamera].set_focaly(calib.focal_lenght);
        _TIME
    }

    unsigned int nr = 0;
    double loopstart = t_now = clock();		
       
    printf("Running...\n");
    socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK);
}

int
GrabSend::loop(){
    while(!stop){
	    double loopstart = t_now = clock();
	    t_now = loopstart;

	    // Grab an image from the camera
	    std::string status = "grab image";

	    /* Get ladybugImage */
        lady->grabImage(&image);
	    _HANDLE_ERROR_LADY
	    _TIME

        /* Create and fill protobuf message */
        message.set_name("windows");
	    message.set_camera("ladybug5");
        message.set_id(nr);
        message.set_serial_number(std::to_string(lady->caminfo.serialBase));

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
            image_msg->set_packages(1);

            if( !seperatedColors){
                if(!lady->config->cfg_rectification){ // rectified images dont have a border
                    image_msg->set_border_left(image.imageBorder.uiLeftCols);
                    image_msg->set_border_right(image.imageBorder.uiRightCols);
                    image_msg->set_border_top(image.imageBorder.uiTopRows);
                    image_msg->set_border_bottem(image.imageBorder.uiBottomRows);
                }
            }else{
                image_msg->set_packages(3);
                image_msg->set_border_left(image.imageBorder.uiLeftCols/2);
                image_msg->set_border_right(image.imageBorder.uiRightCols/2);
                image_msg->set_border_top(image.imageBorder.uiTopRows/2);
                image_msg->set_border_bottem(image.imageBorder.uiBottomRows/2);
            }
        }

        //send protobuff message
        pb_send(socket, &message, ZMQ_SNDMORE); 

        status = "extract images " + std::to_string(nr);
        int flag = ZMQ_SNDMORE;

        for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
        {
			if( uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                    flag = 0;
            }

            if(seperatedColors)
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
                unsigned int image_size = 0;
                char* image_data = NULL;
                       
                extractImageToMsg(&image, uiCamera, &image_data, image_size);
                zmq::message_t msg_image(image_size);
                memcpy(msg_image.data(), image_data, image_size);

				socket->send(msg_image, flag );
            }
        } // end uiCamera loop

        message.Clear();
        msg_timestamp.Clear();
        ++nr;
	    
        socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK); // Loop done
        
        if(lady->isFileStream()){
            double sleepTime = lady->getCycleTime() - (clock() - loopstart) -1;
            if(sleepTime > 0){
                Sleep(sleepTime);
            }
        }
        else{
            Sleep(1); // Chance to terminate the thread
        }
        status = "Sum loop";
	    t_now = loopstart;
        _TIME
    }
    return 0;
_EXIT:
    return 1;
}

GrabSend::~GrabSend(){
    stop = true;
    socket->close();
    delete socket;
    socket_watchdog->close();
    delete socket_watchdog;
    threads.interrupt_all();
    delete lady;
}