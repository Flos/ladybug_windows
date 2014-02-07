#pragma once
#define __WINDOWS__ true

//#define ROS_MASTER "tcp://149.201.37.83:28882"
extern std::string cfg_ros_master;
extern std::string cfg_configFile;
extern bool cfg_threading;
extern bool cfg_panoramic;
extern bool cfg_simulation;
extern bool cfg_postprocessing;
extern bool cfg_full_img_msg;
extern unsigned int cfg_compression_threads; 
/* The size of the stitched image */
extern unsigned int cfg_pano_width;
extern unsigned int cfg_pano_hight;
extern LadybugDataFormat cfg_ladybug_dataformat;
extern LadybugColorProcessingMethod cfg_ladybug_colorProcessing;

