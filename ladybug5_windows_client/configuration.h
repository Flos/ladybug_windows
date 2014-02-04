#pragma once
#define PANORAMIC false
#define THREADING false
#define POSTPROCESSING true
// The size of the stitched image
#define PANORAMIC_IMAGE_HEIGHT   2048
#define PANORAMIC_IMAGE_WIDTH    4096
#define TEST_IMAGE_ZIPPING	false
#define NUMBER_THREADS 4
#define __WINDOWS__ true

#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
//#define COLOR_PROCESSING_METHOD   LADYBUG_HQLINEAR          // High quality method suitable for large stitched images