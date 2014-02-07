#pragma once
#include "timing.h"
#include "ladybug5_windows_client.h"

std::string cfg_ros_master = "tcp://192.168.1.178:28882";
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

typedef boost::bimap< LadybugDataFormat, std::string > ldf_type;
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
//
//typedef boost::bimap< LadybugColorProcessingMethod, std::string > lcpm_type;
//const lcpm_type ladybugColorProcessingMap =
//    boost::assign::list_of< lcpm_type::relation >
//    ( LADYBUG_DISABLE, "DISABLE" )
//    ( LADYBUG_EDGE_SENSING, "EDGE_SENSING" )
//    ( LADYBUG_NEAREST_NEIGHBOR_FAST, "NEAREST_NEIGHBOR_FAST" )
//    ( LADYBUG_RIGOROUS, "RIGOROUS" )
//    ( LADYBUG_DOWNSAMPLE4, "DOWNSAMPLE4" )
//    ( LADYBUG_DOWNSAMPLE16, "DOWNSAMPLE16" )
//    ( LADYBUG_MONO, "MONO" )
//    ( LADYBUG_HQLINEAR, "HQLINEAR" )
//    ( LADYBUG_HQLINEAR_GPU, "HQLINEAR_GPU" )
//    ( LADYBUG_DIRECTIONAL_FILTER, "DIRECTIONAL_FILTER" )
//    ( LADYBUG_COLOR_FORCE_QUADLET, "COLOR_FORCE_QUADLET" )
//    ( LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12, "DATAFORMAT_HALF_HEIGHT_RAW12" );

void createDefaultIni(boost::property_tree::ptree *pt){
    pt->put("Network.ROS_MASTER", cfg_ros_master.c_str());  
    pt->put("Threading.Enabled", cfg_threading);
    pt->put("Threading.NumberCompressionThreads", cfg_compression_threads); 
    pt->put("Threading.OneThreadPerImageGrab", cfg_full_img_msg);
    pt->put("Settings.PostProcessing", cfg_postprocessing);      
    pt->put("Ladybug.Simulate", cfg_simulation);
    pt->put("Ladybug.Panoramic_Only", cfg_panoramic);  
    pt->put("Ladybug.PanoWidth", cfg_pano_width);
    pt->put("Ladybug.PanoHight", cfg_pano_hight);
  //  pt->put("Ladybug.ColorProcessing", ladybugColorProcessingMap.left.find(cfg_ladybug_colorProcessing)->second.c_str());
    pt->put("Ladybug.Dataformat", ladybugDataFormatMap.left.find(cfg_ladybug_dataformat)->second.c_str());
    boost::property_tree::ini_parser::write_ini(cfg_configFile.c_str(), *pt);
}

void loadConfigsFromPtree(boost::property_tree::ptree *pt){
    cfg_ros_master = pt->get<std::string>("Network.ROS_MASTER");
    cfg_threading = pt->get<bool>("Threading.Enabled");
    cfg_compression_threads = pt->get<unsigned int>("Threading.NumberCompressionThreads");
    cfg_full_img_msg = pt->get<bool>("Threading.OneThreadPerImageGrab");
    cfg_postprocessing = pt->get<bool>("Settings.PostProcessing");
    cfg_simulation = pt->get<bool>("Ladybug.Simulate");
    cfg_panoramic = pt->get<bool>("Ladybug.Panoramic_Only");
    cfg_pano_width = pt->get<int>("Ladybug.PanoWidth");
    cfg_pano_hight = pt->get<unsigned int>("Ladybug.PanoHight");
  //  cfg_ladybug_colorProcessing = ladybugColorProcessingMap.right.find( pt->get<std::string>("Ladybug.ColorProcessing"))->second;    
    cfg_ladybug_dataformat = ladybugDataFormatMap.right.find( pt->get<std::string>("Ladybug.Dataformat"))->second;
}

void main( int argc, char* argv[] ){
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    printf("Start ladybug5_windows_client\n");
    setlocale(LC_ALL, ".OCP");

    boost::property_tree::ptree pt;
    if(argc > 1){ // load different config files over commandline
        printf("\n\nLoading configfile: %s\n\n",argv[1]);
        boost::property_tree::ini_parser::read_ini(argv[1], pt);
        loadConfigsFromPtree(&pt);
    }
    else{
        try{
            boost::property_tree::ini_parser::read_ini(cfg_configFile.c_str(), pt);
            loadConfigsFromPtree(&pt);

        }catch (std::exception){ /* if we create a new config with the default values */
            printf("Loading %s failed\nTrying to fix it\n", cfg_configFile.c_str());
            createDefaultIni(&pt);
            loadConfigsFromPtree(&pt);
        }
    }
    printTree(pt, 0);
    printf("\nWating 3 sec...\n");
    Sleep(3000);

    if(!cfg_threading){
        singleThread();
    }
    else{
	    zmq::context_t context(1);

	    boost::thread_group threads;
        if(cfg_simulation){
            threads.create_thread(std::bind(ladybugSimulator, &context)); //image grabbing thread
        }
        else{
            threads.create_thread(std::bind(ladybugThread, &context, "inproc://uncompressed")); // ladybug thread
        }
	
        for(int i=0; i< cfg_compression_threads; ++i){
		    threads.create_thread(std::bind(compresseionThread, &context, i)); //worker thread (jpg-compression)
	    }

	    while(true){
		    Sleep(1000);
	    }
	    std::printf("<PRESS ANY KEY TO EXIT>");
	    _getch();
    }
}

