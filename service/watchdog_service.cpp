#include "stdafx.h"
#include "watchdog_service.h"


Watchdog_service::Watchdog_service(void)
{
	key_autostart = "watchdog.autostart";
	key_ip = "watchdog.socket";

	config.load("watchdog.ini");
	std::string key = config.get(key_autostart);
	std::string path = config.get_path(key);

	try{
		std::string args = config.get_args(key);
		active = MyProcess(path, args);
	}catch(std::exception e)
	{
		active = MyProcess(path);
	}

	zmq_service.init(config.get(key_ip).c_str(), ZMQ_REP);
}

void
Watchdog_service::loop(){
	while(true)
	{
		handle_zmq();
		check_processes();
		Sleep(2000);
	}
}

void 
Watchdog_service::handle_zmq(){
	try
	{
		ladybug5_network::pb_start_msg pb_msg;

		//Check for new messages
		if(zmq_service.receive(pb_msg, ZMQ_NOBLOCK) == false) return; // No message to handle if != 0
		
		//Got a message to handle

		printf("Received: %s\n", pb_msg.DebugString().c_str());
		handle_message(pb_msg);
		printf("handeling sending");

		//pb_msg.Clear();
	}
	catch(std::exception e){
		printf("Exception while msg handeling: %s\n", e.what());
	}
}

void 
Watchdog_service::check_processes(){

	if(	!active.is_alive()) active.start();

		//iterate over propperty tree and close all other services...
		/*boost::property_tree::ptree::const_iterator end = config.pt.end();
		for (boost::property_tree::ptree::const_iterator it = config.pt.begin(); it != end; ++it) {
			starter.stop_process(
			std::cout << it->first << ": " << it->second.get_value<std::string>() << std::endl;
				print(it->second);
    }*/
}

void 
Watchdog_service::handle_message(ladybug5_network::pb_start_msg &message){
	//Check if key exists in ptree
	std::string path;
	ladybug5_network::pb_reply reply_msg;
	ladybug5_network::pb_start_msg* req = new ladybug5_network::pb_start_msg(message);
	reply_msg.set_allocated_start(req);
	std::string message_name = message.name();

	try{
		path = config.get_path(message_name);
	}catch(std::exception e){
		path="";
	}

	if(path.empty()){ 
		reply_msg.set_success(false);
		std::string info_txt = "Path for key \"" + message.name() +"\" not found.\n\nAvailable in "+ config.config_file + ":\n" + config.to_string();
		reply_msg.set_info(info_txt);
		printf("Path for key \"%s\" not found", message.name() );
		
		zmq_service.send(reply_msg);
	}
	else{
		reply_msg.set_success(true);

		std::string info_txt = "Started: \""+ message.name() + "\" successfully.";
		reply_msg.set_info(info_txt);

		//stop active process
		active.stop();

		//update config to autostart it next time
		config.put(key_autostart, message_name);
		if( message.command() == ladybug5_network::DONTSAVE
			|| message.name() == "reboot"
			|| message.name() == "shutdown") 
			config.save();

		//activate new process
		std::string key = config.get(key_autostart);

		try{
			std::string args = config.get_args(key);
			active = MyProcess(path, args);
		}catch(std::exception e)
		{
			active = MyProcess(path);
		}

		//send back reply
		zmq_service.send(reply_msg, ZMQ_NOBLOCK);
	}
}

Watchdog_service::~Watchdog_service(){
	//stop active process
	active.stop();
}
