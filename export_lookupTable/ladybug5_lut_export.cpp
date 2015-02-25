// ladybug5_lut_export.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//


//
// Includes
//
#include "timing.h"
#include "configuration_helper.h"
#include "myLadybug.h"
#include "ladybug_stream.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <Windows.h>
#include <WinBase.h>
#include <boost/filesystem.hpp>

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

void save(cv::Mat &mat_x, cv::Mat &mat_y, std::string filename, CameraCalibration calib, int camera, int height, int width){
	{
		cv::FileStorage fs(filename+".yaml.gz", cv::FileStorage::WRITE);
		fs << "camera" << camera;
		fs << "width" << width;
		fs << "height" << height;
		fs << "cx" << calib.centerX;
		fs << "cy" << calib.centerY;
		fs << "fx" << calib.focal_lenght;
		fs << "fy" << calib.focal_lenght;
		fs << "rx" << calib.rotationX;
		fs << "ry" << calib.rotationY;
		fs << "rz" << calib.rotationZ;
		fs << "x" << calib.translationX;
		fs << "y" << calib.translationY;
		fs << "z" << calib.translationZ;
		fs << "remap_x" << mat_x;
		fs << "remap_y" << mat_y;
		fs.release();
	}

	{
		cv::FileStorage fs(filename+"_calib.yaml", cv::FileStorage::WRITE);
		fs << "camera" << camera;
		fs << "width" << width;
		fs << "height" << height;
		fs << "cx" << calib.centerX;
		fs << "cy" << calib.centerY;
		fs << "fx" << calib.focal_lenght;
		fs << "fy" << calib.focal_lenght;
		fs << "rx" << calib.rotationX;
		fs << "ry" << calib.rotationY;
		fs << "rz" << calib.rotationZ;
		fs << "x" << calib.translationX;
		fs << "y" << calib.translationY;
		fs << "z" << calib.translationZ;
		fs.release();
	}

	{
		std::string tab = "\t";
		std::ofstream file_calib_txt;
		std::string filename = "calib_table_camera_" + std::to_string(camera) + ".txt";

		bool file_exists = boost::filesystem::exists(filename);

		file_calib_txt.open (filename, std::ios::app);
		if(!file_exists){
			file_calib_txt << "camera" << tab << "height" << tab << "width" << tab 
				<< "cx" << tab << "cy" << tab 
				<< "fx" << tab << "fy" << tab 
				<< "x" << tab << "y" << tab << "z" << tab 
				<< "rx" << tab << "ry" << tab << "rz" << std::endl;
		}

		file_calib_txt << camera << tab 
			<< height << tab << width << tab
			<< calib.centerX << tab << calib.centerY << tab 
			<< calib.focal_lenght << tab << calib.focal_lenght << tab
			<< calib.translationX << tab << calib.translationY << tab << calib.translationZ << tab 
			<< calib.rotationX << tab << calib.rotationY << tab << calib.rotationZ << std::endl;
		
		file_calib_txt.close();
	}

}

void load(cv::Mat &mat_x, cv::Mat &mat_y, std::string filename, CameraCalibration &calib){
	cv::FileStorage fs(filename.c_str(), cv::FileStorage::READ);
	fs["centerX"] >> calib.centerX;
	fs["centerY"] >> calib.centerY;
	fs["focal_lenghtX"] >> calib.focal_lenght;
	fs["focal_lenghtY"] >> calib.focal_lenght;
	fs["rotationX"] >> calib.rotationX;
	fs["rotationY"] >> calib.rotationY;
	fs["rotationZ"] >> calib.rotationZ;
	fs["translationX"] >> calib.translationX;
	fs["translationY"] >> calib.translationY;
	fs["translationZ"] >> calib.translationZ;
	fs["remap_x"] >> mat_x;
	fs["remap_y"] >> mat_y;
	fs.release();
}
void createCalibrationMaps(unsigned int h, unsigned int w, unsigned int camera, cv::Mat &morph_mat_x, cv::Mat &morph_mat_y, Ladybug &lady) { // Remap
	std::string filename = std::to_string(lady.caminfo.serialHead) + "_camera" + std::to_string(camera) + "_h" + std::to_string(h) +"_w" +  std::to_string(w);

	fillMap(h, w, morph_mat_x, morph_mat_y, lady, camera);
	//cv::imwrite(filenameX.c_str(), morph_mat_x);
	//cv::imwrite(filenameY.c_str(), morph_mat_y);

	//reduce map accurancy for speed up;
	cv::Mat reduced_map_x(morph_mat_x.size(), CV_16SC2 );
	cv::Mat reduced_map_y(morph_mat_y.size(), CV_16SC2 );

	cv::convertMaps(morph_mat_x, morph_mat_y, reduced_map_x, reduced_map_y, CV_16SC2);

	CameraCalibration cam;
	lady.getCameraCalibration(camera, &cam);

	save(reduced_map_x, reduced_map_y, filename.c_str(), cam, camera, h, w);
}

void main( int argc, char* argv[] ){   
	SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
	std::string status = "Init";
	double t_now = clock();
	//initConfig(0, argv);
	//std::cout << std::endl << "Number of Cores: " << boost::thread::hardware_concurrency() << std::endl;  
	printf("Starting ladybug calibration exporter...\n");

	Configuration config;
	if(argc < 1){
		printf("\nusage: path/to/config.ini rows(optinal) cols(optional)\n");
		return;
	}

	std::cout << "Loading configs from: " << argv[1] << std::endl;
	config.load(std::string(argv[1]));
	

	config.cfg_rectification = false;
	config.cfg_panoramic = false;
	//config.cfg_fileStream = "input/jpg8_ladybug-000000.pgr";
	config.cfg_ladybug_colorProcessing = LADYBUG_HQLINEAR;

	Ladybug lady;
	lady.init(&config, false);

	int cols = 1224;
	int rows = 1024;

	LadybugError error = LADYBUG_OK;
	if(argc == 3){
		double factor = (double)1224/1024;
		std::cout << "factor " << factor << std::endl;
		rows = atoi(argv[2]);
		cols = (double)rows * factor;
		std::cout << "Image size set to W: " << cols << " H: " << rows << std::endl;
	}
	else if(argc == 4){
		rows = atoi(argv[2]);
		cols = atoi(argv[3]);
		std::cout << "Image size set to W: " << cols << " H: " << rows << std::endl;
	}
	else{
		cols = 0;
		rows = 0;
	}

	_TIME
	lady.config->cfg_rectification = true;
	lady.initialised_processing = true;
	error = lady.initProcessing(cols, rows);
	
	status = "init procssing";
	_TIME

	//Get ladybug calibration
	CameraCalibration calibration;

	LadybugImage image;
	error = lady.grabImage(&image);

	_TIME
	double t_loopstart = t_now;
	std::string tab = "\t";
	
	std::cout << "camera" << tab << "w" << tab << "h" << tab << "f" << tab << "cx" << tab << "cy" << tab << "time" << std::endl;
	std::cout.flush();

	for(unsigned int camera = 0; camera < LADYBUG_NUM_CAMERAS; ++camera){

		//LadybugProcessedImage rectified; 
		//error = lady.grabProcessedImage(&rectified, (LadybugOutputImage) (1 << (6+camera)) );
		
		unsigned int h = lady.rectified_image_height; //rectified.uiRows;
		unsigned int w = lady.rectified_images_width; //rectified.uiCols;

		error = lady.getCameraCalibration(camera, &calibration);
		std::cout << camera << tab << w << tab << h << tab << calibration.focal_lenght << tab << calibration.centerX << tab << calibration.centerY << tab;

		//ArpBuffer* buffer = lady.getBuffer();
		//unsigned int data_size;

		//cv::Mat image_raw_RGB = cv::Mat( h, w, CV_8UC3, rectified.pData);
	
		//cv::Mat image_rectified_raw(rectified.uiRows, rectified.uiCols, CV_8UC3, rectified.pData);
		////cv::Mat image_raw_RGB;
		////cv::cvtColor(image_raw_RGBA, image_raw_RGB, CV_RGBA2RGB);
		//cv::Mat image_raw; 
		//cv::Size display_size(800,600);

		cv::Size size;
		size.height = h;
		size.width = w;

		cv::Mat imageUndistorted(size, CV_8UC3);
		cv::Mat morph_mat_x(h, w, CV_32FC1);
		cv::Mat morph_mat_y(h, w, CV_32FC1);

		createCalibrationMaps(h, w, camera, morph_mat_x, morph_mat_y, lady);

	/*	cv::remap(image_raw_RGB, imageUndistorted, morph_mat_x, morph_mat_y, CV_INTER_LINEAR);
		cv::resize(imageUndistorted, imageUndistorted, display_size); 

		cv::Mat diff;
		cv::resize(image_rectified_raw, image_rectified_raw, display_size);
		cv::absdiff( imageUndistorted, image_rectified_raw, diff );

		if(show_images){  
			cv::imshow("Rectified with map defaut cam" + std::to_string(camera), imageUndistorted); 
			cv::imshow("diff cam" + std::to_string(camera), diff); 
		}*/

		//imshow("teset",image_raw_RGB);
		
		status = "loop camera" + std::to_string(camera);
		_TIME

		/*cv::waitKey();
		Sleep(2000);
*/
	}
	status = "Total time in loop";
	t_now=t_loopstart;
	_TIME
		/*if(show_images){ 
			cv::waitKey();
		}*/
		//std::printf("<PRESS ANY KEY TO EXIT>");
		//_getch();

		return;
}