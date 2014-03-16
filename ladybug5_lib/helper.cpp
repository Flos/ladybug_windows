#include "helper.h"




void jpegEncode(unsigned char* _compressedImage, unsigned long *_jpegSize, unsigned char* srcBuffer, int JPEG_QUALITY, int _width, int _height ){
	const int COLOR_COMPONENTS = 3;
		
	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, srcBuffer, _width, 0, _height, TJPF_BGR,
				&_compressedImage, _jpegSize, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);	
}



unsigned int initBuffers(unsigned char** arpBuffers, unsigned int number, unsigned int width, unsigned int height, unsigned int dimensions){
	//
	// Initialize the pointers to NULL 
	//
    unsigned int size = width * height * dimensions;
	std::cout << "Initialised arpBuffers with size: "<< size << " width: " << width << " height: " << height << std::endl;
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		arpBuffers[ uiCamera ] = new unsigned char[ size ];
	}
    return size;
}


void initBuffersWitPicture(unsigned char** arpBuffers, long unsigned int* size){
	//
	// Initialize the pointers to NULL 
	//
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		char filename[24]; 
		std::sprintf(filename, "input\\CAM%i.bmp", uiCamera);
		std::ifstream file (filename, std::ios::in|std::ios::binary|std::ios::ate);
		if (file.is_open())
		{
			size[uiCamera] = file.tellg();
			arpBuffers[ uiCamera ] = new unsigned char [size[uiCamera]];
			file.seekg (0, std::ios::beg);
			file.read( (char*)arpBuffers[ uiCamera ], size[uiCamera]);
			file.close();
			std::cout << "Initialised arpBuffers with size: "<< (double)size[uiCamera]/(1024*1024) << " MiB  "<< filename << std::endl;
		}else{
			std::cout << "Cant open file " << filename << std::endl;
		}
	}
}

void writeToFile(std::string filename, char* data, size_t size){
    std::ofstream file(filename, std::ios::binary);
    if(file.is_open() ) 
    {
        file.write(data, size);
        file.flush();
        file.close();
    }
}

void compressImages(ladybug5_network::pbMessage *message, unsigned char* arpBuffers, TJPF TJPF_BGRA, ladybug5_network::LadybugTimeStamp *msg_timestamp, int uiRawCols, int uiRawRows )
{
	for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
	{
		addImageToMessage(message, &arpBuffers[uiCamera], TJPF_BGRA, msg_timestamp, (ladybug5_network::ImageType) ( 1 << uiCamera), uiRawCols, uiRawRows );
	}
}

void compressImageToMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg, int i, TJPF color){
	//encode to jpg
    double t_now = clock();
	std::string status = "Compression";
	unsigned char* _compressedImage = 0;
	int JPEG_QUALITY = 85;
	
	int img_width =  message->images(i).width();
	int img_height =  message->images(i).height();
	unsigned long img_Size = 0;
	ladybug5_network::ImageType img_type = message->images(i).type();

	unsigned char* pointer = (unsigned char*)zmq_msg->data();
	assert(zmq_msg->size()!=0);
	assert(img_width!=0);
	assert(img_height!=0);
	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, pointer, img_width, 0, img_height, color,
				&_compressedImage, &img_Size, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);
    _TIME
    status = "updating image message";
	assert(img_Size!=0);

	//message->clear_images();
	ladybug5_network::pbImage* img_msg =(ladybug5_network::pbImage*) &message->images(i);

	img_msg->set_image(_compressedImage, img_Size);
	img_msg->set_size(img_Size);

	tjFree(_compressedImage);
    _TIME
}


zmq::message_t compressImageToZmqMsg(ladybug5_network::pbMessage *message, zmq::message_t* zmq_msg, int i, TJPF color){
	//encode to jpg
    double t_now = clock();
	std::string status = "Compression";
	unsigned char* _compressedImage = 0;
	int JPEG_QUALITY = 85;
	
	int img_width =  message->images(i).width();
	int img_height =  message->images(i).height();
	unsigned long img_Size = 0;
	ladybug5_network::ImageType img_type = message->images(i).type();

	unsigned char* pointer = (unsigned char*)zmq_msg->data();
	assert(zmq_msg->size()!=0);
	assert(img_width!=0);
	assert(img_height!=0);
	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, pointer, img_width, 0, img_height, color,
				&_compressedImage, &img_Size, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);
    _TIME
    status = "updating image message";
	assert(img_Size!=0);

    zmq::message_t img_out(img_Size);
    memcpy(img_out.data(), _compressedImage, img_Size);

	tjFree(_compressedImage);
    //_TIME
    return img_out;
}


void addImageToMessage(ladybug5_network::pbMessage *message,  unsigned char* uncompressedBGRImageBuffer, TJPF color, ladybug5_network::LadybugTimeStamp *timestamp, ladybug5_network::ImageType img_type, int _width, int _height){
	//encode to jpg
	//const int COLOR_COMPONENTS = 4;
	unsigned long imgSize = 0;
	unsigned char* _compressedImage = 0;
	int JPEG_QUALITY = 85;

	tjhandle _jpegCompressor = tjInitCompress();
	tjCompress2(_jpegCompressor, uncompressedBGRImageBuffer, _width, 0, _height, color,
				&_compressedImage, &imgSize, TJSAMP_420, JPEG_QUALITY,
				TJFLAG_FASTDCT);
	tjDestroy(_jpegCompressor);
	
	//append to message
	ladybug5_network::pbImage* image_msg = 0;
	image_msg = message->add_images();
	image_msg->set_image(_compressedImage, imgSize);
	image_msg->set_size(imgSize);
	image_msg->set_type(img_type);
	image_msg->set_name(enumToString(img_type));
	tjFree(_compressedImage);
}
