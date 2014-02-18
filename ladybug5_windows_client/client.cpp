#include "timing.h"
#include "client.h"

void main( int argc, char* argv[] ){

    //openPgrFile("ladybug_13122828_20130821_170012-000000.pgr");
    //openPgrFile("ladybug_13122828_20130919_105903-000000.pgr");

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

   {
	    zmq::context_t context(1);
        boost::thread_group threads;

        thread_ladybug_full(&context);
        
        if(!cfg_threading || !cfg_postprocessing ){
            singleThread();
        }else             {
            threads.create_thread(std::bind(ladybugThread, &context, "inproc://uncompressed"));
            for(unsigned int i=0; i< cfg_compression_threads; ++i){
        	    threads.create_thread(std::bind(compresseionThread, &context, i)); //worker thread (jpg-compression)
        }
            threads.create_thread(std::bind(sendingThread, &context));
        }
        
	
	    while(true){
		    Sleep(1000);
	    }
	    std::printf("<PRESS ANY KEY TO EXIT>");
	    _getch();
    }
}

