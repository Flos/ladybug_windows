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


void extractImages(LadybugImage* image){
    unsigned int imgCount = 6*4; /* every channel as jpg */
    switch(image->dataFormat){
        case LADYBUG_DATAFORMAT_JPEG8:
            imgCount = 6;
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12:
        case LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG8:
        case LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8:
            //Get images sizes
            {
                jpgPositionAndSize* jpg = new jpgPositionAndSize[imgCount];
                for(int i=0x0340, j = 0 ; j < imgCount; i=i+8, j++){ /* extract jpg offsets and sizes from LadybugImage */
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
            break;
        default:  //Raw images
            imgCount = 6;
            int colorChannels = 4;
            unsigned int resolution = image->uiFullCols* image->uiFullRows;
            unsigned int dataDepth = 0;
            switch(image->dataFormat){
                case LADYBUG_DATAFORMAT_RAW8:
                case LADYBUG_DATAFORMAT_COLOR_SEP_RAW8:
                case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW8:
                    dataDepth = 8;
                    break;
                case LADYBUG_DATAFORMAT_RAW12:
                case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12:
                    dataDepth = 12;
                    break;
                case LADYBUG_DATAFORMAT_RAW16:
                case LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16:
                    dataDepth = 16;
            }
            unsigned int imageSize = (resolution*dataDepth*colorChannels)/8;
            for(unsigned int i = 0; i < imgCount; ++i){
            std::ostringstream filename;
            filename << "extracted_" << i << ".raw";
            writeToFile(filename.str(),(char*)&image->pData[i*imageSize], imageSize ); //Extract images
            }
    }    
}

void openPgrFile(char* filename){
    LadybugError error;
    LadybugImage image;
    LadybugContext context;
    LadybugStreamContext streamContext;
    //
    // Create contexts and prepare stream for reading
    //
    error = ladybugCreateContext( &context);

    error = ladybugCreateStreamContext( &streamContext);

    error = ladybugInitializeStreamForReading( streamContext, filename );

    error = ladybugReadImageFromStream( streamContext, &image);

    extractImages(&image);
}