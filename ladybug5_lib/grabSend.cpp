#include "grabSend.h"

GrabSend::GrabSend(){
    socket = NULL;
    socket_watchdog = NULL;
    uiRawCols = 0;
    uiRawRows = 0;
    seperatedColors = false;
    stop = false;
	lady = NULL;

    

}

void
GrabSend::init(zmq::context_t* zmq_context)
{
	Configuration config;

	if(config.cfg_panoramic){
		config.cfg_panoramic=false;
		printf("Warning: panoramic image creation not supported in grabber...\n");
	}

	if(config.cfg_transfer_compressed){
		config.cfg_transfer_compressed=false;
		printf("Warning: compressed image transfer is not supported in grabber...\n");
	}

	if(config.cfg_ladybug_dataformat != LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8 
		|| config.cfg_ladybug_dataformat != LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8
		|| config.cfg_ladybug_dataformat != LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12
		|| config.cfg_ladybug_dataformat != LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12
		){
		config.cfg_ladybug_dataformat = LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8;
		printf("Warning: only jpeg color seperated dataformats are supported...\n");
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
   
    seperatedColors = isColorSeperated(&image);
    if(seperatedColors){
        getColorOffset(&image, red_offset, green_offset, blue_offset);
    }
	else{
		throw new std::exception("only jepeg color sep images are supported in grabber");
	}
	
	// Set the size of the image to be processed
    
    uiRawCols = image.uiFullCols / 2;
	uiRawRows = image.uiFullRows / 2;
  
	_TIME
    
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
    }

    unsigned int nr = 0;
    double loopstart = t_now = clock();		
       
    printf("Running...\n");
    socket_watchdog->send(msg_watchdog, ZMQ_NOBLOCK);
}

int
GrabSend::loop(){
    while(!stop){
		try{
			double loopstart = t_now = clock();
			t_now = loopstart;

			// Grab an image from the camera
			std::string status = "grab image";

			/* Get ladybugImage */
			lady->grabImage(&image);
			_HANDLE_ERROR_LADY
			_TIME

			prefill_sensordata(message, image); 
   
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
        
				image_msg->set_packages(3);
				image_msg->set_border_left(image.imageBorder.uiLeftCols/2);
				image_msg->set_border_right(image.imageBorder.uiRightCols/2);
				image_msg->set_border_top(image.imageBorder.uiTopRows/2);
				image_msg->set_border_bottem(image.imageBorder.uiBottomRows/2);
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
                     
					//RGB expected at reciever
					// Red = Index + 3
					send_image(index+red_offset, &image, socket, flag);

					// Green = Index + 1 || 2
					send_image(index+green_offset, &image, socket, flag);

					// Blue = Index 0
        			send_image(index+blue_offset, &image, socket, flag);
                        
				}else{ /* RGGB RAW */
					send_image(uiCamera, &image, socket, flag);
				}
			} // end uiCamera loop

			message.Clear();
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