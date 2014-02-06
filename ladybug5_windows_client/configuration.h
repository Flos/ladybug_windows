#pragma once
#define __WINDOWS__ true

//#define ROS_MASTER "tcp://149.201.37.83:28882"
static std::string cfg_ros_master = "tcp://192.168.1.178:28882";
static std::string cfg_configFile = "config.ini";
static bool cfg_threading = true;
static bool cfg_panoramic = false;
static bool cfg_simulation = true;
static bool cfg_postprocessing = false;
static unsigned int cfg_compression_threads = 4; 
/* The size of the stitched image */
static unsigned int cfg_pano_width = 4096;
static unsigned int cfg_pano_hight = 2048;

#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
//#define COLOR_PROCESSING_METHOD   LADYBUG_HQLINEAR          // High quality method suitable for large stitched images