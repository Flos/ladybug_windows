#include "myLadybug.h";
#include <assert.h>;

LadybugError
LadybugStream::grepNextImage(LadybugImage *image){

    error = ladybugGoToImage( streamContext, currentImage);
    _ERROR
    error = ladybugReadImageFromStream( streamContext, image);
    _ERROR

    ++currentImage;
    if(loop){
        currentImage = currentImage % imagesInTotalStream;
    }
    return error;
}

LadybugStream::LadybugStream(LadybugContext& context, std::string filename, bool loop){
    this->loop = loop;
    this->filename = filename;
    this->currentImage = 0;
    error = ladybugCreateStreamContext( &streamContext);
    _ERROR_NORETURN

    error = ladybugInitializeStreamForReading( streamContext, filename.c_str() );
    _ERROR_NORETURN 

    error = ladybugGetStreamNumOfImages( streamContext, &imagesInTotalStream);
    _ERROR_NORETURN
	    
    error = ladybugGetStreamHeader( streamContext, &streamHeadInfo);
    _ERROR_NORETURN
   
    configfile = filename + ".cfg";
    error = ladybugGetStreamConfigFile( streamContext , configfile.c_str() );
    _ERROR_NORETURN

    LadybugImage image;
    error = ladybugReadImageFromStream( streamContext, &image);
    _ERROR_NORETURN

    error = ladybugLoadConfig( context, configfile.c_str() );
    _ERROR_NORETURN
};

LadybugStream::~LadybugStream(){
    ladybugDestroyStreamContext( &streamContext);
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

void
Ladybug::init(){
    _buffer = NULL;
    _stream = NULL;
    images_processed = false;
    error = ladybugCreateContext( &context );
    _ERROR_NORETURN
}

Ladybug::Ladybug(Configuration* config){
    if(config == NULL){
        this->config = new Configuration();
    }else{
        this->config = new Configuration(*config);
    }
    
    init();

    if(this->config->cfg_fileStream.empty()){
        error = initCamera();
    }
    else{
        error = initStream(this->config->cfg_fileStream);
    }
    error = start();
    _ERROR_NORETURN
};

LadybugError 
Ladybug::grabImage(LadybugImage* image){
    images_processed = false;
    if(_stream != NULL){
        error = _stream->grepNextImage(image);
    }else{
        error = ladybugGrabImage(context, image);
    }
    _ERROR

    _raw_image = *image;
    return error;
}

LadybugError
Ladybug::initCamera()
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

    // Get camera info
	printf( "Getting camera info...\n" );
	error = ladybugGetCameraInfo( context, &caminfo );

    // Set Exposure mode
    error = ladybugSetAutoExposureROI( context, config->cfg_ladybug_autoExposureMode);
    _ERROR

    // Set Shutter Range
    error = ladybugSetAutoShutterRange( context, config->cfg_ladybug_autoShutterRange);
    _ERROR

    error = ladybugStart(
		context,
        config->cfg_ladybug_dataformat
		);

	return error;
}


LadybugError 
Ladybug::initStream(std::string path_streamfile){
    this->_stream = new LadybugStream(context, path_streamfile);
    error = _stream->error;
    _ERROR

    caminfo.serialHead = _stream->streamHeadInfo.serialHead;
    caminfo.serialBase = _stream->streamHeadInfo.serialBase;
    _ERROR
    
    return error;
}

LadybugError 
Ladybug::initProcessing(){
    LadybugImage image;
    grabImage(&image);

	// Set the panoramic view angle
	error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
	_ERROR

	// Make the rendering engine use the alpha mask
	error = ladybugSetAlphaMasking( context, true );
	_ERROR

    // Set color processing method.
    error = ladybugSetColorProcessingMethod( context, config->cfg_ladybug_colorProcessing );
	_ERROR

    unsigned int uiRawCols = image.uiCols;
	unsigned int uiRawRows = image.uiRows;
    if (config->cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
	        config->cfg_ladybug_colorProcessing == LADYBUG_MONO)
    {
	    uiRawCols = image.uiCols / 2;
	    uiRawRows = image.uiRows / 2;
    }

    _buffer = new ArpBuffer(uiRawCols, uiRawRows, 4);

    if(config->cfg_panoramic || config->cfg_rectification){
        unsigned int uiImageTypes = 0;
        printf( "Configure output images in Ladybug library...\n" );
        if(config->cfg_panoramic){ uiImageTypes = uiImageTypes | LADYBUG_PANORAMIC;}
        if(config->cfg_rectification){ uiImageTypes = uiImageTypes | LADYBUG_ALL_RECTIFIED_IMAGES; }

        error = ladybugConfigureOutputImages( context, uiImageTypes );
	    _ERROR
        
        if(config->cfg_panoramic){
   	        error = ladybugSetOffScreenImageSize(
		        context, 
                LADYBUG_PANORAMIC,  
                config->cfg_pano_width,
                config->cfg_pano_hight );  
	        _ERROR
        }
        if(config->cfg_rectification){
            //  
            // Rectified images
            //   
            error = ladybugSetOffScreenImageSize(
                context,
                LADYBUG_ALL_RECTIFIED_IMAGES, 
                uiRawCols, 
                uiRawRows);
        }
         // Configure output images in Ladybug library
	   
        error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
        _ERROR;
    }
	return error;

}

/* Ladybug live and filestream */
LadybugError 
Ladybug::start(){
	
    _ERROR
	printf("Starting %s (%u)...\n", caminfo.pszModelName, caminfo.serialHead);

    if(config->cfg_rectification || config->cfg_panoramic){
        initProcessing();
    }
    error = grabImage(&_raw_image);

	return error;
}

LadybugError 
Ladybug::getCameraCalibration(unsigned int camera_index, CameraCalibration* calibration){
    double extrinsics[6];
    error = ladybugGetCameraUnitExtrinsics(context, camera_index, extrinsics);
    _ERROR
    calibration->rotationX = extrinsics[0];
    calibration->rotationY = extrinsics[1];
    calibration->rotationZ = extrinsics[2];
    calibration->translationX = extrinsics[3];
    calibration->translationY = extrinsics[4];
    calibration->translationZ = extrinsics[5];

    error = ladybugGetCameraUnitFocalLength(context, camera_index, &calibration->focal_lenght);
    _ERROR
       
    error = ladybugGetCameraUnitImageCenter(context , camera_index, &calibration->centerX, &calibration->centerY);
    _ERROR
    return error;
}

LadybugError 
Ladybug::grabProcessedImage(LadybugProcessedImage* image, LadybugOutputImage imageType){
    if(!images_processed){
        error = ladybugConvertImage(context, &_raw_image, _buffer->buffers);
        _ERROR
        error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
        images_processed = true;
    }
    error = ladybugRenderOffScreenImage(context, imageType, LADYBUG_BGR, image);
    _ERROR
    return error;
}

ArpBuffer* 
Ladybug::getBuffer(){
    if(_buffer!=NULL){
        return _buffer;
    }
    else{
       return NULL;
    }
}
    

Ladybug::~Ladybug(){
    ladybugDestroyContext( &context);
    if(_stream != NULL) delete _stream;
    if(config != NULL) delete config;
    if(_buffer != NULL) delete _buffer;
}
