#include "myLadybug.h";
#include <assert.h>;

LadybugError
LadybugStream::grepNextImage(LadybugImage *image){
    ++currentImage;
    if(loop){
        currentImage = currentImage % streamHeadInfo.ulNumberOfImages;
    }
    error = ladybugGoToImage( streamContext, currentImage);
    _ERROR
    error = ladybugReadImageFromStream( streamContext, image);
    _ERROR
    return error;
}

LadybugStream::LadybugStream(std::string filename, bool loop){
    this->loop = loop;
    this->filename = filename;
};

LadybugStream::~LadybugStream(){

};


ArpBuffer::ArpBuffer(unsigned int width, unsigned int height, unsigned int dimmensions){
    
    this->width = width;
    this->height = height;
    this->dimmensions = dimmensions;
    this->size = width * height * dimmensions;
    this->nrBuffers = LADYBUG_NUM_CAMERAS;

    for( unsigned int uiCamera = 0; uiCamera < nrBuffers; uiCamera++ )
	{
		buffers[ uiCamera ] = new unsigned char[ size ];
	}
};

unsigned char* 
ArpBuffer::getBuffer(unsigned int bufferNr){
    assert(bufferNr < nrBuffers);

    return buffers[bufferNr]; 
};

ArpBuffer::~ArpBuffer(){
    for( unsigned int uiCamera = 0; uiCamera < nrBuffers; uiCamera++ ){ 
		delete  [] buffers[ uiCamera ];	    
    }
};

Ladybug::Ladybug(){
    buffer = NULL;
    stream = NULL;
    error = ladybugCreateContext( &context );
    _ERROR
    //error = initCamera();
	//error = start();
};

Ladybug::Ladybug(std::string filestream){
    error = ladybugCreateContext( &context );
    _ERROR

}

LadybugError 
Ladybug::grabImage(LadybugImage* image){
    if(stream != NULL){
        error = stream->grepNextImage(image);
    }else{
        error = ladybugGrabImage(context, image);
    }
    _ERROR
    return error;
}

LadybugError
Ladybug::initCamera(LadybugAutoExposureMode mode, LadybugAutoShutterRange shutter)
{    
	// Initialize camera
	printf( "Initializing camera...\n" );
	LadybugError error = LADYBUG_NOT_STARTED;
	
	error = ladybugInitializeFromIndex( context, 0 );
	_ERROR

	// Load config file
	printf( "Load configuration file...\n" );
	error = ladybugLoadConfig( context, NULL);
    _ERROR

    // Set Exposure mode
    error = ladybugSetAutoExposureROI( context, mode);
    _ERROR

    // Set Shutter Range
    error = ladybugSetAutoShutterRange( context, shutter);
    _ERROR

	return error;
}

//LadybugError
//Ladybug::setOutputImage(LadybugOutputImage imageType){
//    // Configure output images in Ladybug library
//	printf( "Configure output images in Ladybug library...\n" );
//	error = ladybugConfigureOutputImages( context, imageType );
//	_ERROR
//
//	error = ladybugSetOffScreenImageSize(
//		context, 
//        LADYBUG_PANORAMIC,  
//        cfg_pano_width,
//        cfg_pano_hight );  
//	_ERROR
//}

LadybugError 
Ladybug::initStream(std::string path_streamfile){
    this->stream = new LadybugStream(path_streamfile);
    error = stream->error;
    _ERROR
    return error;
}

LadybugError 
Ladybug::initProcessing(LadybugColorProcessingMethod color, unsigned int imageTypes){
	
	// Set the panoramic view angle
	LadybugError error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
	_ERROR

	// Make the rendering engine use the alpha mask
	error = ladybugSetAlphaMasking( context, true );
	_ERROR

	// Set color processing method.
    error = ladybugSetColorProcessingMethod( context, color );
	_ERROR
	return error;
}

LadybugError 
Ladybug::start(LadybugDataFormat format){
	// Get camera info
	printf( "Getting camera info...\n" );
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
            format
			);
		break;

	case LADYBUG_DEVICE_LADYBUG:
	default:
		printf( "Unsupported device.\n");
		error = LADYBUG_FAILED;
	}

	return error;
}
