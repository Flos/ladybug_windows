// ladybug5_lut_export.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "timing.h"
//
// Includes
//
#include "helper.h"
#include "configuration_helper.h"
#include "thread_functions.h"
#include "myLadybug.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <WinBase.h>

bool show_images = false;

void fillMap(unsigned int h, unsigned int w, cv::Mat &mat_x, cv::Mat &mat_y, Ladybug &lady, unsigned int camera){
    // Row is h,y
    // Colum is w,x
    // Iterations

    LadybugError ok;  
    for(unsigned int x = 0; x < w; ++x){
        for(unsigned int y = 0; y < h; ++y){
            double dx = 0;
            double dy = 0;
            ok = ladybugUnrectifyPixel(lady.context, camera, y, x, &dy, &dx);
            if( ok == LADYBUG_OK &&
                dx > 1 && dx < w-1 &&
                dy > 1 && dy < h-1){
                mat_x.ptr<float>(y,x)[0] = (float) dx; 
                mat_y.ptr<float>(y,x)[0] = (float) dy;
            }
        }
    }
}

void save(cv::Mat &mat, std::string filename){
    std::cout << "Exporting " << filename << std::endl; 
    cv::FileStorage fs(filename.c_str(), cv::FileStorage::WRITE);
    fs << "mat" << mat;
    fs.release();
}

void load(cv::Mat &mat, std::string filename){
    cv::FileStorage fs(filename.c_str(), cv::FileStorage::READ);
    fs["mat"] >> mat;
    fs.release();
}
void createCalibrationMaps(unsigned int h, unsigned int w, unsigned int camera, cv::Mat &morph_mat_x, cv::Mat &morph_mat_y, Ladybug &lady) { // Remap
    std::string filenameX = std::to_string(lady.caminfo.serialHead) + "_camera" + std::to_string(camera) + "_h" + std::to_string(h) +"_w" +  std::to_string(w) + "_map_x.yaml";
    std::string filenameY = std::to_string(lady.caminfo.serialHead) + "_camera" + std::to_string(camera) + "_h" + std::to_string(h) +"_w" +  std::to_string(w) + "_map_y.yaml";
    
    fillMap(h, w, morph_mat_x, morph_mat_y, lady, camera);
    //cv::imwrite(filenameX.c_str(), morph_mat_x);
    //cv::imwrite(filenameY.c_str(), morph_mat_y);
   
    save(morph_mat_x, filenameX.c_str());
    save(morph_mat_y, filenameY.c_str());
}

void main( int argc, char* argv[] ){   
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    std::string status = "Init";
    double t_now = clock();
    initConfig(argc, argv);
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    printf("\nWating 5 sec...\n");
    //Sleep(5000); 
    Configuration config;
    config.cfg_rectification = true;
    config.cfg_panoramic = false;
    //config.cfg_ladybug_colorProcessing = LADYBUG_DOWNSAMPLE4;
    
    Ladybug lady(&config);
   
     //Get ladybug calibration
    CameraCalibration calibration;
    LadybugError error = LADYBUG_OK;

    LadybugImage image;
    error = lady.grabImage(&image);

    _TIME
    double t_loopstart = t_now;
    for(unsigned int camera = 0; camera < LADYBUG_NUM_CAMERAS; ++camera){
        error = lady.getCameraCalibration(camera, &calibration);
        std::cout << "Camera: " << camera << std::endl;

        LadybugProcessedImage rectified; 
        error = lady.grabProcessedImage(&rectified, (LadybugOutputImage) (1 << (6+camera)) );
        
        unsigned int h = rectified.uiRows;
        unsigned int w = rectified.uiCols;

        ArpBuffer* buffer = lady.getBuffer();
        cv::Mat image_raw_RGBA = cv::Mat( buffer->height, buffer->width, CV_8UC4,  buffer->getBuffer(camera));
        cv::Mat image_rectified_raw(rectified.uiRows, rectified.uiCols, CV_8UC3, rectified.pData);
        cv::Mat image_raw_RGB;
        cv::cvtColor(image_raw_RGBA, image_raw_RGB, CV_RGBA2RGB);
        cv::Mat image_raw; 
        cv::Size display_size(800,600);

        cv::Size size;
        size.height = buffer->height;
        size.width = buffer->width;

        cv::Mat imageUndistorted(size,CV_8UC3);
        cv::Mat morph_mat_x(h, w, CV_32FC1);
        cv::Mat morph_mat_y(h, w, CV_32FC1);

        createCalibrationMaps(h, w, camera, morph_mat_x, morph_mat_y, lady);

        cv::remap(image_raw_RGB, imageUndistorted, morph_mat_x, morph_mat_y, CV_INTER_LINEAR);
        cv::resize(imageUndistorted, imageUndistorted, display_size); 

        cv::Mat diff;
        cv::resize(image_rectified_raw, image_rectified_raw, display_size);
        cv::absdiff( imageUndistorted, image_rectified_raw, diff );

        if(show_images){  
            cv::imshow("Rectified with map defaut cam" + std::to_string(camera), imageUndistorted); 
            cv::imshow("diff cam" + std::to_string(camera), diff); 
        }
        status = "loop camera" + std::to_string(camera);
        _TIME
    }
    status = "Total time in loop";
    t_now=t_loopstart;
    _TIME
    if(show_images){ 
        cv::waitKey();
    }
	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
    
    return;
}