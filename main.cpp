#include <windows.h>
#include <string>
#include <tchar.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>
#include <objbase.h>

// required libs + one for zip extract
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "oleaut32.lib")

// classs
static const TCHAR* szClassName = _T("Insurgency Patcher");
std::string szWindowTitle;

// Control IDs
#define ID_PATCH_BUTTON 1001 // main patcher
#define ID_BROWSE_BUTTON 1002 // file path browser
#define ID_CLI_BUTTON 1003 // cli window pops up, uses powershell but is a tad bit slower :) bypasses web req. permission error


HWND hMainWindow;
HWND hPatchButton;
HWND hStaticText;
HWND hEditBox;
HWND hBrowseButton;
HWND hCliButton;
HWND hNoteText;
HFONT hBoldFont;


std::string GetOSVersion() {
    // reliable detection for os, doesnt do anything aorn
    HMODULE hMod = GetModuleHandle(_T("ntdll.dll"));
    if (hMod) {
        typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion) {
            RTL_OSVERSIONINFOW rovi;
            ZeroMemory(&rovi, sizeof(rovi));
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (RtlGetVersion(&rovi) == 0) {
                if (rovi.dwMajorVersion == 10) {
                    if (rovi.dwBuildNumber >= 22000) {
                        return "Windows 11";
                    } else {
                        return "Windows 10";
                    }
                } else if (rovi.dwMajorVersion == 6) {
                    switch (rovi.dwMinorVersion) {
                        case 3: return "Windows 8.1";
                        case 2: return "Windows 8";
                        case 1: return "Windows 7";
                        case 0: return "Windows Vista";
                    }
                } else if (rovi.dwMajorVersion == 5) {
                    switch (rovi.dwMinorVersion) {
                        case 1: return "Windows XP";
                        case 0: return "Windows 2000";
                    }
                }
            }
        }
    }
    
    // registry check since above does not work, stolen from james
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        TCHAR productName[256];
        DWORD bufferSize = sizeof(productName);
        if (RegQueryValueEx(hKey, _T("ProductName"), NULL, NULL, (LPBYTE)productName, &bufferSize) == ERROR_SUCCESS) {
            std::string product = std::string(productName, bufferSize / sizeof(TCHAR) - 1);
            if (product.find("Windows 11") != std::string::npos) {
                RegCloseKey(hKey);
                return "Windows 11";
            }
        }
        
        DWORD buildNumber;
        bufferSize = sizeof(buildNumber);
        if (RegQueryValueEx(hKey, _T("CurrentBuildNumber"), NULL, NULL, (LPBYTE)&buildNumber, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (buildNumber >= 22000) {
                return "Windows 11";
            } else if (buildNumber >= 10240) {
                return "Windows 10";
            }
        }
        RegCloseKey(hKey);
    }
    
    
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
        if (osvi.dwMajorVersion == 10) {
            return "Windows 10/11";
        } else if (osvi.dwMajorVersion == 6) {
            switch (osvi.dwMinorVersion) {
                case 3: return "Windows 8.1";
                case 2: return "Windows 8";
                case 1: return "Windows 7";
                case 0: return "Windows Vista";
            }
        }
    }
    return "Windows";
}

// window to confirm os
bool ShowOSConfirmationDialog() {
    std::string osName = GetOSVersion();
    std::string message = "Your operating system seems to be " + osName + ", is that correct?";
    
    int result = MessageBox(NULL, 
                          message.c_str(), 
                          "Insurgency Patcher", 
                          MB_YESNO | MB_ICONQUESTION);
    
    return result == IDYES;
}

// file browser (select root for insurgency, ill add a note later)
void BrowseForFolder(HWND hWnd) {
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = _T("Select Insurgency Game Directory");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL) {
        TCHAR path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            SetWindowText(hEditBox, path);
        }
        CoTaskMemFree(pidl);
    }
}

// Removal of old BattleEye (causes major issues)
bool DeleteDirectory(const std::string& path) {
    std::string searchPath = path + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                std::string filePath = path + "\\" + findData.cFileName;
                
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    DeleteDirectory(filePath);
                } else {
                    SetFileAttributesA(filePath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFileA(filePath.c_str());
                }
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return RemoveDirectoryA(path.c_str()) != 0;
}

// Downloader (used to fix BattleEye)
bool DownloadFile(const std::string& url, const std::string& localPath) {
    HINTERNET hInternet = InternetOpenA("InsurgencyPatcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;
    
    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }
    
    HANDLE hFile = CreateFileA(localPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];
    DWORD bytesRead, bytesWritten;
    
    while (InternetReadFile(hUrl, buffer, bufferSize, &bytesRead) && bytesRead > 0) {
        WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
    }
    
    CloseHandle(hFile);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    
    return true;
}

// .ZIP extractor, uses Powershell (slow but bypasses perm issues) should work aorn
bool ExtractZIP(const std::string& zipPath, const std::string& extractPath) {

    std::string command = "powershell -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + extractPath + "' -Force\"";
    
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    BOOL result = CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    
    if (result) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    
    return result != FALSE;
}

// CLI button workarounds and crap
void OpenCLI(HWND hWnd) {
    TCHAR path[MAX_PATH];
    GetWindowText(hEditBox, path, MAX_PATH);
    
    if (_tcslen(path) == 0) {
        MessageBox(hWnd, _T("Please select a game path first!"), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    std::string gamePath(path, path + _tcslen(path));
    std::string battleEyePath = gamePath + "\\BattleEye";
    std::string zipPath = gamePath + "\\BattlEye.zip";
    
    // batch output below
    std::string batchPath = gamePath + "\\patch_cli.bat";
    FILE* batchFile = fopen(batchPath.c_str(), "w");
    if (batchFile) {
        std::string osVersion = GetOSVersion();
        
        fprintf(batchFile, "@echo off\n");
        fprintf(batchFile, "echo.\n");
        fprintf(batchFile, "color 0a\n"); 
        fprintf(batchFile, "echo Insurgency Patcher,  %s\n", osVersion.c_str());
        fprintf(batchFile, "echo.\n");
        fprintf(batchFile, "color 0f\n"); 
        fprintf(batchFile, "echo Downloading latest BattleEye module..\n");
        fprintf(batchFile, "echo Importing BattleEye..\n");
        fprintf(batchFile, "echo Deleting old configuration..\n");
        fprintf(batchFile, "echo Extracting..\n");
        fprintf(batchFile, "echo.\n");
        
        
        fprintf(batchFile, "del /f /q \"%s\" 2>nul\n", zipPath.c_str());
        fprintf(batchFile, "rmdir /s /q \"%s\" 2>nul\n", battleEyePath.c_str());
        fprintf(batchFile, "powershell -Command \"Invoke-WebRequest -Uri 'https://github.com/zero-cache/insurgency2-patcher/raw/refs/heads/main/bin/battle_eye_module/BattlEye.zip' -OutFile '%s'\"\n", zipPath.c_str()); // DO NOT MANIPULATE!!
        fprintf(batchFile, "if exist \"%s\" (\n", zipPath.c_str());
        fprintf(batchFile, "    powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"\n", zipPath.c_str(), gamePath.c_str());
        fprintf(batchFile, "    if exist \"%s\" (\n", zipPath.c_str()); 
        fprintf(batchFile, "        color 0e\n"); 
        fprintf(batchFile, "        echo Successful.\n");
        fprintf(batchFile, "    ) else (\n");
        fprintf(batchFile, "        color 0c\n"); 
        fprintf(batchFile, "        echo Failed.\n");
        fprintf(batchFile, "    )\n");
        fprintf(batchFile, ") else (\n");
        fprintf(batchFile, "    color 0c\n"); 
        fprintf(batchFile, "    echo Failed.\n");
        fprintf(batchFile, ")\n");
        fprintf(batchFile, "echo.\n");
        fprintf(batchFile, "pause\n");
        fprintf(batchFile, "del /f /q \"%s\"\n", batchPath.c_str());
        
        fclose(batchFile);
        
        
        ShellExecute(NULL, "open", batchPath.c_str(), NULL, NULL, SW_SHOW);
    }
}

// patcher
void PerformPatch(HWND hWnd) {
    TCHAR path[MAX_PATH];
    GetWindowText(hEditBox, path, MAX_PATH);
    
    if (_tcslen(path) == 0) {
        MessageBox(hWnd, _T("Operation failed, try using the CLI version instead."), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    std::string gamePath(path, path + _tcslen(path));
    std::string battleEyePath = gamePath + "\\BattleEye";
    std::string zipPath = gamePath + "\\BattlEye.zip";
    
    // Deletion of old BattleEye (bluescreens on win11)
    if (GetFileAttributesA(battleEyePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        DeleteDirectory(battleEyePath);
    }
    
    // Patch the fix (DO NOT MODIFY!)
    if (!DownloadFile("https://github.com/zero-cache/insurgency2-patcher/raw/refs/heads/main/bin/battle_eye_module/BattlEye.zip", zipPath)) {
        MessageBox(hWnd, _T("Operation failed, try using the CLI version instead."), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    
    if (!ExtractZIP(zipPath, gamePath)) {
        DeleteFileA(zipPath.c_str());
        MessageBox(hWnd, _T("Operation failed, try using the CLI version instead."), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    
    DeleteFileA(zipPath.c_str());
    
    // Success!
    MessageBox(hWnd, _T("Operation successful, restart Insurgency."), _T("Success"), MB_OK | MB_ICONINFORMATION);
}

// main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
        {
            
            LOGFONT lf;
            GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
            lf.lfWeight = FW_BOLD;
            hBoldFont = CreateFontIndirect(&lf);
            
            
            hStaticText = CreateWindow(
                _T("STATIC"), 
                _T("Insurgency Game Path"),
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                20, 20, 200, 25,
                hWnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            
            SendMessage(hStaticText, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
            
            
            hEditBox = CreateWindow(
                _T("EDIT"), 
                _T(""),
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                20, 50, 240, 25,
                hWnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            
            hNoteText = CreateWindow(
                _T("STATIC"), 
                _T("-- Choose the root directory!"),
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                270, 55, 180, 20,
                hWnd, NULL, GetModuleHandle(NULL), NULL
            );
            
            // note text to warn about root (check top)
            LOGFONT noteLf;
            memset(&noteLf, 0, sizeof(LOGFONT));
            noteLf.lfHeight = 14; 
            noteLf.lfWeight = FW_NORMAL;
            noteLf.lfCharSet = DEFAULT_CHARSET;
            noteLf.lfOutPrecision = OUT_DEFAULT_PRECIS;
            noteLf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            noteLf.lfQuality = DEFAULT_QUALITY;
            noteLf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
            _tcscpy_s(noteLf.lfFaceName, _T("Arial"));
            HFONT hNoteFont = CreateFontIndirect(&noteLf);
            SendMessage(hNoteText, WM_SETFONT, (WPARAM)hNoteFont, TRUE);
            
            
            hBrowseButton = CreateWindow(
                _T("BUTTON"), 
                _T("Choose Path"),
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 85, 100, 35,
                hWnd, (HMENU)ID_BROWSE_BUTTON, GetModuleHandle(NULL), NULL
            );
            
            
            hCliButton = CreateWindow(
                _T("BUTTON"), 
                _T("Open CLI"),
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                130, 85, 100, 35,
                hWnd, (HMENU)ID_CLI_BUTTON, GetModuleHandle(NULL), NULL
            );
            
            // patch btn
            hPatchButton = CreateWindow(
                _T("BUTTON"), 
                _T("Patch"),
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                320, 85, 100, 35,
                hWnd, (HMENU)ID_PATCH_BUTTON, GetModuleHandle(NULL), NULL
            );
        }
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_PATCH_BUTTON) {
                PerformPatch(hWnd);
            } else if (LOWORD(wParam) == ID_BROWSE_BUTTON) {
                BrowseForFolder(hWnd);
            } else if (LOWORD(wParam) == ID_CLI_BUTTON) {
                OpenCLI(hWnd);
            }
            break;

        case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(0, 0, 0)); 
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(0, 0, 0)); 
            SetBkColor(hdcEdit, RGB(255, 255, 255)); 
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
// close
        case WM_DESTROY:
            if (hBoldFont) DeleteObject(hBoldFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


bool RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;
    
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); 
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szClassName;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    
    return RegisterClassEx(&wcex) != 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // os dialog thing goes first
    if (!ShowOSConfirmationDialog()) {
        MessageBox(NULL, _T("Application will now exit."), _T("Info"), MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    
    std::string osVersion = GetOSVersion();
    szWindowTitle = "Insurgency Patcher for " + osVersion;
    
    if (!RegisterWindowClass(hInstance)) {
        MessageBox(NULL, _T("Failed to register window class!"), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    
    
    hMainWindow = CreateWindow(
        szClassName,
        szWindowTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 440, 160,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hMainWindow) {
        MessageBox(NULL, _T("Failed to create window!"), _T("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
