// test1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <tchar.h>
#include <ShlObj.h>
#include <strsafe.h>

#include "Utils.h"
#include "CustomStringConversion.h"

const PSTR WT_APPDATA_PATH = (PSTR)"\\Microsoft\\WindowsApps\\wt.exe";
const DWORD WT_APPDATA_PATH_LENGTH = 32;

const PSTR WT_NAME = (PSTR)"WindowsTerminal.exe";
const PWSTR AUMID = (PWSTR)L"Tudor3510.WindowsTerminalShortcut.385bds23";
const int THREAD_HOTKEY_ID = 27;
const char DEBUG_FILE_LOCATION[] = "C:\\Users\\windows\\Desktop\\Debug-File.txt";

FILE* debugFile;

int _tmain(int argc, TCHAR* argv[])
{
    DWORD callResult;
    debugFile = fopen(DEBUG_FILE_LOCATION, "w");


    // Making sure that the app is not running.
    callResult = FindProcessIdByAUMID(AUMID);
    if (callResult)
    {
        callResult = MessageBox(NULL, "The app is already running", "Error", MB_OK);
        return 0;
    }

    if (callResult == NULL && GetLastError() != ERROR_SUCCESS)
    {
        callResult = MessageBox(NULL, "Failed to startup the app", "Error", MB_OK);
        return 0;
    }

    callResult = SetCurrentProcessExplicitAppUserModelID(AUMID);
    if (callResult == S_OK)
    {
        fprintf(debugFile, "The AUMID was set correctly\n");
    }

    PWSTR userDir = NULL;
    callResult = SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, NULL, &userDir);
    if (callResult == E_FAIL)
    {
        fprintf(debugFile, "SHGetKnownFolderPath failed\n");
    }
    else if (callResult == E_INVALIDARG)
    {
        fprintf(debugFile, "SHGetKnownFolderPath has got an invalid argument\n");
    }
    else
    {
        fprintf(debugFile, "SHGetKnownFolderPath worked as it should\n");
    }

    //Copying the obtained user dir to wtFinalPath
    LPSTR wtFinalPath = wcharToChar(userDir);

    //Concating the location of WindowsTerminal to the user directory
    callResult = StringCchCatA(wtFinalPath, wcslen(userDir) + WT_APPDATA_PATH_LENGTH + 1, WT_APPDATA_PATH);
    switch (callResult)
    {
    case S_OK:
        fprintf(debugFile, "WT_PATH_PART2 was concatened successfully\n");
        break;

    case STRSAFE_E_INVALID_PARAMETER:
        fprintf(debugFile, "StringCchCatA received an invalid parameter\n");
        break;

    case STRSAFE_E_INSUFFICIENT_BUFFER:
        fprintf(debugFile, "The memory location that should contain the string after using StringCchCatA is not large enough\n");
        break;

    default:
        fprintf(debugFile, "Something went wrong with StringCchCatA function\n");
        break;
    }

    //Clearing the memory used by userDir, as it specifies in the documentation
    CoTaskMemFree(userDir);

    callResult = RegisterHotKey(NULL, THREAD_HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 0x54);  //0x54 is 'T'
    if (callResult)
    {
        fprintf(debugFile, "Hotkey 'CTRL+ALT+T' registered, using MOD_NOREPEAT flag\n");
    }
    else
    {
        fprintf(debugFile, "Hotkey could not be registered\n");
    }


    fprintf(debugFile, "\n");
    BOOL shouldContinue = TRUE;
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0 && shouldContinue)
    {
        switch (msg.message)
        {
        case WM_HOTKEY:
        {
            fprintf(debugFile, "WM_HOTKEY received\n");

            DWORD WindowsTerminalProcessId = FindProcessIdByName(WT_NAME);
            switch (WindowsTerminalProcessId)
            {
            case 0:
                if (GetLastError() == ERROR_SUCCESS)
                {
                    fprintf(debugFile, "There is no terminal open. Trying to open one\n");
                    StartupProcess(wtFinalPath);
                }

                break;

            default:
                fprintf(debugFile, "There is a terminal open. Trying to bring it to foreground\n");

                HWND windowHandle = FindMainWindow(WindowsTerminalProcessId);

                if (windowHandle)
                {
                    if (IsIconic(windowHandle))
                    {

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
                break;
            }
        }
            break;

        case WM_CLOSE:
        {
            fprintf(debugFile, "WM_CLOSE received\n");
            shouldContinue = FALSE;
            break;
        }

        case WM_QUIT:
        {
            fprintf(debugFile, "WM_QUIT received\n");
            shouldContinue = FALSE;
            break;
        }
        }
        fflush(debugFile);

        fprintf(debugFile, "\n");
    }

    if (UnregisterHotKey(NULL, THREAD_HOTKEY_ID))
    {
        fprintf(debugFile, "Hotkey 'CTRL+ALT+T' unregistered\n");
    }
    else
    {
        fprintf(debugFile, "Hotkey could not be unregistered\n");
    }


    delete[] wtFinalPath;
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
