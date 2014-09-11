#pragma once
#include "MyProcess.h"

//#using <System.dll>

class Starter
{
public:
	Starter(void);
	bool start_process(MyProcess&);
	bool stop_process(MyProcess&);
	bool find_process(MyProcess &process);
	bool is_running(MyProcess&);
	bool is_process_alive(MyProcess&);
	~Starter(void);
};