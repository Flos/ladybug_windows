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

bool DISTORTION_KOEFF = false;
bool LUT              = true;

cv::Mat getIntrinsics(CameraCalibration calibration){
    cv::Mat intrinsic(3, 3, CV_64FC1);
    intrinsic.ptr<double>(0)[0] = calibration.focal_lenght;
    intrinsic.ptr<double>(0)[1] = 0;
    intrinsic.ptr<double>(0)[2] = calibration.centerX;

    intrinsic.ptr<double>(1)[0] = 0;
    intrinsic.ptr<double>(1)[1] = calibration.focal_lenght;
    intrinsic.ptr<double>(1)[2] = calibration.centerY;

    intrinsic.ptr<double>(2)[0] = 0;
    intrinsic.ptr<double>(2)[1] = 0;
    intrinsic.ptr<double>(2)[2] = 1;
    return intrinsic;
}

void printMatrix(cv::Mat* math, std::string name="Matrix"){
    std::string limiter = "-------------";
    std::cout << name << std::endl << limiter << std::endl;
   
    std::cout << *math << std::endl;
    
    std::cout << std::endl << limiter << std::endl;
}

void fillArray(unsigned int h, unsigned int w, std::vector<std::vector<cv::Point3f>> &object_points, std::vector<std::vector<cv::Point2f>> &image_points, Ladybug &lady, unsigned int camera){
        // Row is h,y
        // Colum is w,x
        // Iterations
         double steps = 250;
         double step = w/steps;
         double elem = 25;
         if(false){
             step = 500;
             elem = 2;
         }
         // Iterations
         for( double offset = 0; offset < step; offset = offset + step/elem){
             std::vector<cv::Point2f>image_p;
             std::vector<cv::Point3f>object_p;
             LadybugError ok;
             for(double x = offset; x < w; x = x+step){
                 for(double y = offset; y < h; y = y+step){
                    double dx = x;
                    double dy = y;
                    ok = ladybugUnrectifyPixel(lady.context, camera, y, x, &dy, &dx);
                    if( ok == LADYBUG_OK &&
                         (dx > 1) && dx < w-1 &&
                         (dy > 1) && dy < h-1
                            ){
                        image_p.push_back(cv::Point2f(dx,dy));
                        object_p.push_back(cv::Point3f(x, y, 0.0f));
                    }
                }
             }
             image_points.push_back(image_p);
             object_points.push_back(object_p);
         }
}

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

void createConfigYaml(std::string serialnumber, unsigned int camera, cv::Mat intrinsics, cv::Mat distCoeffs ){
        std::string filename = serialnumber + "_camera" + std::to_string(camera) + ".yaml";
        cv::FileStorage fs(filename.c_str(), cv::FileStorage::WRITE);
        time_t rawtime; time(&rawtime);
        fs << "calibrationDate" << asctime(localtime(&rawtime));
        fs << "intrinsics" << intrinsics << "distCoeffs" << distCoeffs;
        fs.release();
}

void createCalibrationMaps(unsigned int h, unsigned int w, unsigned int camera, cv::Mat &morph_mat_x, cv::Mat &morph_mat_y, Ladybug &lady) { // Remap
    std::string filenameX = std::to_string(lady.caminfo.serialHead) + "_camera" + std::to_string(camera) + "_map_x.bmp";
    std::string filenameY = std::to_string(lady.caminfo.serialHead) + "_camera" + std::to_string(camera) + "_map_y.bmp";
    
    fillMap(h, w, morph_mat_x, morph_mat_y, lady, camera);
    cv::imwrite(filenameX.c_str(), morph_mat_x);
    cv::imwrite(filenameY.c_str(), morph_mat_y);
}

void main( int argc, char* argv[] ){   
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);

    initConfig(argc, argv);
    std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
    printf("\nWating 5 sec...\n");
    //Sleep(5000); 
    Configuration config;
    config.cfg_rectification = true;
    config.cfg_panoramic = false;
    config.cfg_ladybug_colorProcessing = LADYBUG_HQLINEAR;
    
    Ladybug lady(&config);
     //Get ladybug calibration
    CameraCalibration calibration;
    LadybugError error = LADYBUG_OK;

    LadybugImage image;
    error = lady.grabImage(&image);


    for(unsigned int camera = 0; camera < LADYBUG_NUM_CAMERAS; ++camera){
        error = lady.getCameraCalibration(camera, &calibration);
        std::cout << "Camera: " << camera << std::endl;

        LadybugProcessedImage raw; 
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

        std::cout << "Focal lenght " << calibration.focal_lenght << " center: " << calibration.centerX << ", " << calibration.centerY << std::endl;
        cv::Mat intrinsic = getIntrinsics(calibration);
        cv::Mat imageUndistorted(size,CV_8UC3);
        printMatrix(&intrinsic , "intriniscs init");
   
        if(DISTORTION_KOEFF){            
            //std::vector<
            std::vector<std::vector<cv::Point3f>>object_points; //rectified points
            std::vector<std::vector<cv::Point2f>>image_points;
            fillArray( rectified.uiRows, rectified.uiCols, object_points, image_points, lady, camera);
        
            cv::Mat distCoeffs;
            std::vector<cv::Mat> rvecs;
            std::vector<cv::Mat> tvecs;
            int flags = CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_FIX_FOCAL_LENGTH | CV_CALIB_USE_INTRINSIC_GUESS | CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_RATIONAL_MODEL; 
            //flags = 0;
            cv::TermCriteria criteria(cv::TermCriteria::EPS, INT32_MAX, 0.2);
            calibrateCamera(object_points, image_points, size, intrinsic, distCoeffs, rvecs, tvecs, flags, criteria );
            
            createConfigYaml(std::to_string(lady.caminfo.serialHead), camera, intrinsic, distCoeffs );
            printMatrix(&intrinsic , "calib intriniscs");
            printMatrix(&distCoeffs , "distCoeeff");
        
            //cv::namedWindow("rectified lb", cv::WINDOW_NORMAL);
            {
                cv::Mat resized_rgb;
                cv::resize(image_rectified_raw, image_rectified_raw, display_size);
                cv::imshow("rectified lb", image_rectified_raw);
                cv::resize(image_raw_RGB, resized_rgb, display_size);
                cv::imshow("image_raw_RGB", resized_rgb);
            }
            
            {
                cv::Mat newCam = cv::getOptimalNewCameraMatrix(intrinsic, distCoeffs, size, 0, size);
                printMatrix(&distCoeffs , "optimal cam");
                //With optimal cam
                cv::undistort(image_raw_RGB, imageUndistorted, intrinsic, distCoeffs
                    ,newCam
                    );
                //cv::namedWindow("cv rectified", cv::WINDOW_NORMAL);
 
                cv::resize(imageUndistorted, imageUndistorted, display_size);
                cv::imshow("Rectified optimal cam", imageUndistorted);
            }

            {
                //With pg intrinsics
                cv::undistort(image_raw_RGB, imageUndistorted, intrinsic, distCoeffs
                    ,getIntrinsics(calibration)
                );
                cv::resize(imageUndistorted, imageUndistorted, display_size);
                cv::imshow("Rectified init intrinsics", imageUndistorted);
            }

             {
                //With out any
                cv::undistort(image_raw_RGB, imageUndistorted, intrinsic, distCoeffs
                );
                cv::resize(imageUndistorted, imageUndistorted, display_size);
                cv::imshow("Rectified cv defaut", imageUndistorted);
            }
        }
        if(LUT){
            cv::Mat morph_mat_x(h, w, CV_32FC1);
            cv::Mat morph_mat_y(h, w, CV_32FC1);
            createCalibrationMaps(h, w, camera, morph_mat_x, morph_mat_y, lady);

            cv::remap(image_raw_RGB, imageUndistorted, morph_mat_x, morph_mat_y, CV_INTER_LINEAR);
            cv::resize(imageUndistorted, imageUndistorted, display_size);
            cv::imshow("Rectified with map defaut", imageUndistorted);
        }  

        cv::Mat diff;
        cv::resize(image_rectified_raw, image_rectified_raw, display_size);
        cv::absdiff( imageUndistorted, image_rectified_raw, diff );
        cv::imshow("diff", diff);        
    }
    cv::waitKey();
	std::printf("<PRESS ANY KEY TO EXIT>");
	_getch();
    
    return;
}