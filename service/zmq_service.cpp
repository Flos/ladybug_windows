#include "stdafx.h"
#include "zmq_service.h"


Zmq_service::Zmq_service(void)
{
	zmq_context = NULL;
	zmq_socket = NULL;
}


Zmq_service::~Zmq_service(void)
{
	zmq_socket->close();
	delete zmq_context;
	delete zmq_socket;
}

bool 
Zmq_service::send(google::protobuf::Message* pb_msg, int flag){
	std::string pb_serialized;
	pb_msg->SerializeToString(&pb_serialized);

	// create and send the zmq message
	return send(pb_serialized, flag);
}

bool 
Zmq_service::send(ladybug5_network::pb_start_msg* pb_msg, int flag){
	std::string pb_serialized;
	pb_msg->SerializeToString(&pb_serialized);

	// create and send the zmq message
	return send(pb_serialized, flag);
}

bool 
Zmq_service::send(ladybug5_network::pb_reply* pb_msg, int flag){
	std::string pb_serialized;
	pb_msg->SerializeToString(&pb_serialized);

	// create and send the zmq message
	return send(pb_serialized, flag);
}

bool 
Zmq_service::send(ladybug5_network::pbMessage* pb_msg, int flag){
	std::string pb_serialized;
	pb_msg->SerializeToString(&pb_serialized);

	// create and send the zmq message
	return send(pb_serialized, flag);
}

bool 
Zmq_service::send(std::string &string, int flag){
	zmq::message_t msg (string.size());
	memcpy ((void *) msg.data(), string.c_str(), string.size());

	return send(msg, flag);
}

bool 
Zmq_service::send(zmq::message_t &msg, int flag){
	return zmq_socket->send(msg, flag);
}

bool 
Zmq_service::receive(google::protobuf::Message* pb_msg, int flag){
	zmq::message_t msg;
	
	if(!receive(msg, flag)) return false;
	pb_msg->ParseFromArray(msg.data(), msg.size());
	return true;
}

bool 
Zmq_service::receive(ladybug5_network::pb_start_msg* pb_msg, int flag){
	zmq::message_t msg;
	
	if(!receive(msg, flag)) return false;
	pb_msg->ParseFromArray(msg.data(), msg.size());
	return true;
}

bool 
Zmq_service::receive(ladybug5_network::pb_reply* pb_msg, int flag){
	zmq::message_t msg;
	
	if(!receive(msg, flag)) return false;
	pb_msg->ParseFromArray(msg.data(), msg.size());
	return true;
}

bool 
Zmq_service::receive(ladybug5_network::pbMessage* pb_msg, int flag){
	zmq::message_t msg;
	
	if(!receive(msg, flag)) return false;
	pb_msg->ParseFromArray(msg.data(), msg.size());
	return true;
}

bool 
Zmq_service::receive(zmq::message_t &msg, int flag){
	return zmq_socket->recv(&msg, flag);
}

void
Zmq_service::init(std::string socket, int type)
{
	//ZMQ
	zmq_context = new zmq::context_t();
    zmq_socket = new zmq::socket_t(*zmq_context, type);

    int val = 1;
	zmq_socket->setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	zmq_socket->setsockopt(ZMQ_SNDHWM, &val, sizeof(val));
	printf("socket: %s\n", socket.c_str());
	switch (type){
		case ZMQ_REQ:
		case ZMQ_PUSH:
			zmq_socket->connect(socket.c_str());
			break;
		default:
			zmq_socket->bind(socket.c_str());
			break;
	}
}
