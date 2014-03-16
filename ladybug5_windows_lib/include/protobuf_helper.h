#pragma once
#include "imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include <zmq.hpp>
/*Protobuff*/
/* Serialize the ladybug5_network::pbMessage object and send it over the socket */
bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag = 0);
/* Recieve and deserialize the request to a ladybug5_network::pbMessage object*/
bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message);

