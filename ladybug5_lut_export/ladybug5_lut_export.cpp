// ladybug5_lut_export.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "timing.h"
//
// Includes
//
#include "helper.h"
#include "configuration_helper.h"
#include "thread_functions.h"

void main( int argc, char* argv[] ){
    initConfig(argc, argv);
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    printf("\nWating 5 sec...\n");
    //Sleep(5000); 

    LadybugContext context;
    LadybugError error;
    LadybugImage image;
    const LadybugImage3d* arpImage3d[LADYBUG_NUM_CAMERAS];
    int srcCols = 0;
    int srcRows = 0;

    std::string status = "create ladybug context";
    // initialize the camera
    error = ladybugCreateContext( &context );
    _HANDLE_ERROR;

    error = ladybugInitializeFromIndex( context, 0 );
    if ( error != LADYBUG_OK){
        printf( "Error: Ladybug is not found.\n");
        exit( 1);
    }

    error = ladybugLoadConfig(context, NULL);
    _HANDLE_ERROR;

    // grab an image to check the image size
    error = ladybugStart(
        context,
        LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
    _HANDLE_ERROR;

    error = LADYBUG_FAILED;
    int retry = 10;
    do{
        error = ladybugGrabImage(context, &image);
    } while (error != LADYBUG_OK && retry-- > 0);
    _HANDLE_ERROR;

    error = ladybugStop(context);

    srcCols = image.uiCols;
    srcRows = image.uiRows;

    // get the mapping and print it
    printf( "cols %d rows %d\n", srcCols, srcRows);

    for (unsigned int camera = 0; camera < LADYBUG_NUM_CAMERAS; camera++)
    {
        error = ladybugGet3dMap(
            context, 
            camera, 
            srcCols, 
            srcRows,
            srcCols, 
            srcRows, 
            false,
            &arpImage3d[camera] );

        for (int iRow = 0; iRow < srcRows ; iRow++)
        {
            for (int iCol = 0; iCol < srcCols ; iCol++)
            {
                LadybugPoint3d p3d = arpImage3d[camera]->ppoints[iRow * srcCols + iCol];
                printf(
                    "%4.4lf,%4.4lf,%4.4lf\n", 
                    p3d.fX, 
                    p3d.fY, 
                    p3d.fZ);
            }
        }
    }

    // clean up
    error = ladybugDestroyContext(&context);
    _HANDLE_ERROR;
_EXIT:
	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
    
    return;
}