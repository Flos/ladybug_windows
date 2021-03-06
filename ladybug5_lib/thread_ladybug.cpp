#include "thread_functions.h"
#include "timing.h"

int thread_ladybug()
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
    
    Ladybug* lady = NULL;
    while(true){
        if(lady != NULL && lady->error != LADYBUG_OK){
            delete lady;
            lady = NULL;
        }
        if(lady == NULL){
            lady = new Ladybug();
        }
    

    std::string status;
    bool filestream = lady->config->cfg_fileStream.size() > 0;
    bool done = false;
 
    bool separatedColors = false;
    unsigned int red_offset,green_offset,blue_offset;

    //-----------------------------------------------
    // only for filestream mode
    //-----------------------------------------------
    unsigned int sleepTime = 0;
	_TIME

     separatedColors = isColorSeparated(&image);
     if(separatedColors || !lady->config->cfg_transfer_compressed){
         getColorOffset(&image, red_offset, green_offset, blue_offset);
     }
	_TIME

    { 
        std::string connection;
        int socket_type = ZMQ_PUB;
        bool zmq_bind = false;

        if( lady->config->cfg_transfer_compressed && (lady->config->cfg_ladybug_colorProcessing || lady->config->cfg_panoramic)){
            connection = zmq_uncompressed;
            socket_type = ZMQ_PUSH;
            zmq_bind = true;
            
            for(unsigned int i=0; i < boost::thread::hardware_concurrency(); ++i){
        	   threads.create_thread(std::bind(compressionThread, &zmq_context, i)); //worker thread (jpg-compression)
            }
            threads.create_thread(std::bind(sendingThread, &zmq_context));
        }else{
            connection = lady->config->cfg_ros_master.c_str();
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

        for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
		{
            CameraCalibration calib; 
            error = lady->getCameraCalibration(uiCamera, &calib);
            status = "reading camera extrinics and disortion";
            _HANDLE_ERROR
                
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

	    while(!done)
	    {
		    try{
			    loopstart = t_now = clock();
			    t_now = loopstart;

			    // Grab an image from the camera
			    std::string status = "grab image";

			    /* Get ladybugImage */
                error = lady->grabImage(&image);
                
			    _HANDLE_ERROR
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
                    if(separatedColors && !lady->config->cfg_ladybug_colorProcessing && !lady->config->cfg_panoramic){
                        image_msg->set_packages(3);
                    }else{
                        image_msg->set_packages(1);
                    }
                }

                /* Add panoramic image to pb message */
                if(lady->config->cfg_panoramic){  
                    ladybug5_network::pbImage* image_msg = 0;
			        image_msg = message.add_images();
                    image_msg->set_type(ladybug5_network::LADYBUG_PANORAMIC);
			        image_msg->set_name(enumToString(image_msg->type()));
                    image_msg->set_height(lady->config->cfg_pano_hight);
                    image_msg->set_width(lady->config->cfg_pano_width);
                    image_msg->set_packages(1);
                }

                pb_send(socket, &message, ZMQ_SNDMORE);

                if(lady->config->cfg_ladybug_colorProcessing || lady->config->cfg_panoramic)
                {                    
                    int flag = ZMQ_SNDMORE;
				    status = "Adding images with processing";
				    for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
				    {
                        zmq::message_t raw_image(lady->getBuffer()->size);
                        memcpy(raw_image.data(), lady->getBuffer()->getBuffer(uiCamera), lady->getBuffer()->size);
                           
                        if( !lady->config->cfg_panoramic && uiCamera == LADYBUG_NUM_CAMERAS-1 ){
                            flag = 0;
                        }
                        socket->send(raw_image, flag ); // send BGRU images
				    }
				    _TIME

                    if(lady->config->cfg_panoramic){
				        status = "create panorame in graphics card";
				        // Stitch the images (inside the graphics card) and retrieve the output to the user's memory
				        LadybugProcessedImage processedImage;
                        error = lady->grabProcessedImage(&processedImage, LADYBUG_PANORAMIC);
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

                        if(separatedColors){

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
                            //zmq::message_t image(r_size);
                            //memcpy(image.data(), r_data, r_size);
                          /*  memcpy((char*)image.data()+r_size, r_data, g_size);
                            memcpy((char*)image.data()+r_size*2, r_data, b_size);*/
                            //socket->send(image, flag );
                            throw new std::exception("RGB8 uncompressed not supported");
                        }          
                    }
                }
                message.Clear();
                msg_timestamp.Clear();
                ++nr;
			    status = "Sum loop";
			    t_now = loopstart;
			   
                if(filestream){
                    if(!lady->config->cfg_panoramic && !lady->config->cfg_ladybug_colorProcessing){
                        Sleep(sleepTime); /* wait for the next frame. Only usefull for real time transfer, processing is slower than the framerate */
                    }
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
}