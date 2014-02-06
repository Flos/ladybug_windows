#ifndef TIMING_H_
#define TIMING_H_

#include <iostream>
#include <time.h>
#include <string>
#include <ladybug.h>
#include "protobuf\imageMessage.pb.h"



#ifndef _TIME
#define _TIME \
	t_now = time_diff(status, t_now, __FUNCTION__);\

#endif

double time_diff(std::string status, double start, std::string name);
std::string enumToString(ladybug5_network::ImageType type);
#endif