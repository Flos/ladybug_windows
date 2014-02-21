#pragma once
#include "timing.h"
#include "turbojpeg.h"
#include "protobuf/imageMessage.pb.h"
#include "zmq.hpp"
#include <iostream>
#include <fstream>

/*Helper*/
unsigned int initBuffers(unsigned char** arpBuffers, unsigned int number, unsigned int width, unsigned int height, unsigned int dimensions = 4);
void initBuffersWitPicture(unsigned char** arpBuffers, long unsigned int* size);
void compressImageToMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg, int i, TJPF color = TJPF_BGRA);
zmq::message_t compressImageToZmqMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg, int i, TJPF color = TJPF_BGRA);
void addImageToMessage(ladybug5_network::pbMessage *message,  unsigned char* uncompressedBGRImageBuffer, TJPF color, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height);
void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height );
void writeToFile(std::string filename, char* data, size_t size);
