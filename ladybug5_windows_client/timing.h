#ifndef TIMING_H_
#define TIMING_H_

#include <iostream>
#include <time.h>
#include <string>

#define _TIME \
	t_now = time_diff(status, t_now);\

double time_diff(std::string status, double start){
	double t_now = clock();
	//printf("%s : %f \n", s, seconds);
	std::cout << " Time: " << (t_now-start)/CLOCKS_PER_SEC << " : to pass " << status  << std::endl;
 
	return t_now;
}
#endif