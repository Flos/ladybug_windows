#include "config.h"

Config::Config(void)
{
}

void
Config::load(std::string filename){
	config_file = filename;
	try
	{
		boost::property_tree::ini_parser::read_ini(config_file, pt);
	}
	catch (boost::exception &e){
		printf("Ini parsing failed: %s\nCreating new ini file.", e);
		create_default();
	}
}

void
Config::save(){
	boost::property_tree::write_ini(config_file, pt);
}

void 
Config::put_path(std::string key, std::string path){
	pt.put(get_sub_key(key,"PATH"), path);
}

void 
Config::put_args(std::string key, std::string args){
	pt.put(get_sub_key(key,"ARGS"), args);
}

void 
Config::put(std::string key, std::string value){
	pt.put(key, value);
}

std::string
Config::get_path(std::string key){
	return pt.get<std::string>(get_sub_key(key,"PATH")); 
}

std::string
Config::get_args(std::string key){
	return pt.get<std::string>(get_sub_key(key,"ARGS"));;
}

std::string
Config::get(std::string key){
	return pt.get<std::string>(key); 
}

std::string
Config::get_sub_key(std::string key, std::string sub_key){
	return key + "." + sub_key;
}

std::string
Config::to_string(){
	std::stringstream ss;
	boost::property_tree::write_json(ss, pt);
	return ss.str();
}

void
Config::create_default(){
	
	put("watchdog.socket","tcp://*:28883");
	put("watchdog.autostart","jpg_raw");	

	put_path("jpg_raw","..\\bin\\grabber_x64_Release.exe");
	put_path("panoramic","..\\bin\\panoramic_x64_Release.exe");
	put_path("calibration","..\\bin\\export_distCoeffs_x64_Release.exe");
	put_path("full_processing","..\\bin\\full_processing_x64_Release.exe");

	put_path("debug_jpg_raw","..\\bin\\grabber_x64_Debug.exe");
	put_path("debug_panoramic","..\\bin\\panoramic_x64_Debug.exe");
	put_path("debug_calibration","..\\bin\\export_distCoeffs_x64_Debug.exe");
	put_path("debug_full_processing","..\\bin\\full_processing_x64_Debug.exe");
	
	put_path("shutdown","C:\\Windows\\System32\\shutdown.exe");
	put_args("shutdown","/s");
	put_path("restart","C:\\Windows\\System32\\shutdown.exe");
	put_args("restart","/r");
	save();
}