#pragma once
#include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>

class Config
{
public:
	Config(void);
	void load(std::string filename="watchdog.ini");
	void create_default();
    void save();

	void put_path(std::string key, std::string path);
	void put_args(std::string key, std::string args);
	void put(std::string key, std::string value);
	
	std::string get_path(std::string key);
	std::string get_args(std::string key);
	std::string get(std::string key);

	boost::property_tree::ptree pt;
	std::string config_file;
	std::string to_string();
private:
	std::string get_sub_key(std::string key, std::string sub_key);
};

