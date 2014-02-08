#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <iostream>
/* Boost */
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/assign.hpp>
/* Ladybug */
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>
#define __WINDOWS__ true

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
extern LadybugAutoShutterRange cfg_ladybug_autoShutterRange;
extern LadybugAutoExposureMode cfg_ladybug_autoExposureMode;

/* Settings paths */
extern const char* PATH_ROS_MASTER;
extern const char* PATH_THREADING;
extern const char* PATH_NR_THREADS; 
extern const char* PATH_BATCH_THREAD;
extern const char* PATH_POST_PROCESS;      
extern const char* PATH_LADYBUG;
extern const char* PATH_PANO;  
extern const char* PATH_PANO_WIDTH;
extern const char* PATH_PANO_HIGHT;
extern const char* PATH_COLOR_PROCESSING;
extern const char* PATH_LB_DATA;
extern const char* PATH_EXPOSURE;
extern const char* PATH_SHUTTER;

/* typedefs for enum to string */
typedef boost::bimap< LadybugDataFormat, std::string > ldf_type;
typedef boost::bimap< LadybugColorProcessingMethod, std::string > lcpm_type;
typedef boost::bimap< LadybugOutputImage, std::string > loi_type;
typedef boost::bimap< LadybugAutoShutterRange, std::string > lsr_type;
typedef boost::bimap< LadybugAutoExposureMode, std::string > lex_type;

/*Config*/
void printTree (boost::property_tree::ptree &pt, int level);
void createDefaultIni(boost::property_tree::ptree *pt);
void loadConfigsFromPtree(boost::property_tree::ptree *pt);
void createOptionsFile();
extern const ldf_type ladybugDataFormatMap;
extern const lcpm_type ladybugColorProcessingMap;
extern const lsr_type ladybugAutoShutterRangeMap;
extern const lex_type ladybugAutoExposureModeMap;
