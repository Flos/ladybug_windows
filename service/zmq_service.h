#pragma once
#include "stdafx.h"
#include <zmq.hpp>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <imageMessage.pb.h>

class Zmq_service
{
public:
	Zmq_service(void);
	void init(std::string socket, int type);

	bool send(google::protobuf::Message& pb_msg, int flag = 0);
	bool send(ladybug5_network::pb_start_msg& pb_msg, int flag = 0);
	bool send(ladybug5_network::pb_reply& pb_msg, int flag = 0);
	bool send(ladybug5_network::pbMessage& pb_msg, int flag = 0);
	bool send(std::string &string, int flag = 0);
	bool send(zmq::message_t &msg, int flag = 0);
	
	bool receive(google::protobuf::Message& pb_msg, int flag = 0);
	bool receive(ladybug5_network::pb_start_msg& pb_msg, int flag = 0);
	bool receive(ladybug5_network::pb_reply& pb_msg, int flag = 0);
	bool receive(ladybug5_network::pbMessage& pb_msg, int flag = 0);
	bool receive(zmq::message_t &msg, int flag = 0);

	~Zmq_service(void);
private:
	zmq::context_t*zmq_context; 
	zmq::socket_t* zmq_socket;
};

