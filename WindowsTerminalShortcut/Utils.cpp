#include <Windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <appmodel.h>
#include "Utils.h"


const DWORD AUMID_BUFFER_LENGTH = 100;

// A struct that will help us in the Enum Windows Callback function (the one used with AUMID and the one used with PID)
// We will pass a pointer from the Find Main Window function (the one used with AUMID and the one used with PID) to the callback function and the callback function will return when we find the desired window
struct HandleWindowsData {
	DWORD process_id;
	HWND window_handle;
	PWSTR AUMID;
};


// The callback function that will be called with the EnumWindows function called in the FindMainWindowPid function
// It will be called more than one time and each time will get a different handle.
// This will be repeated until we find the window we desire or until all the windows opened in our OS are processed in this function
// We should return false when we want to stop the iteration through the windows.
// If all the windows are iterated and we didnt return FALSE in an iteration, this function will return TRUE.
BOOL CALLBACK EnumWindowsCallbackPID(HWND handle, LPARAM lParam)
{
	DWORD GetWindowThreadProcessIdResult = NULL;

	HandleWindowsData* data = (HandleWindowsData*)lParam;
	DWORD process_id = NULL;
	GetWindowThreadProcessIdResult = GetWindowThreadProcessId(handle, &process_id);

	if (GetWindowThreadProcessIdResult == NULL)
	{
		SetLastError(CANT_GET_PROCESS_ID_OF_WINDOW);
		return FALSE;
	}

	if (data->process_id != process_id || !IsWindowVisible(handle))
		return TRUE;

	data->window_handle = handle;
	return FALSE;
}

// This is the function that finds the HWND of the main window of a given process id
// It uses the EnumWindows function (from windows.h) in order to iterate through all the opened windows in the callback function (EnumWindowsCallbackPID)
// It returns the HWND of the given window
// If there is an error the function returns NULL and to get extended error information, call GetLastError.
// If the function succeeds and there is no main windows for that process, the function returns NULL and GetLastError will return NO_MAIN_WINDOW
// GetLastError should be called if the functions returns NULL in order to clear the thread's error stack.
HWND FindMainWindowPID(DWORD process_id)
{
	BOOL EnumWindowsResult = NULL;

	HandleWindowsData data;
	data.process_id = process_id;
	data.window_handle = NULL;
	data.AUMID = NULL;
	EnumWindowsResult = EnumWindows(EnumWindowsCallbackPID, (LPARAM)&data);

	if (EnumWindowsResult == TRUE && data.window_handle == 0)
	{
		SetLastError(NO_MAIN_WINDOW);
	}

	return data.window_handle;
}


// The callback function that will be called with the EnumWindows function called in the FindMainWindowAUMID function
// It will be called more than one time and each time will get a different handle.
// This will be repeated until we find the window we desire or until all the windows opened in our OS are processed in this function
// We should return false when we want to stop the iteration through the windows.
// If all the windows are iterated and we didnt return FALSE in an iteration, this function will return TRUE.
BOOL CALLBACK EnumWindowsCallbackAUMID(HWND handle, LPARAM lParam)
{
	DWORD GetWindowThreadProcessIdResult = NULL;

	HandleWindowsData* data = (HandleWindowsData*)lParam;
	DWORD process_id = NULL;
	GetWindowThreadProcessIdResult = GetWindowThreadProcessId(handle, &process_id);

	// If we didint get the process id from the window handle, we should stop the iteration(return FALSE)
	// and set the specific error
	if (GetWindowThreadProcessIdResult == NULL)
	{
		SetLastError(CANT_GET_PROCESS_ID_OF_WINDOW);
		return FALSE;
	}

	// If window is not visible, we should get to the next element of the iteration(return TRUE)
	if (!IsWindowVisible(handle))
		return TRUE;

	// If we cant OpenProcess, we should get to the next element of the iteration and clear the last element on the error stack 
	HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, process_id);
	if (processHandle == NULL)
	{
		DWORD toClearErrorStack = GetLastError();
		return TRUE;
	}

	// Trying to get the AUMID in the returnBuffer
	UINT32 returnBufferLength = AUMID_BUFFER_LENGTH;
	WCHAR returnBuffer[AUMID_BUFFER_LENGTH];
	DWORD callResult = GetApplicationUserModelId(processHandle, &returnBufferLength, returnBuffer);

	// If there was an error when trying to get the AUMID, we should get to the next iteration(possibly because that process does not have an AUMID
	// or we dont have enough rights to access that process).
	if (callResult != ERROR_SUCCESS)
	{
		return TRUE;
	}

	// If it is not the AUMID we were looking for, we should get to the next iteration(return TRUE)
	if (wcscmp(returnBuffer, data->AUMID) != ERROR_SUCCESS)
	{
		return TRUE;
	}

	// Here we know for sure we have found the process with the given AUMID(otherwise we will have returned TRUE)
	// We copy the window handle to the date structure and we exit the iteration(return FALSE)
	data->window_handle = handle;
	return FALSE;
}


// This is the function that finds the HWND of the main window of a given process id
// It uses the EnumWindows function (from windows.h) in order to iterate through all the opened windows in the callback function (EnumWindowsCallbackAUMID)
// It returns the HWND of the given window
// If there is an error the function returns NULL and to get extended error information, call GetLastError.
// If the function succeeds and there is no main windows for that process, the function returns NULL and GetLastError will return NO_MAIN_WINDOW
// GetLastError should be called if the functions returns NULL in order to clear the thread's error stack.
HWND FindMainWindowAUMID(PWSTR AUMID)
{
	BOOL EnumWindowsResult = NULL;

	HandleWindowsData data;
	data.process_id = NULL;
	data.window_handle = NULL;
	data.AUMID = AUMID;
	EnumWindowsResult = EnumWindows(EnumWindowsCallbackAUMID, (LPARAM)&data);

	if (EnumWindowsResult == TRUE && data.window_handle == 0)
	{
		SetLastError(NO_MAIN_WINDOW);
	}

	return data.window_handle;
}

// Starts the app at the specified path
// If the function fails, it returns zero and extended error information can be retrieved using GetLastError
BOOL StartupProcess(PWSTR lpApplicationPath)
{
	BOOL result = FALSE;

	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	result = CreateProcessW(lpApplicationPath,   // the path
		NULL,           // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return result;
}


// This function will return the processId of the first process found that has the given name
// If the process does not exist, it will return NULL and will set the last error to 0
// If the process exists, it will return the process id, but the last error will NOT be set to 0.
// If there is an error, it will return NULL, but the last error will not be set to 0, it will be set according to the error description
DWORD FindProcessIdByName(PWSTR processname)
{
	DWORD result = NULL;

	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	ZeroMemory(&pe32, sizeof(pe32));

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		SetLastError(CANT_CREATE_PROCESS_SNAPSHOT);
		return NULL;
	}


	pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);          // clean the snapshot object
		SetLastError(CANT_GET_FIRST_PROCESS);
		return NULL;
	}

	do
	{
		//printf("Checking process %s\n", pe32.szExeFile);
		if (0 == wcscmp(processname, pe32.szExeFile))
		{
			result = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	if (result == NULL)
	{
		SetLastError(ERROR_SUCCESS);
	}

	CloseHandle(hProcessSnap);

	return result;
}


// This function uses a given AUMID in order to find the first process which has the given AUMID.
// The PID of the first process with the given AUMID is returned
// If there is an error, then the function will return 0 and then the function GetLastError() can be called in order to get more information about the error
// If there is not an process with the given AUMID, then the function will return 0 and ERROR_SUCCESS will be putted on the thread error stack
// (should call GetLastError() in order to clear the error stack and to be sure that the function was succesful).

/*
DWORD FindProcessIdByAUMID(PWSTR AUMID)
{
	DWORD result = NULL;
	DWORD callResult = NULL;

	const DWORD ARRAY_INITIAL_SIZE = 30000;
	DWORD processesArray[ARRAY_INITIAL_SIZE];
	DWORD processesNo = 0;

	// Enumerating the processes in the processesArray
	callResult = EnumProcesses(processesArray, ARRAY_INITIAL_SIZE * sizeof(int), &processesNo);
	processesNo /= sizeof(int);

	if (!callResult)
	{
		SetLastError(CANT_ENUMERATE_PROCESSES);
		return NULL;
	}

	DWORD lengthProcessAUMID = wcslen(AUMID);
	WCHAR* processAUMID = new WCHAR[lengthProcessAUMID + 1];
	for (int i = 0; i < processesNo; i++)
	{
		DWORD currProcessId = processesArray[i];
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, TRUE, processesArray[i]);

		if (processHandle)
		{
			lengthProcessAUMID = wcslen(AUMID) + 1;
			callResult = GetApplicationUserModelId(processHandle, (UINT32*)&lengthProcessAUMID, processAUMID);

			if (callResult == ERROR_SUCCESS && wcscmp(AUMID, processAUMID) == 0)
			{
				result = processesArray[i];
			}
		}
		else
		{
			callResult = GetLastError();
		}

	}

	if (result == NULL)
	{
		SetLastError(ERROR_SUCCESS);
	}

	return result;
}
*/