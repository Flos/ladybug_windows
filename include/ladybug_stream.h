#pragma once
#include <ladybugstream.h>
#include <ladybuggeom.h>
#include <iostream>
#include <sstream>
#include "helper.h"
#include <assert.h>

unsigned int getDataBitDepth(LadybugImage* image);
unsigned int getImageCount(LadybugImage* image);
bool isColorSeparated(LadybugImage* image);
std::string getBayerEncoding(LadybugImage *image);
void extractImagesToFiles(LadybugImage* image);
void extractImageToMsg(LadybugImage* image, unsigned int i, char** begin, unsigned int& size);
void openPgrFile(char* filename);
void getColorOffset(LadybugImage *image, unsigned int& red_idx_offset, unsigned int& green_idx_offset, unsigned int& blue_idx_offset);

void* getImagePointer(LadybugImage* image, unsigned int i, unsigned int& size);
unsigned int getDataBitDepth(LadybugProcessedImage* image);
