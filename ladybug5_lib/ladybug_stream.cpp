#include "ladybug_stream.h";

unsigned int BigLiltleEndianSwap (unsigned int i)
{
  unsigned char b1, b2, b3, b4;

  b1 = i & 255;
  b2 = ( i >> 8 ) & 255;
  b3 = ( i>>16 ) & 255;
  b4 = ( i>>24 ) & 255;

  return ((unsigned int)b1 << 24) + ((unsigned int)b2 << 16) + ((unsigned int)b3 << 8) + b4;
}

struct jpgPositionAndSize{
    unsigned int offset;
    unsigned int size;
};

void getColorOffset(LadybugImage *image, unsigned int& red_idx_offset, unsigned int& green_idx_offset, unsigned int& blue_idx_offset){
    switch(image->stippledFormat){
    case LADYBUG_BGGR:
        red_idx_offset = 3; 
        green_idx_offset = 1;
        blue_idx_offset = 0;
        break;
    case LADYBUG_GBRG:
        red_idx_offset = 2; 
        green_idx_offset = 0;
        blue_idx_offset = 1;
        break;
    case LADYBUG_GRBG:
        red_idx_offset = 1; 
        green_idx_offset = 0;
        blue_idx_offset = 2;
        break;
    case LADYBUG_RGGB:
        red_idx_offset = 0; 
        green_idx_offset = 1;
        blue_idx_offset = 3;
        break;
    default:
        throw new std::exception("getColorOffset, this image stippledFormat not implemted");
    }
}

std::string getBayerEncoding(LadybugImage *image){
	switch(image->stippledFormat){
    case LADYBUG_BGGR:
       return "BGGR";
    case LADYBUG_GBRG:
        return "GBRG";
    case LADYBUG_GRBG:
        return "GRBG";
    case LADYBUG_RGGB:
        return "RGGB";
    default:
        return "none";
    }
}
void extractImagesToFiles(LadybugImage* image){
    unsigned int imgCount = getImageCount(image); /* every channel as jpg */
    if(isColorSeparated(image)){
        jpgPositionAndSize* jpg = new jpgPositionAndSize[imgCount];
        for(unsigned int i=0x0340, j = 0 ; j < imgCount; i=i+8, j++){ /* extract jpg offsets and sizes from LadybugImage */
            unsigned int offset = *(unsigned int*)&image->pData[i];
            unsigned int size =  *(unsigned int*)&image->pData[i+4];
            jpg[j].size = BigLiltleEndianSwap(size); // jpg size and offset is stored in big endian
            jpg[j].offset = BigLiltleEndianSwap(offset); // jpg size and offset is stored in big endian
            //printf("Jpg%i begins at byte: %i and has a size of %i bytes", j, jpg[j].offset, jpg[j].size);
        }
        for(unsigned int i = 0; i < imgCount; ++i){
            std::ostringstream filename;
            filename << "extracted_" << i << ".jpg";
            //Extract images
            writeToFile(filename.str(),(char*)&image->pData[jpg[i].offset], jpg[i].size );
        }
    }
    else{
        imgCount = 6;
        int colorChannels = 4;
        unsigned int resolution = image->uiFullCols* image->uiFullRows;
        unsigned int dataDepth = getDataBitDepth(image);
        unsigned int imageSize = (resolution*dataDepth*colorChannels)/8;
        for(unsigned int i = 0; i < imgCount; ++i){
            std::ostringstream filename;
            filename << "extracted_" << i << ".raw";
            writeToFile(filename.str(),(char*)&image->pData[i*imageSize], imageSize ); //Extract images
        }
    }    
}

void extractImageToMsg(LadybugImage* image, unsigned int i, char** begin, unsigned int& size){
    unsigned int imgCount = getImageCount(image);
    if(isColorSeparated(image))
    {
        jpgPositionAndSize jpg;
        unsigned int offset = 0x0340 + i*8;
        unsigned int image_offset = *(unsigned int*)&image->pData[offset];
        unsigned int image_size = *(unsigned int*)&image->pData[offset+4];
        jpg.size = BigLiltleEndianSwap(image_size); // jpg size and offset is stored in big endian
        jpg.offset = BigLiltleEndianSwap(image_offset); // jpg size and offset is stored in big endian

        *begin = (char*)&image->pData[jpg.offset];
        size =  jpg.size;
        return;
    }
    else{
        unsigned int resolution = image->uiFullCols*image->uiFullRows;
        unsigned int imagesize = (double)resolution * ((double)getDataBitDepth(image)/8);
        unsigned int offset = imagesize*i;

        *begin = (char*)&image->pData[offset];
        size =  imagesize;
        return;
    }    
}

unsigned int getDataBitDepth(LadybugImage* image){
     switch(image->dataFormat){
            case LADYBUG_DATAFORMAT_RAW8:
            case LADYBUG_DATAFORMAT_COLOR_SEP_RAW8:
            case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8:
                return 8;
                break;
            case LADYBUG_DATAFORMAT_RAW12:
            case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12:
                return 12;
                break;
            case LADYBUG_DATAFORMAT_RAW16:
            case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16:
                return 16;
                break;
            default:
                throw new std::exception("bit size can not be dettermined");
     }
}

bool isColorSeparated(LadybugImage* image){
    switch(image->dataFormat){
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8:
            return true;
        default:
            return false;
    }
}

unsigned int getImageCount(LadybugImage* image){
     switch(image->dataFormat){
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8:
            return 6*4;
        default:
            return 6;
    }
}

void openPgrFile(char* filename){
    LadybugError error;
    LadybugImage image;
    LadybugContext context;
    LadybugStreamContext streamContext;
    unsigned int stream_image_count;
    //
    // Create contexts and prepare stream for reading
    //
    error = ladybugCreateContext( &context);

    error = ladybugCreateStreamContext( &streamContext);

    error = ladybugInitializeStreamForReading( streamContext, filename );

    error = ladybugGetStreamNumOfImages( streamContext, &stream_image_count);

    error = ladybugReadImageFromStream( streamContext, &image);  

    extractImagesToFiles(&image);
}