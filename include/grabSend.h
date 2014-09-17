#pragma once
#include "thread_functions.h"
#include "timing.h"

class GrabSend{
public:
    GrabSend();
	void init(zmq::context_t* zmq_context);
    int loop();
    ~GrabSend();
    bool stop;
private:
	void msg_sensordata();
    unsigned int nr;
    double t_now;
    zmq::message_t msg_watchdog;
    zmq::socket_t* socket;
    zmq::socket_t* socket_watchdog;
    zmq::context_t* zmq_context;

	Configuration config;
    Ladybug *lady;
    unsigned int uiRawCols;
	unsigned int uiRawRows;
    LadybugImage image;
	LadybugProcessedImage processedImage;
    std::string status;

    bool separatedColors;
    unsigned int red_offset;
    unsigned int green_offset;
    unsigned int blue_offset;
	std::string bayer_encoding;
	std::string color_encoding;

    ladybug5_network::pbMessage message;
    ladybug5_network::pbPosition position[LADYBUG_NUM_CAMERAS];
    ladybug5_network::pbDisortion disortion[LADYBUG_NUM_CAMERAS];
};