#pragma once
#include "timing.h"
#include "ladybug5_windows_client.h"

std::string cfg_ros_master = "tcp://192.168.1.178:28882";
std::string cfg_configFile = "config.ini";
bool cfg_threading = true;
bool cfg_panoramic = false;
bool cfg_simulation = true;
bool cfg_postprocessing = false;
bool cfg_full_img_msg = true;
unsigned int cfg_compression_threads = 4; 
/* The size of the stitched image */
unsigned int cfg_pano_width = 4096;
unsigned int cfg_pano_hight = 2048;
LadybugDataFormat cfg_ladybug_dataformat = LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8;
LadybugColorProcessingMethod cfg_ladybug_colorProcessing = LADYBUG_DOWNSAMPLE4;

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
    ( LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8, "RAW8" )
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
    try{
        boost::property_tree::ini_parser::read_ini(cfg_configFile.c_str(), pt);
        loadConfigsFromPtree(&pt);

    }catch (std::exception){ /* if we create a new config with the default values */
        printf("Loading %s failed\nTrying to fix it\n", cfg_configFile.c_str());
        createDefaultIni(&pt);
        loadConfigsFromPtree(&pt);
    }
   printf("Send-%i\n", cfg_full_img_msg);
    printTree(pt, 0);
    printf("\n");

    if(!cfg_threading){
        singleThread();
    }
    else{
	    zmq::context_t context(1);
	    zmq::socket_t grabber(context, ZMQ_ROUTER);
        //zmq_bind (grabber, "inproc://uncompressed");

        //  Socket to send messages to
        zmq::socket_t workers(context, ZMQ_DEALER);
        //zmq_bind (workers, "inproc://workers");
	
	    //zmq::socket_t grabber(context, ZMQ_ROUTER);
	    //unsigned int val = 12; //buffer size
	    //grabber.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	    //grabber.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled
	    //grabber.bind("inproc://uncompressed");
     //
	    //zmq::socket_t workers(context, ZMQ_DEALER);
	    //workers.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	    //grabber.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled
	    //zmq_bind(workers, "inproc://workers");
	

    //  Start a queue device zmq_device (ZMQ_QUEUE, frontend, backend)
	    /*zmq::socket_t ladybugSocket (*context, ZMQ_ROUTER);
        ladybugSocket.bind("inproc://uncompressed");

        zmq::socket_t workers (*context, ZMQ_DEALER);
        workers.bind("inproc://worker");*/

	    //boost::thread grab(ladybugThread, &context, "uncompressed");
	
	    //zmq::proxy(ladybugSocket, workers, NULL);
	  

	    boost::thread_group threads;
	    threads.create_thread(std::bind(ladybugSimulator, &context)); //ladybug grabbing thread
	    //threads.create_thread(std::bind(sendingThread, &context)); // sending thread
	
        for(int i=0; i< cfg_compression_threads; ++i){
		    threads.create_thread(std::bind(compresseionThread, &context, i)); //worker thread (jpg-compression)
	    }
	
	    //zmq_proxy(grabber, workers, NULL); 

        // Launch pool of worker threads
	    while(true){
		    Sleep(1000);
	    }
	    std::printf("<PRESS ANY KEY TO EXIT>");
	    _getch();
    }
}

int singleThread()
{
_RESTART:
	
	double t_now = clock();	
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;

	std::string status = "Creating zmq context and connect";
	zmq::context_t zmq_context(1);

	zmq::socket_t socket (zmq_context, ZMQ_PUB);
	int val = 6; //buffer size
	socket.setsockopt(ZMQ_RCVHWM, &val, sizeof(val));  //prevent buffer get overfilled
	socket.setsockopt(ZMQ_SNDHWM, &val, sizeof(val));  //prevent buffer get overfilled

    printf("Sending images to %s\n", cfg_ros_master.c_str());
	socket.connect (cfg_ros_master.c_str());
	_TIME
	
	unsigned char* arpBuffers[ LADYBUG_NUM_CAMERAS ];
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = NULL;
	}
	status = "Create Ladybug context";
	LadybugError error;

	int retry = 10;

	// create ladybug context
	LadybugContext context = NULL;
	error = ladybugCreateContext( &context );
	if ( error != LADYBUG_OK )
	{
		printf( "Failed creating ladybug context. Exit.\n" );
		return (1);
	}
	_TIME

	status = "Initialize the camera";

	// Initialize  the camera
	error = initCamera(context);
	_HANDLE_ERROR
	_TIME	

	if(cfg_panoramic){
		status = "configure for panoramic stitching";
		error = configureLadybugForPanoramic(context);
		_HANDLE_ERROR
		_TIME
	}else{
		printf("Skipped configuration for panoramic pictures\n"); 
	}

	status = "start ladybug";
	error = startLadybug(context);
	_HANDLE_ERROR
	_TIME
	
	// Grab an image to inspect the image size
	status = "Grab an image to inspect the image size";
	error = LADYBUG_FAILED;
	LadybugImage image;
	while ( error != LADYBUG_OK && retry-- > 0)
	{
		error = ladybugGrabImage( context, &image ); 
	}
	_HANDLE_ERROR
	_TIME

	status = "inspect image size";
	// Set the size of the image to be processed
    if (cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
		cfg_ladybug_colorProcessing == LADYBUG_MONO)
	{
		uiRawCols = image.uiCols / 2;
		uiRawRows = image.uiRows / 2;
	}
	else
	{
		uiRawCols = image.uiCols;
		uiRawRows = image.uiRows;
	}
	_TIME

	// Initialize alpha mask size - this can take a long time if the
	// masks are not present in the current directory.
	status = "Initializing alpha masks (this may take some time)...";
	error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
	_HANDLE_ERROR
	_TIME

	initBuffers(arpBuffers, LADYBUG_NUM_CAMERAS, uiRawCols, uiRawRows, 4);

	while(true)
	{
		try{
			double loopstart = t_now = clock();	
			double t_now = loopstart;	
			ladybug5_network::pbMessage message = createMessage("windows","ladybug5");

			printf("\nGrab Image loop...\n");
			// Grab an image from the camera
			std::string status = "grab image";
			LadybugImage image;
			error = ladybugGrabImage(context, &image); 
			_HANDLE_ERROR
			_TIME

			ladybug5_network::LadybugTimeStamp msg_timestamp;
			msg_timestamp.set_ulcyclecount(image.timeStamp.ulCycleCount);
			msg_timestamp.set_ulcycleoffset(image.timeStamp.ulCycleOffset);
			msg_timestamp.set_ulcycleseconds(image.timeStamp.ulCycleSeconds);
			msg_timestamp.set_ulmicroseconds(image.timeStamp.ulMicroSeconds);
			msg_timestamp.set_ulseconds(image.timeStamp.ulSeconds);

			status = "Convert images to 6 RGB buffers";
			// Convert the image to 6 RGB buffers
			error = ladybugConvertImage(context, &image, arpBuffers);
			_HANDLE_ERROR
			_TIME

			if(cfg_panoramic){
				status = "Send RGB buffers to graphics card";
				// Send the RGB buffers to the graphics card
				error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
				_HANDLE_ERROR
				_TIME

				status = "create panorame in graphics card";
				// Stitch the images (inside the graphics card) and retrieve the output to the user's memory
				LadybugProcessedImage processedImage;
				error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
				_HANDLE_ERROR
				_TIME
			
				status = "Add image to message";
				addImageToMessage(&message, processedImage.pData, TJPF_BGR, &msg_timestamp, ladybug5_network::LADYBUG_PANORAMIC, processedImage.uiCols, processedImage.uiRows );
				_TIME
			}
			else{
				status = "Adding images without processing";
				for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
				{
					addImageToMessage(&message, arpBuffers[uiCamera], TJPF_BGRA, &msg_timestamp, (ladybug5_network::ImageType) ( 1 << uiCamera), uiRawCols, uiRawRows );
				}
				_TIME
			}
			printf("Sending...\n");
			status = "send img over network";
            pb_send(&socket, &message);
			_TIME

			status = "Sum loop";
			t_now = loopstart;
			_TIME
		}
		
		catch(std::exception e){
			printf("Exception, trying to recover\n");
			goto _EXIT; 
		};
	}
	
	printf("Done.\n");
_EXIT:
	//
	// clean up
	//
	ladybugStop( context );
	ladybugDestroyContext( &context );

	google::protobuf::ShutdownProtobufLibrary();
	socket.close();
	
	for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ ){
		if ( arpBuffers[ uiCamera ] != NULL )
		{
			delete  [] arpBuffers[ uiCamera ];
		}
	}
	goto _RESTART;

	return 1;
}
