
#include "Starter.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::ComponentModel;

Starter::Starter(void)
{
}


Starter::~Starter(void)
{
}

bool 
Starter::is_process_alive(MyProcess &process)
{
	HANDLE processH = OpenProcess(SYNCHRONIZE, FALSE, process.id_process);
	DWORD ret = WaitForSingleObject(processH, 0);
	CloseHandle(processH);
	return ret == WAIT_TIMEOUT;
}


/* 
* in: Process to find the id
* out: true if processname found
*/
bool 
Starter::find_process(MyProcess &process){
	
	String^ file;
	file = gcnew String(process.name);

	// Get all instances of Notepad running on the local 
	// computer. 
	array<Process^>^localByName = Process::GetProcessesByName( file );
	
	for each (Process^ P in localByName){
		process.id_process = P->Id;
		return true;
	}
	return false;
}

bool 
Starter::is_running(MyProcess &process)
{
	String^ file;
	file = gcnew String(process.name);


	// Get all instances of process running on the local computer. 
	array<Process^>^localByName = Process::GetProcessesByName( file );
	
	if( localByName->Length > 0) return true;
	return false;
}

bool
Starter::start_process(MyProcess &process)
{
	PROCESS_INFORMATION pi;
	if ( !CreateProcess(process.path, 0, 0, FALSE, 0, 0, 0, 0, &process.startup_info, &pi) )
	{
		printf("CreateProcess failed ( %s ).\n", GetLastError() );
	
		return false;
	}
	else
	{
		process.id_thread = pi.dwThreadId;
		process.id_process = pi.dwProcessId;

		WaitForSingleObject(pi.hProcess, 5 * 1000);
		
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		return is_process_alive(process);
	}
}

bool
Starter::stop_process(MyProcess &process)
{
	String^ file;
	file = gcnew String(process.name);
	
	//Try to stop the thread softly
	if ( PostThreadMessage(process.id_thread, WM_QUIT, 0, 0) ) // Good
	{
		printf("\nRequest to terminate process has been sent to %s!\n", file);
	}
	Sleep(1000);

	// Get all instances of the process running on the local computer. 
	array<Process^>^localByName = Process::GetProcessesByName( file );
	
	try{
		for each (Process^ P in localByName){

#ifndef NDEBUG
			printf("Process: %s\n", P->ProcessName);
#endif

			if (P->ProcessName->Equals(file))
			{
				SetLastError(0);
				HANDLE processH = OpenProcess(PROCESS_TERMINATE, FALSE, P->Id);
				printf("Opening process error? %d\n", GetLastError());
				if ( TerminateProcess(processH, 0) ) {// Evil
					printf("Process terminated!\n");
				}
				else{
					printf("unable to terminate! %d\n", GetLastError());
				}
				CloseHandle(processH);
			}
		}
	}catch( std::exception e){
		printf("Exception %s\n", e.what() );
	}

	return true;
}