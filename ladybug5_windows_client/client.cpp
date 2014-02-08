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
    printf("\nWating 10 sec...");
    Sleep(10000);

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

