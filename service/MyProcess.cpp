#include "MyProcess.h"

using namespace System;
using namespace System::Diagnostics;
using namespace System::ComponentModel;


MyProcess::MyProcess(){
}

MyProcess::MyProcess(std::string executable, std::string arguments)
{
    path = new TCHAR[executable.length() + 1];      
    strncpy( path, executable.c_str(), executable.length()+1);  

	name = new TCHAR[executable.length() + 1];      
    strncpy( name, executable.c_str(), executable.length()+1);

	if(arguments.empty()){
		args = new TCHAR[arguments.length() + executable.length() +1 ];
		strncpy( args, executable.c_str(), executable.length()+1);
	}
	else{
		args = new TCHAR[arguments.length() + executable.length() +2 ];
		std::string space =" ";
		strncpy( args, executable.c_str(), executable.length());
		strncpy( args + executable.length(),   space.c_str(), space.length());
		strncpy( args + executable.length()+1, arguments.c_str(), arguments.length()+1);
	}

	name = PathFindFileName(name);
	PathRemoveExtension(name);
	id_thread = 0;
	id_process = 0;

	printf("Process: %s\npath: %s, args: %s\npath: %s, args: %s.\n", 
			name, 
			executable.c_str(), arguments.c_str(), 
			path, args);
}

bool 
MyProcess::is_alive()
{
	HANDLE processH = OpenProcess(SYNCHRONIZE, FALSE, id_process);
	DWORD ret = WaitForSingleObject(processH, 0);
	CloseHandle(processH);
	return ret == WAIT_TIMEOUT;
}


/* 
* in: Process to find the id
* out: true if processname found
*/
bool 
MyProcess::find(){
	
	String^ file;
	file = gcnew String(name);

	// Get all instances of Notepad running on the local 
	// computer. 
	array<Process^>^localByName = Process::GetProcessesByName( file );
	
	for each (Process^ P in localByName){
		id_process = P->Id;
		return true;
	}
	return false;
}

bool 
MyProcess::is_running()
{
	String^ file;
	file = gcnew String(name);

	// Get all instances of process running on the local computer. 
	array<Process^>^localByName = Process::GetProcessesByName( file );
	
	if( localByName->Length > 0) return true;
	return false;
}

bool
MyProcess::start()
{
	printf("Starting process: %s, args: %s.\n", path, args);

	PROCESS_INFORMATION pi = {0};
	STARTUPINFO startup_info = {0};

	if ( !CreateProcess(path, args, 0, FALSE, 0, 0, 0, 0, &startup_info, &pi) )
	{
		printf("CreateProcess failed ( %s ).\n", GetLastError() );
	
		return false;
	}
	else
	{
		id_thread = pi.dwThreadId;
		id_process = pi.dwProcessId;

		WaitForSingleObject(pi.hProcess, 5 * 1000);
		
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		return is_alive();
	}
}

bool
MyProcess::stop()
{
	String^ file;
	file = gcnew String(name);
	
	//Try to stop the thread softly
	if ( PostThreadMessage(id_thread, WM_QUIT, 0, 0) ) // Good
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

MyProcess::~MyProcess(){

#ifndef NDEBUG
	printf("Deleting MyProcess %s, %s\n", name, path);
#endif // !NDEBUG
	//TODO: Fixme (Gets called direct after instanziation)
	/*delete[] path;
	delete[] name;*/
}
