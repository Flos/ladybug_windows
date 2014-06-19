#include "protobuf_helper.h"

bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag){
	// serialize the request to a string
	std::string pb_serialized;
	pb_message->SerializeToString(&pb_serialized);
    
	// create and send the zmq message
	zmq::message_t request (pb_serialized.size());
	memcpy ((void *) request.data (), pb_serialized.c_str(), pb_serialized.size());
	return socket->send(request, flag);
}

bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message){
	
	zmq::message_t zmq_msg;
	bool result = socket->recv(&zmq_msg);
	pb_message->ParseFromArray(zmq_msg.data(), zmq_msg.size());
	return result;
}



std::string enumToString(ladybug5_network::ImageType type){
	switch (type){
	case ladybug5_network::LADYBUG_DOME:
		return "LADYBUG_DOME";
	case ladybug5_network::LADYBUG_PANORAMIC:
		return "LADYBUG_PANORAMIC";
	case ladybug5_network::LADYBUG_RAW_CAM0:
		return "LADYBUG_RAW_CAM0";
	case ladybug5_network::LADYBUG_RAW_CAM1:
		return "LADYBUG_RAW_CAM1";
	case ladybug5_network::LADYBUG_RAW_CAM2:
		return "LADYBUG_RAW_CAM2";
	case ladybug5_network::LADYBUG_RAW_CAM3:
		return "LADYBUG_RAW_CAM3";
	case ladybug5_network::LADYBUG_RAW_CAM4:
		return "LADYBUG_RAW_CAM4";
	case ladybug5_network::LADYBUG_RAW_CAM5:
		return "LADYBUG_RAW_CAM5";
	default:
		return "UNKOWN";
	}
}

void prefill_sensordata( ladybug5_network::pbMessage& message, LadybugImage &image){
		ladybug5_network::pbFloatTriblet gyro;
		ladybug5_network::pbFloatTriblet accelerometer;
		ladybug5_network::pbFloatTriblet compass;
		ladybug5_network::pbSensor sensor;
		ladybug5_network::LadybugTimeStamp msg_timestamp;

		/* Create and fill protobuf message */
        message.set_name("windows");
	    message.set_camera("ladybug5");
		message.set_id(image.imageInfo.ulSerialNum);
		
        message.set_serial_number(std::to_string(image.imageInfo.ulSerialNum));

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
}

void send_image( unsigned int index, LadybugImage *image, zmq::socket_t *socket, int flag){
	//send images 
                     
	unsigned int image_size;
	char* image_data = NULL;

	extractImageToMsg(image, index, &image_data, image_size);
                            
	//RGB expected at reciever
	zmq::message_t zmq_image(image_size);
	memcpy(zmq_image.data(), image_data, image_size);
	socket->send(zmq_image, flag ); 
}