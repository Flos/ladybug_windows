#include "client.h"

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