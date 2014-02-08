#include "configuration_helper.h"

/* Settings paths */
const char* PATH_ROS_MASTER =  "Network.ROS_MASTER";
const char* PATH_THREADING  =  "Threading.Enabled";
const char* PATH_NR_THREADS =  "Threading.NumberCompressionThreads"; 
const char* PATH_BATCH_THREAD ="Threading.OneThreadPerImageGrab";
const char* PATH_POST_PROCESS=  "Settings.PostProcessing";      
const char* PATH_LADYBUG    =   "Ladybug.Simulate";
const char* PATH_PANO       =   "Ladybug.Panoramic_Only";  
const char* PATH_PANO_WIDTH =   "Ladybug.PanoWidth";
const char* PATH_PANO_HIGHT =   "Ladybug.PanoHight";
const char* PATH_COLOR_PROCESSING = "Ladybug.ColorProcessing";
const char* PATH_LB_DATA    =   "Ladybug.Dataformat";
const char* PATH_EXPOSURE   =   "Ladybug.ExposureMode";
const char* PATH_SHUTTER    =   "Ladybug.ShutterRange";

std::string cfg_ros_master = "tcp://10.1.1.1:28882";
std::string cfg_configFile = "config.ini";
bool cfg_threading = true;
bool cfg_panoramic = false;
bool cfg_simulation = false;
bool cfg_postprocessing = false;
bool cfg_full_img_msg = true;
unsigned int cfg_compression_threads = 4; 
/* The size of the stitched image */
unsigned int cfg_pano_width = 4096;
unsigned int cfg_pano_hight = 2048;
LadybugDataFormat cfg_ladybug_dataformat = LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8;
LadybugColorProcessingMethod cfg_ladybug_colorProcessing = LADYBUG_DOWNSAMPLE4;//LADYBUG_NEAREST_NEIGHBOR_FAST; //LADYBUG_DOWNSAMPLE4;
LadybugAutoShutterRange cfg_ladybug_autoShutterRange = LADYBUG_AUTO_SHUTTER_MOTION;
LadybugAutoExposureMode cfg_ladybug_autoExposureMode = LADYBUG_AUTO_EXPOSURE_ROI_FULL_IMAGE ;

std::string indent(int level) {
  std::string s; 
  for (int i=0; i<level; i++) s += "  ";
  return s; 
} 

void printTree (boost::property_tree::ptree &pt, int level) {
  if (pt.empty()) {
    std::cout << "\""<< pt.data()<< "\"";
  } else {
    if (level) std::cout << std::endl; 
    std::cout << indent(level) << "{" << std::endl;     
    for (boost::property_tree::ptree::iterator pos = pt.begin(); pos != pt.end();) {
      std::cout << indent(level+1) << "\"" << pos->first << "\": "; 
      printTree(pos->second, level + 1); 
      ++pos; 
      if (pos != pt.end()) {
        std::cout << ","; 
      }
      std::cout << std::endl;
    } 
    std::cout << indent(level) << " }";     
  } 
  return; 
}

void createDefaultIni(boost::property_tree::ptree *pt){
    pt->put(PATH_ROS_MASTER, cfg_ros_master.c_str());  
    pt->put(PATH_THREADING, cfg_threading);
    pt->put(PATH_NR_THREADS, cfg_compression_threads); 
    pt->put(PATH_BATCH_THREAD, cfg_full_img_msg);
    pt->put(PATH_POST_PROCESS, cfg_postprocessing);      
    pt->put(PATH_LADYBUG, cfg_simulation);
    pt->put(PATH_PANO, cfg_panoramic);  
    pt->put(PATH_PANO_WIDTH, cfg_pano_width);
    pt->put(PATH_PANO_HIGHT, cfg_pano_hight);
    pt->put(PATH_COLOR_PROCESSING, ladybugColorProcessingMap.left.find(cfg_ladybug_colorProcessing)->second.c_str());
    pt->put(PATH_LB_DATA, ladybugDataFormatMap.left.find(cfg_ladybug_dataformat)->second.c_str());
    pt->put(PATH_EXPOSURE, ladybugAutoExposureModeMap.left.find(cfg_ladybug_autoExposureMode)->second.c_str());
    pt->put(PATH_SHUTTER, ladybugAutoShutterRangeMap.left.find(cfg_ladybug_autoShutterRange)->second.c_str());
    boost::property_tree::ini_parser::write_ini(cfg_configFile.c_str(), *pt);
}

void loadConfigsFromPtree(boost::property_tree::ptree *pt){
    cfg_ros_master = pt->get<std::string>(PATH_ROS_MASTER);
    cfg_threading = pt->get<bool>(PATH_THREADING);
    cfg_compression_threads = pt->get<unsigned int>(PATH_NR_THREADS);
    cfg_full_img_msg = pt->get<bool>(PATH_BATCH_THREAD);
    cfg_postprocessing = pt->get<bool>(PATH_POST_PROCESS);
    cfg_simulation = pt->get<bool>(PATH_LADYBUG);
    cfg_panoramic = pt->get<bool>(PATH_PANO);
    cfg_pano_width = pt->get<int>(PATH_PANO_WIDTH);
    cfg_pano_hight = pt->get<unsigned int>(PATH_PANO_HIGHT);
    cfg_ladybug_colorProcessing = ladybugColorProcessingMap.right.find( pt->get<std::string>(PATH_COLOR_PROCESSING))->second;    
    cfg_ladybug_dataformat = ladybugDataFormatMap.right.find( pt->get<std::string>(PATH_LB_DATA))->second;
    cfg_ladybug_autoExposureMode = ladybugAutoExposureModeMap.right.find( pt->get<std::string>(PATH_EXPOSURE))->second;
    cfg_ladybug_autoShutterRange = ladybugAutoShutterRangeMap.right.find( pt->get<std::string>(PATH_SHUTTER))->second;
}

const ldf_type ladybugDataFormatMap =
  boost::assign::list_of< ldf_type::relation >
    ( LADYBUG_DATAFORMAT_RAW8, "RAW8" )
    ( LADYBUG_DATAFORMAT_JPEG8, "JPEG8" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_RAW8, "COLOR_SEP_RAW8" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8, "COLOR_SEP_JPEG8" )
    ( LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8, "HALF_HEIGHT_RAW8" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_RAW8, "COLOR_SEP_RAW8" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8, "COLOR_SEP_HALF_HEIGHT_JPEG8" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12, "COLOR_SEP_JPEG12" )
    ( LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16, "HALF_HEIGHT_RAW16" )
    ( LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12, "COLOR_SEP_HALF_HEIGHT_JPEG12" )
    ( LADYBUG_DATAFORMAT_RAW12, "RAW12" )
    ( LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12, "HALF_HEIGHT_RAW12" );

const lcpm_type ladybugColorProcessingMap =
    boost::assign::list_of< lcpm_type::relation >
    ( LADYBUG_DISABLE, "DISABLE" )
    ( LADYBUG_EDGE_SENSING, "EDGE_SENSING" )
    ( LADYBUG_NEAREST_NEIGHBOR_FAST, "NEAREST_NEIGHBOR_FAST" )
    ( LADYBUG_RIGOROUS, "RIGOROUS" )
    ( LADYBUG_DOWNSAMPLE4, "DOWNSAMPLE4" )
    ( LADYBUG_DOWNSAMPLE16, "DOWNSAMPLE16" )
    ( LADYBUG_MONO, "MONO" )
    ( LADYBUG_HQLINEAR, "HQLINEAR" )
    ( LADYBUG_HQLINEAR_GPU, "HQLINEAR_GPU" )
    ( LADYBUG_DIRECTIONAL_FILTER, "DIRECTIONAL_FILTER" )
    ( LADYBUG_COLOR_FORCE_QUADLET, "COLOR_FORCE_QUADLET" );

const loi_type ladybugOutputImageMap = 
    boost::assign::list_of< loi_type::relation >
    ( LADYBUG_RAW_CAM0, "RAW_CAM0" )
    ( LADYBUG_RAW_CAM1, "RAW_CAM1" )
    ( LADYBUG_RAW_CAM2, "RAW_CAM2" )
    ( LADYBUG_RAW_CAM3, "RAW_CAM3" )
    ( LADYBUG_RAW_CAM4, "RAW_CAM4" )
    ( LADYBUG_RAW_CAM5, "RAW_CAM5" )
    ( LADYBUG_ALL_RAW_IMAGES, "ALL_RAW_IMAGES" )
    ( LADYBUG_RECTIFIED_CAM0, "RECTIFIED_CAM0" )
    ( LADYBUG_RECTIFIED_CAM1, "RECTIFIED_CAM1" )
    ( LADYBUG_RECTIFIED_CAM2, "RECTIFIED_CAM2" )
    ( LADYBUG_RECTIFIED_CAM3, "RECTIFIED_CAM3" )
    ( LADYBUG_RECTIFIED_CAM4, "RECTIFIED_CAM4" )
    ( LADYBUG_RECTIFIED_CAM5, "RECTIFIED_CAM5" )
    ( LADYBUG_ALL_RECTIFIED_IMAGES, "ALL_RECTIFIED_IMAGES" )
    ( LADYBUG_PANORAMIC, "PANORAMIC" )
    ( LADYBUG_DOME, "DOME" )
    ( LADYBUG_SPHERICAL, "SPHERICAL" )
    ( LADYBUG_ALL_CAMERAS_VIEW, "ALL_CAMERAS_VIEW" )
    ( LADYBUG_ALL_OUTPUT_IMAGE, "ALL_OUTPUT_IMAGE" );

const lsr_type ladybugAutoShutterRangeMap = 
    boost::assign::list_of< lsr_type::relation >
    ( LADYBUG_AUTO_SHUTTER_MOTION, "MOTION" )
    ( LADYBUG_AUTO_SHUTTER_INDOOR, "INDOOR" )
    ( LADYBUG_AUTO_SHUTTER_LOW_NOISE, "LOW_NOISE" )
    ( LADYBUG_AUTO_SHUTTER_CUSTOM, "CUSTOM" )
    ( LADYBUG_AUTO_SHUTTER_FORCE_QUADLET, "FORCE_QUADLET" );

const lex_type ladybugAutoExposureModeMap = 
    boost::assign::list_of< lex_type::relation >
    ( LADYBUG_AUTO_EXPOSURE_ROI_FULL_IMAGE, "FULL_IMAGE" )
    ( LADYBUG_AUTO_EXPOSURE_ROI_BOTTOM_50, "BOTTOM_50" )
    ( LADYBUG_AUTO_EXPOSURE_ROI_TOP_50, "TOP_50" )
    ( LADYBUG_AUTO_EXPOSURE_ROI_FORCE_QUADLET, "FORCE_QUADLET" );

template< class MapType >
void print_map(const MapType & map,
               const std::string & separator,
               std::ofstream & os )
{
    typedef typename MapType::const_iterator const_iterator;

    for( const_iterator i = map.begin(), iend = map.end(); i != iend; ++i )
    {
        //os << i->first << separator << i->second << std::endl;
        os << i->second << "\n";
    }
}

void createOptionsFile(){
    const char* filename = "config.ini.options.txt";
    const char* lb = "\n";
    const char* limitter = "-------------------------------------------";

    std::ofstream file(filename);
    if(file.is_open() ) //&& tellg() == 0)
    {
        std::cout << "Creating " << filename << lb;
        /* Exposure options */
        file << "Options" << lb;
        file << limitter << lb;
        file << PATH_EXPOSURE << lb;
        print_map( ladybugAutoExposureModeMap.left, " - ", file);
           
        /* Ladybug data options */
        file << limitter << lb;
        file << PATH_LB_DATA << lb;
        print_map( ladybugDataFormatMap.left, " - ", file);

        /* Color Processing options */
        file << limitter << lb;
        file << PATH_COLOR_PROCESSING << lb;
        print_map( ladybugColorProcessingMap.left, " - ", file);

        /* Shutter options */
        file << limitter << lb;
        file << PATH_SHUTTER << lb;
        print_map( ladybugAutoShutterRangeMap.left, " - ", file);
        file.flush();
        file.close();
    }
    else
    {
        printf("Creating the option file failed\n");
    }
}