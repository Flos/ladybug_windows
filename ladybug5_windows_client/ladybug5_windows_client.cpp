//=============================================================================
// Copyright © 2004 Point Grey Research, Inc. All Rights Reserved.
// 
// This software is the confidential and proprietary information of Point
// Grey Research, Inc. ("Confidential Information").  You shall not
// disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with Point Grey Research, Inc. (PGR).
// 
// PGR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. PGR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================

//=============================================================================
//
// ladybugPanoStitchExample.cpp
// 
// This example shows users how to extract an image set from a Ladybug camera,
// stitch it together and write the final stitched image to disk.
//
// Since Ladybug library version 1.3.alpha.01, this example is modified to use
// ladybugRenderOffScreenImage(), which is hardware accelerated, to render the
// the stitched images.
//
// Typing ladybugPanoStitchExample /? (or ? -?) at the command prompt will  
// print out the usage information of this application.
//
//=============================================================================

//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <iostream>

//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>

//=============================================================================
// Other Includes
//=============================================================================
#include "socket.h"
#include "timing.h"
#include "turbojpeg.h"

//=============================================================================
// Macro Definitions
//=============================================================================

#define _HANDLE_ERROR \
	if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! Ladybug library reported %s\n", \
	::ladybugErrorToString( error ) ); \
	assert( true ); \
	goto _EXIT; \
	}	

#define IMAGES_TO_GRAB            10

// The size of the stitched image
#define PANORAMIC_IMAGE_WIDTH    8048
#define PANORAMIC_IMAGE_HEIGHT   4024

//#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
#define COLOR_PROCESSING_METHOD   LADYBUG_HQLINEAR          // High quality method suitable for large stitched images

//
// This function reads an image from camera
//
LadybugError
	initCamera( LadybugContext context)
{    
	// Initialize camera
	printf( "Initializing camera...\n" );
	LadybugError error = ladybugInitializeFromIndex( context, 0 );
	if (error != LADYBUG_OK)
	{
		return error;    
	}

	// Load config file
	printf( "Load configuration file...\n" );
	error = ladybugLoadConfig( context, NULL);
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
			LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8
			//LADYBUG_DATAFORMAT_RAW8 
			);
		break;

	case LADYBUG_DEVICE_LADYBUG:
	default:
		printf( "Unsupported device.\n");
		error = LADYBUG_FAILED;
	}

	return error;
}

LadybugError sendImages(LadybugContext context, zmq::socket_t &socket){
	double t_now = clock();	
	std::string status = "Create empty protobuf message";

	ladybug5_network::ImageMessage message;
	message.set_id(1);
	message.set_name("Windows_Client");
	_TIME

	
	LadybugError error;
	ladybug5_network::ImageType img_type;

	status = "create panorame in graphics card";
	// Stitch the images (inside the graphics card) and retrieve the output to the user's memory
	LadybugProcessedImage processedImage;
	char pszOutputName[256];
	sprintf( pszOutputName, "pano.jpg");
	error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
	_HANDLE_ERROR
	_TIME

	status = "Write panoramic picture to disk";

	//printf( "Writing image %s...\n", pszOutputName);
	error = ladybugSaveImage( context, &processedImage, pszOutputName, LADYBUG_FILEFORMAT_JPG);
	_TIME
_EXIT:

	status = "read panoramic from disk and create a message";
	ladybug5_network::Image* image_msg = 0;
	image_msg = message.add_images();
	//image_msg->set_number(1);
	image_msg->set_type(ladybug5_network::LADYBUG_PANORAMIC);

	unsigned int size;
	char* memblock=0;
	
	std::ifstream file (pszOutputName, std::ios::binary);
	if (file.is_open())
	{
		file.seekg(0, file.end);
		size = (unsigned int)file.tellg();
		assert(size != 0);
		memblock = new char [size];
		file.seekg (0, file.beg);
		file.read (memblock, size);
		file.close();
		image_msg->set_image(memblock, size);
		image_msg->set_size(size);
		_TIME

		status = "send img over network";
		socket_write(&socket, &message);
		_TIME
	}
	
	if(memblock !=0){
	delete[] memblock;
	
	_HANDLE_ERROR
	}

	return error;
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

//=============================================================================
// Main Routine
//=============================================================================
int 
	main( int argc, char* argv[] )
{
	printf("Start ladybug5_windows_client.cpp\nPano w: %i h: %i\n\n",PANORAMIC_IMAGE_WIDTH, 
		PANORAMIC_IMAGE_HEIGHT);
	double t_now = clock();	
	std::string status = "Creating zmq context and connect";
	
	zmq::context_t zmq_context(1);
	zmq::socket_t socket (zmq_context, ZMQ_PUB);


	//socket.connect ("tcp://149.201.37.83:28882");
	socket.connect ("tcp://149.201.37.83:28882");

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

	tjhandle _jpegCompressor = tjInitCompress();
_EXIT:	

	while(true)
	{
		try{

		printf("\nGrab Image loop...\n");
		// Grab an image from the camera
		status = "grab image";
		error = ladybugGrabImage(context, &image); 
		_HANDLE_ERROR
		_TIME

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
		char pszOutputName[256];
		sprintf( pszOutputName, "pano.jpg");
		error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
		_HANDLE_ERROR
		_TIME

		status = "jpg encode image";
		const int JPEG_QUALITY = 85;
		const int COLOR_COMPONENTS = 3;
		int _width = processedImage.uiCols; 
		int _height = processedImage.uiRows;
		long unsigned int _jpegSize = 0;
		unsigned char* _compressedImage = NULL; //!< Memory is allocated by tjCompress2 if _jpegSize == 0
		
		

		tjCompress2(_jpegCompressor, processedImage.pData, _width, 0, _height, TJPF_BGR,
				  &_compressedImage, &_jpegSize, TJSAMP_420, JPEG_QUALITY,
				  TJFLAG_FASTDCT);

		_TIME
		
		status = "write file to disk";
		//std::ofstream file("test.jpg", std::ios::binary);
		char* jpgCompressed = new char[image.uiDataSizeBytes];
		memcpy(jpgCompressed, _compressedImage, _jpegSize);
		tjFree(_compressedImage);

		//file.write(jpgCompressed, _jpegSize);
		//file.close();
		_TIME

		//error = saveImages(context, i);
		printf("Sending...\n");
		ladybug5_network::ImageMessage message;
		message.set_id(1);
		message.set_name("Windows_Client");
	
		LadybugError error;
		ladybug5_network::ImageType img_type;
		status = "read panoramic from disk and create a message";
		ladybug5_network::Image* image_msg = 0;
		image_msg = message.add_images();
		//image_msg->set_number(1);
		image_msg->set_image(jpgCompressed, _jpegSize);
		image_msg->set_size(_jpegSize);
		image_msg->set_type(ladybug5_network::LADYBUG_PANORAMIC);
		status = "send img over network";
		socket_write(&socket, &message);
		//error = sendImages(context, socket);		
		delete jpgCompressed;
		_TIME
		
		status = "Sum sending image";
		/*message="read from socket";
		socket_read(&socket);
		_TIME*/
		}
		catch(std::exception e){
			Sleep(1);
			printf("Exception, trying to recover\n");
		};
	}
	tjDestroy(_jpegCompressor);
	printf("Done.\n");


	//
	// clean up
	//

	ladybugStop( context );
	ladybugDestroyContext( &context );

	printf("<PRESS ANY KEY TO EXIT>");
	_getch();
	return 0;
}

