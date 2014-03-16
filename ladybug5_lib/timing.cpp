#include "timing.h"

double time_diff(std::string status, double start, std::string name){
	double t_now = clock();
	//printf("%s : %f \n", s, seconds);
#ifdef _DEBUG
    printf("%f\t %s\t to pass %s\n", (t_now-start)/CLOCKS_PER_SEC,  name.c_str(), status.c_str());
#endif // !debug

  
	return t_now;
}