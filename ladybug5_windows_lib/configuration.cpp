#include "include\configuration.h"

Configuration::Configuration(std::string filename){
    init();
    cfg_configFile = filename;
    try{
        load(filename);
    }catch(std::exception){
        save(filename);
    }
}

void 
Configuration::init(){
        cfg_ros_master = "tcp://10.1.1.1:28882";
        cfg_configFile = "config.ini";
        cfg_fileStream = "";
        cfg_panoramic = false;
        cfg_transfer_compressed = true;
        cfg_rectification = false;
        /* The size of the stitched image */
        cfg_pano_width = 4096;
        cfg_pano_hight = 2048;
        cfg_ladybug_dataformat = LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8;
        cfg_ladybug_colorProcessing = LADYBUG_DOWNSAMPLE4;//LADYBUG_NEAREST_NEIGHBOR_FAST; //LADYBUG_DOWNSAMPLE4;
        cfg_ladybug_autoShutterRange = LADYBUG_AUTO_SHUTTER_MOTION;
        cfg_ladybug_autoExposureMode = LADYBUG_AUTO_EXPOSURE_ROI_FULL_IMAGE ; 
}

void 
Configuration::load(std::string filename){
    boost::property_tree::ini_parser::read_ini(filename, pt);
    cfg_ros_master = pt.get<std::string>(PATH_ROS_MASTER);
    cfg_transfer_compressed = pt.get<bool>(PATH_TRANSFER_COMPRESSED);
    cfg_fileStream = pt.get<std::string>(PATH_LADYBUG_STREAMFILE);
    cfg_rectification = pt.get<bool>(PATH_RECTIFICATION);
    cfg_panoramic = pt.get<bool>(PATH_PANO);
    cfg_pano_width = pt.get<int>(PATH_PANO_WIDTH);
    cfg_pano_hight = pt.get<unsigned int>(PATH_PANO_HIGHT);
    cfg_ladybug_colorProcessing = ladybugColorProcessingMap.right.find( pt.get<std::string>(PATH_COLOR_PROCESSING))->second;    
    cfg_ladybug_dataformat = ladybugDataFormatMap.right.find( pt.get<std::string>(PATH_LB_DATA))->second;
    cfg_ladybug_autoExposureMode = ladybugAutoExposureModeMap.right.find( pt.get<std::string>(PATH_EXPOSURE))->second;
    cfg_ladybug_autoShutterRange = ladybugAutoShutterRangeMap.right.find( pt.get<std::string>(PATH_SHUTTER))->second;
}

void
Configuration::save(std::string filename){
    pt.put(PATH_ROS_MASTER, cfg_ros_master.c_str()); 
    pt.put(PATH_TRANSFER_COMPRESSED, cfg_transfer_compressed);
    pt.put(PATH_LADYBUG_STREAMFILE, cfg_fileStream.c_str());
    pt.put(PATH_RECTIFICATION, cfg_rectification);
    pt.put(PATH_PANO, cfg_panoramic);  
    pt.put(PATH_PANO_WIDTH, cfg_pano_width);
    pt.put(PATH_PANO_HIGHT, cfg_pano_hight);
    pt.put(PATH_COLOR_PROCESSING, ladybugColorProcessingMap.left.find(cfg_ladybug_colorProcessing)->second.c_str());
    pt.put(PATH_LB_DATA, ladybugDataFormatMap.left.find(cfg_ladybug_dataformat)->second.c_str());
    pt.put(PATH_EXPOSURE, ladybugAutoExposureModeMap.left.find(cfg_ladybug_autoExposureMode)->second.c_str());
    pt.put(PATH_SHUTTER, ladybugAutoShutterRangeMap.left.find(cfg_ladybug_autoShutterRange)->second.c_str());
    boost::property_tree::ini_parser::write_ini(cfg_configFile.c_str(), pt);
}

Configuration::~Configuration(){
}