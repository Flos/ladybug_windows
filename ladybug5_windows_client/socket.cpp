#include "socket.h"
#include "assert.h"
#include <iostream>

ladybug5_network::ImageMessage*
socket_read(zmq::socket_t* socket){

  std::cout << "reading from socket" << std::endl;
  std::string s_recived;
  zmq::message_t msg_recieved;
  socket->recv(&msg_recieved);
  std::cout << "message recieved" << std::endl;

  ladybug5_network::ImageMessage* pb_message = new ladybug5_network::ImageMessage();
  pb_message->ParseFromArray(msg_recieved.data(),msg_recieved.size());

  std::cout << "parsing of " << msg_recieved.size() << " Byte done" <<  std::endl<<  std::endl;

  return pb_message;
}

void
socket_write(zmq::socket_t* socket, ladybug5_network::ImageMessage* message){
  // Socket to talk to clients
    std::cout << "Try to send "<< message->ByteSize() << " Bytes " << std::endl;

    zmq::message_t request (message->ByteSize());
    message->SerializeToArray(request.data(), message->ByteSize());

	double seconds=0;
	time_t timer1, timer2;
	time(&timer1);  /* get current time  */
	 socket->send(request);
	time(&timer2);  /* get current time  later */
	seconds = difftime(timer2,timer1);

   /* timeval start, end;
    gettimeofday(&start, 0);
    socket->send(request);
    gettimeofday(&end, 0);
*/
    std::cout << "msg send : " << message->ByteSize() << " Bytes in " << seconds << " Seconds" << std::endl <<  std::endl;
  return;
}
