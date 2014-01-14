#include "socket.h"
#include "assert.h"
#include <iostream>

ladybug5_network::ImageMessage*
socket_read(zmq::socket_t* socket){
  std::string s_recived;
  zmq::message_t msg_recieved;
  socket->recv(&msg_recieved);
  ladybug5_network::ImageMessage* pb_message = new ladybug5_network::ImageMessage();
  pb_message->ParseFromArray(msg_recieved.data(),msg_recieved.size());
  return pb_message;
}

void
socket_write(zmq::socket_t* socket, ladybug5_network::ImageMessage* message){
  // Socket to talk to clients
    zmq::message_t request (message->ByteSize());
    message->SerializeToArray(request.data(), message->ByteSize());	
	socket->send(request);
  return;
}
