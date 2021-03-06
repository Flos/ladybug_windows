#pragma once
#include "imageMessage.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include <zmq.hpp>
#include "error.h"
#include "timing.h"
#include "ladybug_stream.h"

/*Protobuff*/
/* Serialize the ladybug5_network::pbMessage object and send it over the socket */
bool pb_send(zmq::socket_t* socket, const ladybug5_network::pbMessage* pb_message, int flag = 0);
/* Recieve and deserialize the request to a ladybug5_network::pbMessage object*/
bool pb_recv(zmq::socket_t* socket, ladybug5_network::pbMessage* pb_message);

std::string enumToString(ladybug5_network::ImageType type);

void prefill_sensordata( ladybug5_network::pbMessage& message, LadybugImage &image);

void send_image( unsigned int index, LadybugImage *image, zmq::socket_t *socket, int flag);
