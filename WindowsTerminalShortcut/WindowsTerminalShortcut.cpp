// test1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <tchar.h>
#include <ShlObj.h>
#include <strsafe.h>
#include "Utils.h"

const LPCTSTR WT_APPDATA_PATH = "\\Microsoft\\WindowsApps\\wt.exe";
const DWORD WT_APPDATA_PATH_LENGTH = 32;

const LPCTSTR WT_NAME = "WindowsTerminal.exe";
const int THREAD_HOTKEY_ID = 27;
const char DEBUG_FILE_LOCATION[] = "C:\\Users\\windows\\Desktop\\Debug-File.txt";

FILE* debugFile;

int _tmain(int argc, TCHAR* argv[])
{
    DWORD callResult = NULL;
    debugFile = fopen(DEBUG_FILE_LOCATION, "w");

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
    DWORD userDirLength = wcslen(userDir);
    TCHAR *wtFinalPath = new TCHAR[userDirLength + WT_APPDATA_PATH_LENGTH + 1];
    for (int i = 0; i < userDirLength; i++)
    {
        wtFinalPath[i] = userDir[i];
    }
    wtFinalPath[userDirLength] = 0;

    //Clearing the memory used by userDir, as it specifies in the documentation
    CoTaskMemFree(userDir);

    //Concating the location of WindowsTerminal to the user directory
    callResult = StringCchCatA(wtFinalPath, userDirLength + WT_APPDATA_PATH_LENGTH + 1, WT_APPDATA_PATH);
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

            DWORD WindowsTerminalProcessId = FindProcessId(WT_NAME);
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
