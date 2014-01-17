#include "ladybug5_windows_client.h"

// The size of the stitched image
#define PANORAMIC_IMAGE_HEIGHT   4000
#define PANORAMIC_IMAGE_WIDTH    8000

#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
//#define COLOR_PROCESSING_METHOD   LADYBUG_HQLINEAR          // High quality method suitable for large stitched images

//
// This function reads an image from camera
//
LadybugError
	initCamera( LadybugContext context)
{    
	// Initialize camera
	printf( "Initializing camera...\n" );
	LadybugError error = LADYBUG_NOT_STARTED;
	
	error = ladybugInitializeFromIndex( context, 0 );
	int i = 0;
	while(error != LADYBUG_OK && i<10){
		++i;
		printf( "Initializing camera...try %i\n", i );
		Sleep(2500);
		error = ladybugInitializeFromIndex( context, 0 );
	}
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	// Load config file
	printf( "Load configuration file...\n" );
	error = ladybugLoadConfig( context, NULL);
	i = 0;
	while(error != LADYBUG_OK && i<10){
		++i;
		printf( "Load configuration file...try %i\n", i );
		Sleep(500);
		error = ladybugLoadConfig( context, NULL);
	}
	if (error != LADYBUG_OK)
	{
		return error;    
	}
	return error;
}

LadybugError configureLadybugForPanoramic(LadybugContext context){
	
	// Set the panoramic view angle
	LadybugError error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	// Make the rendering engine use the alpha mask
	error = ladybugSetAlphaMasking( context, true );
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	// Set color processing method.
	error = ladybugSetColorProcessingMethod( context, COLOR_PROCESSING_METHOD );
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	// Configure output images in Ladybug library
	printf( "Configure output images in Ladybug library...\n" );
	error = ladybugConfigureOutputImages( context, LADYBUG_PANORAMIC );
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	error = ladybugSetOffScreenImageSize(
		context, 
		LADYBUG_PANORAMIC,  
		PANORAMIC_IMAGE_WIDTH, 
		PANORAMIC_IMAGE_HEIGHT );  
	if (error != LADYBUG_OK)
	{
		return error;    
	}
	return error;
}

LadybugError startLadybug(LadybugContext context){
	// Get camera info
	printf( "Getting camera info...\n" );
	LadybugCameraInfo caminfo;
	LadybugError error = ladybugGetCameraInfo( context, &caminfo );
	if (error != LADYBUG_OK)
	{
		return error;    
	}
	printf("Starting %s (%u)...\n", caminfo.pszModelName, caminfo.serialHead);

	switch( caminfo.deviceType )
	{
	case LADYBUG_DEVICE_COMPRESSOR:
	case LADYBUG_DEVICE_LADYBUG3:
	case LADYBUG_DEVICE_LADYBUG5:
		printf( "Starting Ladybug camera...\n" );
		error = ladybugStart(
			context,
			//LADYBUG_DATAFORMAT_JPEG8 
			//LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8
			LADYBUG_DATAFORMAT_RAW8 
			);
		break;

	case LADYBUG_DEVICE_LADYBUG:
	default:
		printf( "Unsupported device.\n");
		error = LADYBUG_FAILED;
	}

	return error;
}

//=============================================================================
// Main Routine
//=============================================================================
int 
	main( int argc, char* argv[] )
{

	printf("Start ladybug5_windows_client.exe\nPano w: %i h: %i\n\n", PANORAMIC_IMAGE_WIDTH, 
		PANORAMIC_IMAGE_HEIGHT);
_RESTART:
	double t_now = clock();	
	std::string status = "Creating zmq context and connect";
	
	zmq::context_t zmq_context(1);
	zmq::socket_t socket (zmq_context, ZMQ_PUB);

	std::string connection = "tcp://149.201.37.83:28882";
	printf("Sending images to %s\n",connection.c_str());
	socket.connect (connection.c_str());

	/*while(true){
		socket_write( &socket, &message);
		socket_read(&socket);
		Sleep(1);
	}*/

	_TIME
	status = "Create Ladybug context";
	LadybugContext context = NULL;
	LadybugError error;
	unsigned int uiRawCols = 0;
	unsigned int uiRawRows = 0;
	int retry = 10;

	// create ladybug context
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

	status = "configure for panoramic stitching";
	error = configureLadybugForPanoramic(context);
	_HANDLE_ERROR
	_TIME

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
	if (COLOR_PROCESSING_METHOD == LADYBUG_DOWNSAMPLE4 || 
		COLOR_PROCESSING_METHOD == LADYBUG_MONO)
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

	while(true)
	{
		try{
			double t_now = clock();	
			LadybugError error;
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

			status = "Convert images ti 6 RGB buffers";
			// Convert the image to 6 RGB buffers
			error = ladybugConvertImage(context, &image, NULL);
			_HANDLE_ERROR
			_TIME

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
	
			
			ladybug5_network::ImageMessage message = createMessage();
			status = "Add image to message";
			addImageToMessage(&message, processedImage.pData, &msg_timestamp, ladybug5_network::LADYBUG_PANORAMIC, processedImage.uiCols, processedImage.uiRows );
			_TIME

			printf("Sending...\n");
			status = "send img over network";
			_TIME

			socket_write(&socket, &message);
			//status = "Sum sending image";
			/*message="read from socket";
			socket_read(&socket);
			_TIME*/
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

	goto _RESTART;

	printf("<PRESS ANY KEY TO EXIT>");
	_getch();
	return 0;
}

ladybug5_network::ImageMessage createMessage(){
	ladybug5_network::ImageMessage message;
	message.set_id(1);
	message.set_name("Windows_Client"); // hostname here
	return message;
}

void addImageToMessage(ladybug5_network::ImageMessage *message,  unsigned char* uncompressedBGRImageBuffer, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height){
	//encode to jpg
	const int COLOR_COMPONENTS = 3;
	unsigned long imgSize = 0;
	unsigned char* _compressedImage = 0;
	int JPEG_QUALITY = 85;

	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, uncompressedBGRImageBuffer, _width, 0, _height, TJPF_BGR,
				&_compressedImage, &imgSize, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);
	
	//append to message
	ladybug5_network::Image* image_msg = 0;
	image_msg = message->add_images();
	image_msg->set_image(_compressedImage, imgSize);
	image_msg->set_size(imgSize);
	image_msg->set_type(img_type);
	image_msg->set_allocated_time(new ladybug5_network::LadybugTimeStamp(*timestamp));
	tjFree(_compressedImage);
}

void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height ){
	const int COLOR_COMPONENTS = 3;
		
	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, srcBuffer, _width, 0, _height, TJPF_BGR,
				&_compressedImage, _jpegSize, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);	
}


LadybugError saveImages(LadybugContext context, int i){

	LadybugError error;
	for(int j=0; j<LADYBUG_NUM_CAMERAS+1; j++)
	{
		// Stitch the images (inside the graphics card) and retrieve the output to the user's memory
		LadybugProcessedImage processedImage;
		char pszOutputName[256];
		sprintf( pszOutputName, "%03d_cam%d.jpg",i ,j);
		switch (j)
		{
		case 0:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM0, LADYBUG_BGR, &processedImage);
			break;
		case 1:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM1, LADYBUG_BGR, &processedImage);
			break;
		case 2:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM2, LADYBUG_BGR, &processedImage);
			break;
		case 3:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM3, LADYBUG_BGR, &processedImage);
			break;
		case 4:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM4, LADYBUG_BGR, &processedImage);
			break;
		case 5:
			error = ladybugRenderOffScreenImage(context, LADYBUG_RECTIFIED_CAM5, LADYBUG_BGR, &processedImage);
			break;
		case LADYBUG_NUM_CAMERAS:
			error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
			sprintf( pszOutputName, "%03d_pano.jpg",i);
			break;
		}
		_HANDLE_ERROR

			printf( "Writing image %s...\n", pszOutputName);
		error = ladybugSaveImage( context, &processedImage, pszOutputName, LADYBUG_FILEFORMAT_JPG);
		_HANDLE_ERROR
	}


_EXIT:
	return error;
}
