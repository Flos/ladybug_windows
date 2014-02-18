#include "ladybug_helper.h"

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

    // Set Exposure mode
    error = ladybugSetAutoExposureROI( context, cfg_ladybug_autoExposureMode);
    if (error != LADYBUG_OK)
	{
		return error;    
	}

    // Set Shutter Range
    error = ladybugSetAutoShutterRange( context, cfg_ladybug_autoShutterRange);
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
    error = ladybugSetColorProcessingMethod( context, cfg_ladybug_colorProcessing );
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
        cfg_pano_width,
        cfg_pano_hight );  
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
			//LADYBUG_DATAFORMAT_RAW8 
            cfg_ladybug_dataformat
			);
		break;

	case LADYBUG_DEVICE_LADYBUG:
	default:
		printf( "Unsupported device.\n");
		error = LADYBUG_FAILED;
	}

	return error;
}

void ladybugImagetoDisk(LadybugImage *image, std::string filename){
    image->stippledFormat;
    image->dataFormat;
    image->uiDataSizeBytes;
    image->bStippled;
    size_t size = sizeof(image);
    std::ofstream myfile(filename.c_str() , std::ios::out | std::ios::app | std::ios::binary);	
    if (myfile.is_open()) {

    }
    
    //LadybugRead

}


LadybugError saveImages(LadybugContext context, int i){

	LadybugError error;
	std::string status = "saving images";
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


void extractCalibration( LadybugContext context, LadybugImage *image){
    LadybugCameraInfo info;
    ladybugGetCameraInfo(context, &info);
    for(int i = 0; i < 6; ++i){
        double extrinsics[6];
        ladybugGetCameraUnitExtrinsics(context, i, extrinsics);
        double focal_lenght; 
        ladybugGetCameraUnitFocalLength(context, i, &focal_lenght);
        double centerX,centerY;
        ladybugGetCameraUnitImageCenter(context , i, &centerX, &centerY);
        printf("Camera %i Extrensics: [%f, %f, %f, %f, %f, %f]", i, extrinsics[0], extrinsics[1], extrinsics[2], extrinsics[3], extrinsics[4], extrinsics[5]); 
        printf("FocalLength: %f", focal_lenght);
        printf("CameraUnitImageCenter: %f, %f", focal_lenght);
    }
}