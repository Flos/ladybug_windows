#include "timing.h"

double time_diff(std::string status, double start, std::string name){
	double t_now = clock();
	//printf("%s : %f \n", s, seconds);
    printf("Time: %f : In %s\t to pass %s\n", (t_now-start)/CLOCKS_PER_SEC,  name.c_str(), status.c_str());
 
	return t_now;
}


std::string enumToString(ladybug5_network::ImageType type){
	switch (type){
	case ladybug5_network::LADYBUG_DOME:
		return "LADYBUG_DOME";
	case ladybug5_network::LADYBUG_PANORAMIC:
		return "LADYBUG_PANORAMIC";
	case ladybug5_network::LADYBUG_RAW_CAM0:
		return "LADYBUG_RAW_CAM0";
	case ladybug5_network::LADYBUG_RAW_CAM1:
		return "LADYBUG_RAW_CAM1";
	case ladybug5_network::LADYBUG_RAW_CAM2:
		return "LADYBUG_RAW_CAM2";
	case ladybug5_network::LADYBUG_RAW_CAM3:
		return "LADYBUG_RAW_CAM3";
	case ladybug5_network::LADYBUG_RAW_CAM4:
		return "LADYBUG_RAW_CAM4";
	case ladybug5_network::LADYBUG_RAW_CAM5:
		return "LADYBUG_RAW_CAM5";
	default:
		return "UNKOWN";
	}
}