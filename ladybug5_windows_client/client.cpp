#include "timing.h"
#include "client.h"

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
LadybugAutoShutterRange cfg_ladybug_autoShutterRange = LADYBUG_AUTO_SHUTTER_MOTION;
LadybugAutoExposureMode cfg_ladybug_autoExposureMode = LADYBUG_AUTO_EXPOSURE_ROI_FULL_IMAGE ;

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
	
        for(unsigned int i=0; i< cfg_compression_threads; ++i){
		    threads.create_thread(std::bind(compresseionThread, &context, i)); //worker thread (jpg-compression)
	    }

	    while(true){
		    Sleep(1000);
	    }
	    std::printf("<PRESS ANY KEY TO EXIT>");
	    _getch();
    }
}

