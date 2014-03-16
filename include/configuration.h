#pragma once
#include <string>
/* Boost */
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
/* Ladybug */
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>

#include "configuration_helper.h"

class Configuration{
public:
    std::string cfg_ros_master;
    std::string cfg_configFile;
    std::string cfg_fileStream;
    bool cfg_panoramic;
    bool cfg_transfer_compressed;
    bool cfg_rectification;
    /* The size of the stitched image */
    unsigned int cfg_pano_width;
    unsigned int cfg_pano_hight;
    LadybugDataFormat cfg_ladybug_dataformat;
    LadybugColorProcessingMethod cfg_ladybug_colorProcessing;
    LadybugAutoShutterRange cfg_ladybug_autoShutterRange;
    LadybugAutoExposureMode cfg_ladybug_autoExposureMode;

    Configuration(std::string filename="config.ini");
    void load(std::string filename="config.ini");
    void save(std::string filename="config.ini");
    void saveOptions(std::string filename="config.ini.options.txt");
    ~Configuration();
private:
    void init();
    boost::property_tree::ptree pt;
};