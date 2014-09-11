#pragma once

class Watchdog_service
{
public:
	Watchdog_service(void);
	void loop(void);
	void handle_zmq();
	void check_processes();	
	~Watchdog_service();
private:
	std::string key_autostart;
	std::string key_ip;
	Config config;
	MyProcess active;
	Zmq_service zmq_service;
	void handle_message(ladybug5_network::pb_start_msg &message);
};