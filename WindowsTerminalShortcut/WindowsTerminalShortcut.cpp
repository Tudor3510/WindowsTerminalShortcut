// test1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <tchar.h>
#include <ShlObj.h>
#include <strsafe.h>
#include <appmodel.h>

#include "Utils.h"
#include "CustomStringConversion.h"

const PWSTR WT_APPDATA_PATH = (PWSTR)L"\\Microsoft\\WindowsApps\\wt.exe";
const DWORD WT_APPDATA_PATH_LENGTH = 32;
const DWORD WT_FINAL_PATH_BUFFER_LENGTH = 150;

const PWSTR WT_NAME = (PWSTR)L"WindowsTerminal.exe";
const PWSTR AUMID = (PWSTR)L"Tudor3510.WindowsTerminalShortcut.385bds23";
const PWSTR WT_AUMID = (PWSTR)L"Microsoft.WindowsTerminal_8wekyb3d8bbwe!App";
const PWSTR MUTEX_NAME = (PWSTR)L"WindowsTerminalShortcut.385bds23";
const int THREAD_HOTKEY_ID = 27;
const char DEBUG_FILE_LOCATION[] = "C:\\Users\\windows\\Desktop\\Debug-File.txt";

FILE* debugFile;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	DWORD callResult;

#ifdef _DEBUG
	debugFile = fopen(DEBUG_FILE_LOCATION, "w");
#endif

	// Creating a mutex so that we can know if the app is already running
	HANDLE mutexHandle = CreateMutexW(NULL, TRUE, MUTEX_NAME);
	callResult = GetLastError();

	// Verify if we receive a valid handle to the mutex
	if (mutexHandle == NULL)
	{
		callResult = MessageBox(NULL, L"Failed to set the app identity using mutex", L"Error", MB_OK);
		return 0;
	}

	// Verify if the mutex was already created
	if (callResult != ERROR_SUCCESS)
	{
		callResult = MessageBox(NULL, L"The app is already running", L"Error", MB_OK);
		CloseHandle(mutexHandle);
		return 0;
	}

	PWSTR userDir = NULL;
	callResult = SHGetKnownFolderPath(FOLDERID_LocalAppData, NULL, NULL, &userDir);
	if (callResult == E_FAIL)
	{
		callResult = MessageBox(NULL, L"SHGetKnownFolderPath failed", L"Error", MB_OK);
		return 0;
	}
	else if (callResult == E_INVALIDARG)
	{
		callResult = MessageBox(NULL, L"SHGetKnownFolderPath has got an invalid argument", L"Error", MB_OK);
		return 0;
	}

	//Copying the obtained user dir to wtFinalPath
	PWSTR wtFinalPath = new WCHAR[WT_FINAL_PATH_BUFFER_LENGTH];

	callResult = wcscpy_s(wtFinalPath, WT_FINAL_PATH_BUFFER_LENGTH, userDir);
	if (callResult != S_OK)
	{
		callResult = MessageBox(NULL, L"Could not copy userDir to wtFinalPath", L"Error", MB_OK);
		return 0;
	}

	//Concating the location of WindowsTerminal to the user directory
	callResult = wcscat_s(wtFinalPath, WT_FINAL_PATH_BUFFER_LENGTH, WT_APPDATA_PATH);
	if (callResult != S_OK)
	{
		callResult = MessageBox(NULL, L"Could not concatenate the WT_APPDATA_PATH to userDir", L"Error", MB_OK);
		return 0;
	}

	//Clearing the memory used by userDir, as it specifies in the documentation
	CoTaskMemFree(userDir);

	callResult = RegisterHotKey(NULL, THREAD_HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 0x54);  //0x54 is 'T'
	if (!callResult)
	{
		callResult = MessageBox(NULL, L"Hotkey could not be registered", L"Error", MB_OK);
		return 0;
	}

#ifdef _DEBUG
	fprintf(debugFile, "\n");
#endif


	BOOL shouldContinue = TRUE;
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) != 0 && shouldContinue)
	{
		switch (msg.message)
		{
		case WM_HOTKEY:
		{
#ifdef _DEBUG
			fprintf(debugFile, "WM_HOTKEY received\n");
#endif

			HWND wtWindowHandle = FindMainWindowAUMID(WT_AUMID);
			switch ((DWORD)wtWindowHandle)
			{
			case 0:
				if (GetLastError() == NO_MAIN_WINDOW)
				{
#ifdef _DEBUG
					fprintf(debugFile, "There is no terminal open. Trying to open one\n");
#endif
					StartupProcess(wtFinalPath);
				}

				break;

			default:
#ifdef _DEBUG
				fprintf(debugFile, "There is a terminal open. Trying to bring it to foreground\n");
#endif

				if (IsIconic(wtWindowHandle))
				{

					callResult = ShowWindow(wtWindowHandle, SW_NORMAL);
#ifdef _DEBUG
					if (callResult)
					{
						fprintf(debugFile, "Windows Terminal was restored correctly from the taskbar to the foreground\n");
					}
					else
					{
						fprintf(debugFile, "Can not restore the window from the taskbar to the foreground\n");
					}
#endif
				}
				else
				{
					callResult = SetForegroundWindow(wtWindowHandle);
#ifdef _DEBUG
					if (callResult)
					{
						fprintf(debugFile, "Windows Terminal was set correctly to the foreground\n");
					}
					else
					{
						fprintf(debugFile, "Can not bring the window to the foreground\n");
					}
#endif
				}
				break;
			}
		}
		break;

		case WM_CLOSE:
		{
#ifdef _DEBUG
			fprintf(debugFile, "WM_CLOSE received\n");
#endif
			shouldContinue = FALSE;
			break;
		}

		case WM_QUIT:
		{
#ifdef _DEBUG
			fprintf(debugFile, "WM_QUIT received\n");
#endif
			shouldContinue = FALSE;
			break;
		}
		}
#ifdef _DEBUG
		fflush(debugFile);
		fprintf(debugFile, "\n");
#endif
	}


	callResult = UnregisterHotKey(NULL, THREAD_HOTKEY_ID);

#ifdef _DEBUG
	if (callResult)
	{
		fprintf(debugFile, "Hotkey 'CTRL+ALT+T' unregistered\n");
	}
	else
	{
		fprintf(debugFile, "Hotkey could not be unregistered\n");
	}
#endif


	delete[] wtFinalPath;
	CloseHandle(mutexHandle);

#ifdef _DEBUG
	fclose(debugFile);
#endif

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
