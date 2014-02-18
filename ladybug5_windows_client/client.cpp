#include "timing.h"
#include "client.h"

void main( int argc, char* argv[] ){

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    printf("Start %s\n", argv[0]);
    createOptionsFile();
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
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    printf("\nWating 5 sec...\n");
    Sleep(5000);
    thread_ladybug_full(); 
	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
}


