#pragma once
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>
#include <string>
#include <stdio.h>
#include "configuration.h"

#ifndef _ERROR
#define _ERROR \
    if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! Ladybug library reported %s\n", \
	::ladybugErrorToString( error ) );\
    return error;\
    }
#endif

#ifndef _ERROR_NORETURN
#define _ERROR_NORETURN \
    if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! Ladybug library reported %s\n", \
	::ladybugErrorToString( error ) );\
    throw new std::exception(::ladybugErrorToString( error ));\
    }
#endif

struct CameraCalibration{
    double rotationX,rotationY,rotationZ,translationX,translationY,translationZ;
    double focal_lenght; 
    double centerX, centerY;
    double distortion[5];
};

class LadybugStream{
public:
    std::string filename;
    unsigned int currentImage;
    LadybugError error;
    LadybugStreamContext streamContext;
    LadybugStreamHeadInfo streamHeadInfo;
    LadybugError grepNextImage(LadybugImage* image);
    bool loop;
    unsigned int imagesInTotalStream;
    std::string configfile;
    LadybugStream(LadybugContext& context, std::string filename, bool loop = true);
    ~LadybugStream();
};

class ArpBuffer{
public:
    ArpBuffer(unsigned int width, unsigned int height, unsigned int dimmensions);
	unsigned char* getBuffer(unsigned int);
    ~ArpBuffer();
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int dimmensions;
    unsigned int nrBuffers;
    unsigned char* buffers[ LADYBUG_NUM_CAMERAS ];
};

class Ladybug{
public:
    Ladybug();
	void init(Configuration* config = NULL);
    ~Ladybug();
    LadybugError grabImage(LadybugImage* image);
    LadybugError grabProcessedImage(LadybugProcessedImage* image, LadybugOutputImage imageType);
    LadybugCameraInfo caminfo;
    Configuration* config;
    LadybugError getCameraCalibration(unsigned int camera_index, CameraCalibration* calibration);
    LadybugError initProcessing(unsigned int cols = 0, unsigned int rows = 0);
    ArpBuffer* getBuffer();
    LadybugContext context;
    LadybugError error;
    bool isFileStream();
    double getCycleTime();
private:
    LadybugStream* _stream;
    ArpBuffer* _buffer;
    LadybugImage _raw_image;
    bool images_processed;
    bool initialised_processing;
    LadybugError start();
    LadybugError initCamera();
    LadybugError Ladybug::initStream(std::string path_streamfile);
};