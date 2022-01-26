// test1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <signal.h>
#include <ShlObj.h>
#include <strsafe.h>

LPCTSTR WT_USER_PATH = "\\Appdata\\Local\\Microsoft\\WindowsApps\\wt.exe";
LPCTSTR WT_NAME = "WindowsTerminal.exe";
const int THREAD_HOTKEY_ID = 27;
const char DEBUG_FILE_LOCATION[] = "C:\\Users\\windows\\Desktop\\Debug-File.txt";

FILE *debugFile;
DWORD mainThreadId = NULL;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        printf("Ctrl-C event\n\n");
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        printf("Ctrl-Close event\n\n");
        return TRUE;

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        printf("Ctrl-Break event\n\n");
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        printf("Ctrl-Logoff event\n\n");
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        printf("Ctrl-Shutdown event\n\n");
        return FALSE;

    default:
        return FALSE;
    }
}

// A struct that will help us in the Enum Windows Callback function
// We will pass a pointer from the Find Main Window function to the callback function and the callback function will return when we find the desired window
struct HandleWindowsData {
    DWORD process_id;
    HWND window_handle;
};

//The callback function that will be called with the EnumWindows function
//It will be called more than one time and each time will get a different handle.
//This will be repeated until we find the window we desire or until all the windows opened in our OS are processed in this function
BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    HandleWindowsData* data = (HandleWindowsData*)lParam;
    DWORD process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data->process_id != process_id || !IsWindowVisible(handle))
        return TRUE;
    data->window_handle = handle;
    return FALSE;
}

//This is the function that finds the main window of a given process id
//It uses the EnumWindows function (from windows.h) in order to iterate through all the opened windows in the callback function (EnumWindowsCallback)
HWND FindMainWindow(DWORD process_id)
{
    HandleWindowsData data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(EnumWindowsCallback, (LPARAM)&data);
    return data.window_handle;
}


VOID StartupProcess(LPCTSTR lpApplicationName)
{
    // additional information
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    if (CreateProcessA(lpApplicationName,   // the path
        NULL,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    ))
    {
        fprintf(debugFile, "Process was created\n");
    }
    else
    {
        fprintf(debugFile, "Process was not created. Error code: %d\n", GetLastError());
    }
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

DWORD FindProcessId(const char* processname)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD result = NULL;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcessSnap)
        return(FALSE);

    pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);          // clean the snapshot object
        fprintf(debugFile, "!!! Failed to gather information on system processes! \n");
        return(NULL);
    }

    do
    {
        //printf("Checking process %s\n", pe32.szExeFile);
        if (0 == strcmp(processname, pe32.szExeFile))
        {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return result;
}

DWORD WINAPI HotkeyThread(LPVOID lpParam) {
    // Do stuff.  This will be the first function called on the new thread.
    // When this function returns, the thread goes away.  See MSDN for more details.

    if (RegisterHotKey(
        NULL,
        THREAD_HOTKEY_ID,
        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT,
        0x54))  //0x54 is 'T'
    {
        printf("Hotkey 'CTRL+ALT+T' registered, using MOD_NOREPEAT flag\n");
    }
    else
    {
        printf("Hotkey could not be registered\n");
    }

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            printf("WM_HOTKEY received\n");

            DWORD WindowsTerminalProcessId = FindProcessId("WindowsTerminal.exe");
            if (WindowsTerminalProcessId)
            {
                fprintf(debugFile, "There is a terminal open. Trying to bring it to foreground\n");

                HWND windowHandle = FindMainWindow(WindowsTerminalProcessId);

                /*
                if (IsWindowVisible(windowHandle))
                {
                    printf("Window is visible\n");
                }
                else
                {
                    printf("Window is not visible\n");
                }
                */

                if (IsIconic(windowHandle)) {

                    if (ShowWindow(windowHandle, SW_NORMAL))
                    {
                        fprintf(debugFile, "Windows Terminal was restored correctly to the foreground\n");
                    }
                    else
                    {
                        fprintf(debugFile, "Can not restore the window to the foreground\n");
                    }

                }
                else
                {
                    if (SetForegroundWindow(windowHandle))
                    {
                        fprintf(debugFile, "Windows Terminal was set correctly to the foreground\n");
                    }
                    else
                    {
                        fprintf(debugFile, "Can not bring the window to the foreground\n");
                    }

                }
            }
            else
            {
                fprintf(debugFile, "There is no terminal open. Trying to open one\n");
             //   StartupProcess(wtFinalPat);
            }

        }
        fflush(debugFile);
        if (msg.message == WM_CLOSE)
        {
            fprintf(debugFile, "WM_CLOSE received\n");
            break;

        }
        printf("\n");
    }

    if (UnregisterHotKey(
        NULL,
        THREAD_HOTKEY_ID
    ))
    {
        fprintf(debugFile, "Hotkey 'CTRL+ALT+T' unregistered\n");
    }
    else
    {
        fprintf(debugFile, "Hotkey could not be unregistered\n");
    }

    return 0;
}


int _tmain(int argc, TCHAR* argv[])
{
    DWORD callResult = NULL;
    debugFile = fopen(DEBUG_FILE_LOCATION, "w");

    PWSTR userDir;
    callResult = SHGetKnownFolderPath(FOLDERID_Profile, NULL, NULL, &userDir);
    if (callResult == E_FAIL)
    {
        fprintf(stdout, "SHGetKnownFolderPath failed\n");
    }
    else if (callResult == E_INVALIDARG)
    {
        fprintf(stdout, "SHGetKnownFolderPath has got an invalid argument\n");
    }
    else
    {
        fprintf(stdout, "SHGetKnownFolderPath worked as it should\n");
    }

    TCHAR wtFinalPath[200];

    //Copying the obtained user dir to wtFinalPath
    DWORD userDirLength = wcslen(userDir);
    for (int i = 0; i < userDirLength; i++)
    {
        wtFinalPath[i] = userDir[i];
    }
    wtFinalPath[userDirLength] = 0;

    //Concating the location of WindowsTerminal to the user directory
    callResult = StringCchCatA(wtFinalPath, 200, WT_USER_PATH);
    switch (callResult)
    {
    case S_OK:
        fprintf(stdout, "WT_PATH_PART2 was concatened successfully\n");
        break;

    case STRSAFE_E_INVALID_PARAMETER:
        fprintf(stdout, "StringCchCatA received an invalid parameter\n");
        break;

    case STRSAFE_E_INSUFFICIENT_BUFFER:
        fprintf(stdout, "The memory location that should contain the string after using StringCchCatA is not large enough\n");
        break;

    default:
        fprintf(stdout, "Something went wrong with StringCchCatA function\n");
        break;
    }

    //Clearing the memory used by userDir, as it specifies in the documentation
    CoTaskMemFree(userDir);

    callResult = SetConsoleCtrlHandler(CtrlHandler, TRUE);
    if (callResult)
    {
        fprintf(stdout, "The setConsoleCtrlHandler worked as it should\n");
    }
    else
    {
        fprintf(stdout, "The setConsoleCtrlHandler did not work\n");
    }

    /*
    HANDLE thread = CreateThread(NULL, 0, HotkeyThread, NULL, 0, NULL);
    if (thread)
    {
        fprintf(debugFile, "Thread was created successfully\n");
        WaitForSingleObject(thread, INFINITE);
    }
    else
    {
        fprintf(debugFile, "Thread could not be created\n");
    }

    fclose(debugFile);

    */

    // Do stuff.  This will be the first function called on the new thread.
// When this function returns, the thread goes away.  See MSDN for more details.

    callResult = RegisterHotKey(NULL, THREAD_HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 0x54);  //0x54 is 'T'
    if (callResult)
    {
        fprintf(stdout, "Hotkey 'CTRL+ALT+T' registered, using MOD_NOREPEAT flag\n");
    }
    else
    {
        fprintf(stdout, "Hotkey could not be registered\n");
    }

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            fprintf(debugFile, "WM_HOTKEY received\n");

            DWORD WindowsTerminalProcessId = FindProcessId(WT_NAME);
            if (WindowsTerminalProcessId)
            {
                fprintf(debugFile, "There is a terminal open. Trying to bring it to foreground\n");

                HWND windowHandle = FindMainWindow(WindowsTerminalProcessId);

                /*
                if (IsWindowVisible(windowHandle))
                {
                    printf("Window is visible\n");
                }
                else
                {
                    printf("Window is not visible\n");
                }
                */

                if (IsIconic(windowHandle)) {

                    callResult = ShowWindow(windowHandle, SW_NORMAL);
                    if (callResult)
                    {
                        fprintf(debugFile, "Windows Terminal was restored correctly from the taskbar to the foreground\n");
                    }
                    else
                    {
                        fprintf(debugFile, "Can not restore the window from the taskbar to the foreground\n");
                    }

                }
                else
                {
                    callResult = SetForegroundWindow(windowHandle);
                    if (callResult)
                    {
                        fprintf(debugFile, "Windows Terminal was set correctly to the foreground\n");
                    }
                    else
                    {
                        fprintf(debugFile, "Can not bring the window to the foreground\n");
                    }

                }
            }
            else
            {
                fprintf(debugFile, "There is no terminal open. Trying to open one\n");
                StartupProcess(wtFinalPath);
            }

        }
        fflush(debugFile);

        if (msg.message == WM_CLOSE)
        {
            fprintf(debugFile, "WM_CLOSE received\n");
            break;
        }
        if (msg.message == WM_QUIT)
        {
            fprintf(debugFile, "WM_QUIT received\n");
            break;
        }

        printf("\n");
    }

    if (UnregisterHotKey(NULL, THREAD_HOTKEY_ID))
    {
        fprintf(debugFile, "Hotkey 'CTRL+ALT+T' unregistered\n");
    }
    else
    {
        fprintf(debugFile, "Hotkey could not be unregistered\n");
    }


    fclose(debugFile);

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
