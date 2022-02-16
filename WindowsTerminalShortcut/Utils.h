#pragma once

// Here comes the custom defined error. The errors that start with 4(in hexa) are error codes completely made by us. 
// The errors that start with 6(in hexa) are error codes defined by us, but more information can be retrieved if we call GetLastError one more time.
// Example:
/*
    lastError = GetLastError();
    if ((lastError>>29) == 1 && (lastError >> 28) == 1) //it means it starts with 6 in hexa
    {
        errorDetails = GetLastError();
        ....
    }
*/

#define NO_MAIN_WINDOW 0x40000001
#define CANT_CREATE_PROCESS_SNAPSHOT 0x60000001
#define CANT_GET_FIRST_PROCESS 0x60000002
#define CANT_GET_PROCESS_ID_OF_WINDOW 0x60000004
#define CANT_ENUMERATE_PROCESSES 0x60000008


// This is the function that finds the HWND of the main window of a given process id
// It uses the EnumWindows function (from windows.h) in order to iterate through all the opened windows in the callback function (EnumWindowsCallbackPID)
// It returns the HWND of the given process
HWND FindMainWindowPID(DWORD process_id);

HWND FindMainWindowAUMID(PWSTR AUMID);

// Starts the app at the specified path
BOOL StartupProcess(PWSTR lpApplicationPath);


// This function will return the processId of the first process found that has the given name
// If the process does not exist, it will return NULL and will set the last error to 0
// If the process exists, it will return the process id, but the last error will NOT be set to 0.
// If there is an error, it will return NULL, but the last error will not be set to 0, it will be set according to the error description
DWORD FindProcessIdByName(PWSTR processname);

/*
DWORD FindProcessIdByAUMID(PWSTR AUMID);
*/