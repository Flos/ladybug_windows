#pragma once
#include "stdafx.h"
#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>

class MyProcess
{
public:
	MyProcess();
	MyProcess(std::string executable, std::string arguments="");
	~MyProcess();

	bool start();
	bool stop();
	bool find();
	bool is_running();
	bool is_alive();
private:
	TCHAR* path;
	TCHAR* name;
	TCHAR* args;
	DWORD id_process;
	DWORD id_thread;
};

