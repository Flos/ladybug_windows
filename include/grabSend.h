#pragma once
#include "thread_functions.h"
#include "timing.h"

class GrabSend{
public:
    GrabSend(zmq::context_t* zmq_context);
    int loop();
    ~GrabSend();
    boost::thread_group threads;
    bool stop;
private:
	void msg_sensordata();
    unsigned int nr;
    double t_now;
    zmq::message_t msg_watchdog;
    zmq::socket_t* socket;
    zmq::socket_t* socket_watchdog;
    zmq::context_t* zmq_context;
    Ladybug *lady;
    unsigned int uiRawCols;
	unsigned int uiRawRows;
    LadybugImage image;
	LadybugProcessedImage processedImage;
    std::string status;
    bool seperatedColors;
    unsigned int red_offset;
    unsigned int green_offset;
    unsigned int blue_offset;
    ladybug5_network::pbMessage message;
    ladybug5_network::LadybugTimeStamp msg_timestamp;

    ladybug5_network::pbFloatTriblet gyro;
    ladybug5_network::pbFloatTriblet accelerometer;
    ladybug5_network::pbFloatTriblet compass;
    ladybug5_network::pbSensor sensor;
        
    ladybug5_network::pbPosition position[6];
    ladybug5_network::pbDisortion disortion[6];
};