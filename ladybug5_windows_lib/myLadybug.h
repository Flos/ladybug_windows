#pragma once
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>
#include <string>
#include <stdio.h>

#ifndef _ERROR
#define _ERROR \
    if( error != LADYBUG_OK ) \
	{ \
	printf( "Error! Ladybug library reported %s\n", \
	::ladybugErrorToString( error ) );\
    }
#endif


class LadybugStream{
public:
    std::string filename;
    unsigned int currentImage;
    LadybugError error;
    LadybugStreamContext streamContext;
    LadybugStreamHeadInfo streamHeadInfo;
    LadybugError grepNextImage(LadybugImage* image);
    bool loop;
    LadybugStream(std::string filename, bool loop = true);
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
private:
    unsigned char* buffers[ LADYBUG_NUM_CAMERAS ];
};

class Ladybug{
public:
    Ladybug();
    Ladybug(std::string filestream);
    LadybugError grabImage(LadybugImage* image);
    LadybugError initCamera(LadybugAutoExposureMode mode, LadybugAutoShutterRange shutter);
    LadybugError Ladybug::initStream(std::string path_streamfile);
    LadybugError initProcessing(LadybugColorProcessingMethod color, unsigned int imageTypes);
    ArpBuffer* buffer;
    LadybugCameraInfo caminfo;
private:
    LadybugStream* stream;
    LadybugContext context;
    LadybugError error;
    LadybugError start(LadybugDataFormat format);
};