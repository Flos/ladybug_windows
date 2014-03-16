// export_distCoeffs.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
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
         double steps = 9;
         double stepW = w/steps;
         double stepH = h/steps;
         double elem = 45;
         if(false){
             stepH = 500;
             stepW = 500;
             elem = 2;
         }
         // Iterations
         for( double offset = 0; offset < stepH; offset = offset + stepH/elem){
             std::vector<cv::Point2f>image_p;
             std::vector<cv::Point3f>object_p;
             LadybugError ok;
             for(double x = offset; x < w; x = x+stepW){
                 for(double y = offset; y < h; y = y+stepH){
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

void fillCenterArray(unsigned int h, unsigned int w, std::vector<std::vector<cv::Point3f>> &object_points, std::vector<std::vector<cv::Point2f>> &image_points, Ladybug &lady, unsigned int camera, double border_X, double border_Y){
// Row is h,y
// Colum is w,x
// Iterations
    double stepsW = 7;
    double stepsH = 4; //stepsW*(3/4);
    double stepW = (w-(2*border_X))/stepsW;
    double stepH = (h-(2*border_Y))/stepsH;
    // Iterations
    std::vector<cv::Point2f>image_p;
    std::vector<cv::Point3f>object_p;
    LadybugError ok;
     double dx,dy;
    for(double x = border_X; x < w - border_X; x = x+stepW){
        for(double y = border_Y ; y < h - border_Y; y = y+stepH){
            dx = x;
            dy = y;
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
   if(image_p.size() >0) //= (stepsW*stepsH)/2)
   {
        image_points.push_back(image_p);
        object_points.push_back(object_p);
   }else{
       std::cout << "Recejted points only "  << image_p.size() << " found from " << stepsW*stepsH << std::endl;
   }
    
}

void calculateChessboardPoints(unsigned int h, unsigned int w, std::vector<std::vector<cv::Point3f>> &object_points, std::vector<std::vector<cv::Point2f>> &image_points, Ladybug &lady, unsigned int camera, double cx, double cy){
// Row is h,y
// Colum is w,x
// Iterations
//Borders: X: 514 // Y: 485;
    double steps = 5;
    double stepW = cx/steps;
    double stepH = cy/steps;
    double offsetMax = 800;
    // Iterations
    for( double off_X = 0; off_X < offsetMax; off_X = off_X + 160){
        for( double off_Y = 0; off_Y < offsetMax; off_Y = off_Y + 160){
            for( double i = 1; i < steps-1; ++i){
                double border_X = cx - i*stepW + off_X - (offsetMax/2);
                double border_Y = cy - i*stepH + off_Y - (offsetMax/2);
                fillCenterArray(h, w, object_points, image_points, lady, camera, border_X, border_Y);
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

void main( int argc, char* argv[] )
{
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
    config.cfg_ladybug_colorProcessing = LADYBUG_HQLINEAR;
    
    Ladybug lady(&config);
     //Get ladybug calibration
    CameraCalibration calibration;
    LadybugError error = LADYBUG_OK;

    LadybugImage image;
    error = lady.grabImage(&image);

    _TIME
    double t_loopstart = t_now;

    for(unsigned int camera = 5; camera < LADYBUG_NUM_CAMERAS; ++camera){
        double t_current_loop = clock();
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
        cv::Size display_size(800,600);

        cv::Size size;
        size.height = buffer->height;
        size.width = buffer->width;

        std::cout << "Focal lenght " << calibration.focal_lenght << " center: " << calibration.centerX << ", " << calibration.centerY << std::endl;
        cv::Mat intrinsic = getIntrinsics(calibration);
        cv::Mat imageUndistorted(size,CV_8UC3);
        printMatrix(&intrinsic , "intriniscs init");
             
        //std::vector<
        std::vector<std::vector<cv::Point3f>>object_points; //rectified points
        std::vector<std::vector<cv::Point2f>>image_points;
        calculateChessboardPoints( rectified.uiRows, rectified.uiCols, object_points, image_points, lady, camera, calibration.centerX, calibration.centerY);
        status = "chessboard calculation";
        _TIME

        cv::Mat distCoeffs;
        std::vector<cv::Mat> rvecs;
        std::vector<cv::Mat> tvecs;
        int flags = CV_CALIB_USE_INTRINSIC_GUESS | CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_FIX_FOCAL_LENGTH | CV_CALIB_RATIONAL_MODEL;  // CV_CALIB_FIX_ASPECT_RATIO | CV_CALIB_RATIONAL_MODEL | CV_CALIB_FIX_PRINCIPAL_POINT | CV_CALIB_FIX_FOCAL_LENGTH |
        flags = 0;
        cv::TermCriteria criteria(cv::TermCriteria::EPS, INT32_MAX, 0.02);
        double result = calibrateCamera(object_points, image_points, size, intrinsic, distCoeffs, rvecs, tvecs, flags, criteria );
        status = "calibration with: " + std::to_string(result);
        _TIME

        //initUndistortRectifyMap(i

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
            printMatrix(&newCam , "optimal cam");
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
            cv::undistort(image_raw_RGB, imageUndistorted, intrinsic, distCoeffs);
            cv::resize(imageUndistorted, imageUndistorted, display_size);
            cv::imshow("Rectified cv defaut", imageUndistorted);
        }
         t_now = t_current_loop;
         status = "loop camera" + std::to_string(camera);
        _TIME
    }
    status = "Total time in loop";
    t_now=t_loopstart;
    cv::waitKey();
	return;
}

