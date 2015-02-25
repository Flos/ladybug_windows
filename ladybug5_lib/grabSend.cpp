#include "grabSend.h"

GrabSend::GrabSend(){
    socket = NULL;
    socket_watchdog = NULL;
    uiRawCols = 0;
    uiRawRows = 0;
    separatedColors = false;
    stop = false;
	lady = NULL;
	nr = 0;
    

}

void
GrabSend::init(zmq::context_t* zmq_context)
{	
	if(config.cfg_panoramic){
		config.cfg_panoramic=false;
		printf("Warning: panoramic image creation not supported in grabber...\n");
	}

	if(config.cfg_transfer_compressed){
		config.cfg_transfer_compressed=false;
		printf("Warning: compressed image transfer is not supported in grabber...\n");
	}

	lady = new Ladybug();
	lady->init(&config);

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
   
    separatedColors = isColorSeparated(&image);
	bayer_encoding = getBayerEncoding(&image) +  std::to_string(lady->config->get_image_depth());
	color_encoding = lady->config->get_color_encoding();

	printf("Sending color: %s bayer: %s depth: %ibit\n", color_encoding.c_str(), bayer_encoding.c_str(), lady->config->get_image_depth());

    if(separatedColors){
        getColorOffset(&image, red_offset, green_offset, blue_offset);
		uiRawCols = image.uiFullCols / 2;
		uiRawRows = image.uiFullRows / 2;
    }
	else{
		//printf("Exception: only jepeg color sep images are supported in grabber\n");
		//throw new std::exception("only jepeg color sep images are supported in grabber");

		uiRawCols = image.uiFullCols;
		uiRawRows = image.uiFullRows;
	}

	_TIME
    
    std::string connection;
	int socket_type = ZMQ_PUB;
    bool zmq_bind = false;
 
    connection = cfg_ros_master.c_str();

    status = "connect with zmq to " + connection;

	socket = new zmq::socket_t(*zmq_context, socket_type);
	int val = 2; //buffer size
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
            
        disortion[uiCamera].set_centerx(calib.centerX/2);
        disortion[uiCamera].set_centery(calib.centerY/2);
        disortion[uiCamera].set_focalx(calib.focal_lenght/2); //TODO: Check it thats right??
        disortion[uiCamera].set_focaly(calib.focal_lenght/2);
        _TIME

		ladybug5_network::pbImage* image_msg = 0;
		image_msg = message.add_images();
		image_msg->set_type((ladybug5_network::ImageType) ( 1 << uiCamera));
		image_msg->set_name(enumToString(image_msg->type()));
		image_msg->set_height(uiRawRows);
		image_msg->set_width(uiRawCols);
		image_msg->set_allocated_distortion(new ladybug5_network::pbDisortion(disortion[uiCamera]));
		image_msg->set_allocated_position(new ladybug5_network::pbPosition(position[uiCamera]));
        
		image_msg->set_border_left(image.imageBorder.uiLeftCols/2);
		image_msg->set_border_right(image.imageBorder.uiRightCols/2);
		image_msg->set_border_top(image.imageBorder.uiTopRows/2);
		image_msg->set_border_bottem(image.imageBorder.uiBottomRows/2);
		image_msg->set_color_encoding(color_encoding);
		image_msg->set_bayer_encoding(bayer_encoding);
		image_msg->set_depth(lady->config->get_image_depth());

		if(separatedColors){
			image_msg->set_packages(3);
		}
		else{
			image_msg->set_packages(1);
		}
		_TIME
    }

    unsigned int nr = 0;
    double loopstart = t_now = clock();		
       
    printf("Running...\n");
    socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK);
}

int
GrabSend::loop(){
	std::string status = "loop init";
	double loopstart;	
	
    while(!stop){
		try{
			loopstart = t_now = clock();
			t_now = loopstart;

			// Grab an image from the camera
			status = "wait for image";
			_TIME
			/* Get ladybugImage */
			lady->grabImage(&image);
			_HANDLE_ERROR_LADY
			// Grab an image from the camera
			status = "got images";
			_TIME

			prefill_sensordata(message, image); 
			status = "get sensordata";
			_TIME

			//send protobuff message
			pb_send(socket, &message, ZMQ_SNDMORE); 
			status = "send header";
			_TIME

			status = "extract images " + std::to_string(nr);
			int flag = ZMQ_SNDMORE;

			for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
			{
				if( uiCamera == LADYBUG_NUM_CAMERAS-1 ){
						flag = 0;
				}

				if(separatedColors)
				{
					unsigned int index = uiCamera*4;
					//send images 
                     
					//RGB expected at reciever
					// Red = Index + 3
					send_image(index+red_offset, &image, socket, ZMQ_SNDMORE);

					// Green = Index + 1 || 2
					send_image(index+green_offset, &image, socket, ZMQ_SNDMORE);

					// Blue = Index 0
        			send_image(index+blue_offset, &image, socket, flag);
                        
				}else{ /* RGGB RAW */
					send_image(uiCamera, &image, socket, flag);
				}
			} // end uiCamera loop

			_TIME
			//message.Clear();
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
		catch(std::exception e){
			stop = true;
		}
	}
	return 0;
	
_EXIT:
    return 1;
}

GrabSend::~GrabSend(){
    stop = true;
	Sleep(500);

	if(socket != NULL) 
	{
		socket->close();
		delete socket;
	}

	if(socket_watchdog != NULL) 
	{
		socket_watchdog->close();
		delete socket_watchdog;
	}
	
	if(lady != NULL) delete lady;
}