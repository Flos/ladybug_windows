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