#include "timing.h"
#include "client.h"

void main( int argc, char* argv[] ){

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    initConfig(argc, argv);
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    printf("\nWating 5 sec...\n");
    Sleep(5000);
    thread_ladybug_full(); 
	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
}


