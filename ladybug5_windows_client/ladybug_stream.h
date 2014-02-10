#pragma once
#include <ladybugstream.h>
#include <ladybuggeom.h>
#include <iostream>
#include <sstream>
#include "functions.h"
#include <assert.h>

unsigned int getDataBitDepth(LadybugImage* image);
unsigned int getImageCount(LadybugImage* image);
bool isColorSeperated(LadybugImage* image);
void extractImagesToFiles(LadybugImage* image);
void extractImageToMsg(LadybugImage* image, unsigned int i, zmq::message_t* msg);
void openPgrFile(char* filename);