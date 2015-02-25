#include "myLadybug.h";
#include <boost\filesystem.hpp>
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

	if(!boost::filesystem::exists(filename)){
		std::cout << "File: " << filename << " dosent exits" << std::endl;
		_ERROR_NORETURN
	}

    this->currentImage = 0;
    error = ladybugCreateStreamContext( &streamContext);
    _ERROR_NORETURN

    error = ladybugInitializeStreamForReading( streamContext, filename.c_str() );
    _ERROR_NORETURN 

    error = ladybugGetStreamNumOfImages( streamContext, &imagesInTotalStream);
    _ERROR_NORETURN
	    
    error = ladybugGetStreamHeader( streamContext, &streamHeadInfo);
    _ERROR_NORETURN

	LadybugImage image;
    error = ladybugReadImageFromStream( streamContext, &image);
    _ERROR_NORETURN
   
    configfile = filename + ".cfg";
    error = ladybugGetStreamConfigFile( streamContext , configfile.c_str() );
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
Ladybug::init(Configuration* config, bool init_processing ){

	if(config == NULL){
        this->config = config;
    }else{
        this->config = config;
    }
    
    error = ladybugCreateContext( &context );
	_ERROR_NORETURN

    if(this->config->cfg_fileStream.empty()){
        error = initCamera();
    }
    else{
        error = initStream(this->config->cfg_fileStream);

		//Read information form stream back to config
		this->config->cfg_ladybug_dataformat = _stream->streamHeadInfo.dataFormat;
    }
	if( error != LADYBUG_OK ) 
	{     
		throw new std::exception(::ladybugErrorToString( error ));
	}

	error = start(init_processing);
	_ERROR_NORETURN
}


Ladybug::Ladybug(){
    _buffer = NULL;
    _stream = NULL;
    images_processed = false;
    initialised_processing = false;
	rectified_images_width = 0;
	rectified_image_height = 0;
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
    _ERROR

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

bool
Ladybug::isFileStream(){
    return !config->cfg_fileStream.empty();
}

LadybugError 
Ladybug::initProcessing(unsigned int cols, unsigned int rows){
    _ERROR
	// Set the panoramic view angle
	error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
	_ERROR

	// Make the rendering engine use the alpha mask
	error = ladybugSetAlphaMasking( context, true );
	_ERROR

    // Set color processing method.
    error = ladybugSetColorProcessingMethod( context, config->cfg_ladybug_colorProcessing );
	_ERROR

	calculate_rectified_image_size(cols, rows);

    //_buffer = new ArpBuffer(uiRawCols, uiRawRows, 4);

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
        else if(config->cfg_rectification){
            //  
            // Rectified images
            //   
			error = set_rectification_image_size(rectified_images_width, rectified_image_height);
			_ERROR
        }	   
        error = ladybugInitializeAlphaMasks( context, rectified_images_width, rectified_image_height );
        _ERROR;
    }

    initialised_processing = true;

	return error;
}

LadybugError
Ladybug::set_rectification_image_size(unsigned int cols, unsigned int rows){
	 error = ladybugSetOffScreenImageSize(
                context,
                LADYBUG_ALL_RECTIFIED_IMAGES, 
                cols, 
                rows);

	rectified_images_width = cols;
	rectified_image_height = rows;

	return error;
}

void 
Ladybug::calculate_rectified_image_size(unsigned int cols, unsigned int rows){
	rectified_images_width = _raw_image.uiCols;
	rectified_image_height = _raw_image.uiRows;

	if(cols == 0 || rows == 0){
		
		if (config->cfg_ladybug_colorProcessing == LADYBUG_DOWNSAMPLE4 || 
				config->cfg_ladybug_colorProcessing == LADYBUG_MONO)
		{
			rectified_images_width = _raw_image.uiCols / 2;
			rectified_image_height = _raw_image.uiRows / 2;
		}
	}
	else{
		rectified_images_width = cols;
		rectified_image_height = rows;
	}
}

double 
Ladybug::getCycleTime(){
    if( _stream != NULL)
    {
        return 1000/_stream->streamHeadInfo.ulFrameRate;
    }
    else
    {
        if(config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12 
            || config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8
            || config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8)
        {
            return 1000/16; // 16 FPS
        }
        else if(config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12
            || config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8)
        {
            return 1000/10;
        }
        else if(config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_RAW8
            || config->cfg_ladybug_dataformat == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16)
        {
            return 1000/8;
        }
        else{
            throw new std::exception("Dataformat not implemented");
        }
    }  
}

/* Ladybug live and filestream */
LadybugError 
Ladybug::start( bool init_processing ){
	printf("Starting Ladybug5 %d ...\n", caminfo.serialHead);
	_ERROR
    error = grabImage(&_raw_image);
    _ERROR

	if(init_processing){
		error = initProcessing();
		_ERROR
	}

	return error;
}

LadybugError 
Ladybug::getCameraCalibration(unsigned int camera_index, CameraCalibration* calibration){
    _ERROR
    if(!initialised_processing){ initProcessing(); };

	error = set_rectification_image_size(rectified_images_width, rectified_image_height); // make sure we get the right informations
	_ERROR

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

        error = ladybugConvertImage(context, &_raw_image, NULL); // _buffer->buffers);
        _ERROR

        error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
        _ERROR
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
	std::cout << "Ladybug destructor" << std::endl;
    ladybugDestroyContext( &context);
    if(_stream != NULL) delete _stream;
    if(_buffer != NULL) delete _buffer;
}
