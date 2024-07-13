#include <windows.h>
#include <gdiplus.h>
#include <thread>
#include <chrono>
#include <shlobj.h>
#include <string>
#include <vector>
#include <filesystem>
#include <commctrl.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Splash screen definitions and functions
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ShowSplashScreen(HDC hdc);

const int splashScreenWidth = 540;
const int splashScreenHeight = 250;

void ShowSplash(HINSTANCE hInstance, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"TransparentSplash";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = 540;
    int windowHeight = 250;
    int windowPosX = (screenWidth - windowWidth) / 2;
    int windowPosY = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Transparent Splash Screen",
        WS_POPUP,
        windowPosX, windowPosY, windowWidth, windowHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // Set the window to be transparent with alpha blending
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    std::thread([](HWND hwnd) {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }, hwnd).detach();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        ShowSplashScreen(hdc);
        EndPaint(hwnd, &ps);
    } break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void ShowSplashScreen(HDC hdc) {
    Graphics graphics(hdc);
    Image image(L"splash.png");

    if (image.GetLastStatus() != Ok) {
        MessageBoxW(NULL, L"Failed to load image!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    int imgWidth = image.GetWidth();
    int imgHeight = image.GetHeight();

    // Calculate the center position for the image within the window
    int x = (splashScreenWidth - imgWidth) / 2;
    int y = (splashScreenHeight - imgHeight) / 2;

    graphics.DrawImage(&image, x, y, imgWidth, imgHeight);
}

// Main application definitions and functions
std::string BrowseFolder() {
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    TCHAR path[MAX_PATH];

    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.pszDisplayName = path;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        SHGetPathFromIDList(pidl, path);
        std::string folderPath(path);

        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return folderPath;
    }
    return "";
}

std::string FormatFileSize(uintmax_t size) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (size >= (1ULL << 30))
        ss << (size / (1ULL << 30)) << " GB";
    else if (size >= (1ULL << 20))
        ss << (size / (1ULL << 20)) << " MB";
    else if (size >= (1ULL << 10))
        ss << (size / (1ULL << 10)) << " KB";
    else
        ss << size << " B";
    return ss.str();
}

std::string FormatFileDate(std::filesystem::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&cftime), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void ListExeFiles(const std::string& folder, std::vector<std::filesystem::directory_entry>& exeFiles) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
        if (entry.path().extension() == ".exe") {
            exeFiles.push_back(entry);
        }
    }
}

void AddFirewallRules(const std::vector<std::string>& exeFiles) {
    for (const auto& file : exeFiles) {
        std::string fileName = std::filesystem::path(file).filename().string();
        std::string inRule = "netsh advfirewall firewall add rule name=\"" + fileName + " IN\" dir=in action=block protocol=any program=\"" + file + "\"";
        std::string outRule = "netsh advfirewall firewall add rule name=\"" + fileName + " OUT\" dir=out action=block protocol=any program=\"" + file + "\"";

        system(inRule.c_str());
        system(outRule.c_str());
    }
}

HWND hwndList;
std::vector<std::filesystem::directory_entry> exeFiles;

INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        hwndList = GetDlgItem(hwndDlg, IDC_LIST1);

        ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_WIDTH | LVCF_TEXT;
        lvColumn.pszText = const_cast<LPSTR>("File Name");
        lvColumn.cx = 250;
        ListView_InsertColumn(hwndList, 0, &lvColumn);

        lvColumn.pszText = const_cast<LPSTR>("Size");
        lvColumn.cx = 100;
        ListView_InsertColumn(hwndList, 1, &lvColumn);

        lvColumn.pszText = const_cast<LPSTR>("Date");
        lvColumn.cx = 130;
        ListView_InsertColumn(hwndList, 2, &lvColumn);

        LVITEM lvItem;
        lvItem.mask = LVIF_TEXT | LVIF_STATE;
        lvItem.state = 0;
        lvItem.stateMask = 0;

        for (size_t i = 0; i < exeFiles.size(); ++i) {
            std::string fileName = exeFiles[i].path().string();
            std::string fileSize = FormatFileSize(std::filesystem::file_size(exeFiles[i]));
            std::string fileDate = FormatFileDate(exeFiles[i].last_write_time());

            lvItem.iItem = i;
            lvItem.iSubItem = 0;
            lvItem.pszText = const_cast<LPSTR>(fileName.c_str());
            ListView_InsertItem(hwndList, &lvItem);

            lvItem.iSubItem = 1;
            lvItem.pszText = const_cast<LPSTR>(fileSize.c_str());
            ListView_SetItem(hwndList, &lvItem);

            lvItem.iSubItem = 2;
            lvItem.pszText = const_cast<LPSTR>(fileDate.c_str());
            ListView_SetItem(hwndList, &lvItem);

            ListView_SetCheckState(hwndList, i, TRUE);
        }

        HWND hwndLabel = GetDlgItem(hwndDlg, IDC_STATIC);
        std::string labelText = "\nFound " + std::to_string(exeFiles.size()) + " files.\n\nIf any of the below files shouldn’t be blocked, please uncheck them before proceeding.\n";
        SetWindowText(hwndLabel, labelText.c_str());

        HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
        SendMessage(hwndLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        return TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            int count = ListView_GetItemCount(hwndList);
            std::vector<std::string> selectedFiles;
            for (int i = 0; i < count; ++i) {
                if (ListView_GetCheckState(hwndList, i)) {
                    char buffer[MAX_PATH];
                    LVITEM lvItem;
                    lvItem.iSubItem = 0;
                    lvItem.cchTextMax = MAX_PATH;
                    lvItem.pszText = buffer;
                    lvItem.iItem = i;
                    lvItem.mask = LVIF_TEXT;
                    ListView_GetItem(hwndList, &lvItem);
                    selectedFiles.push_back(buffer);
                }
            }
            AddFirewallRules(selectedFiles);
            EndDialog(hwndDlg, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwndDlg, IDCANCEL);
        }
        return TRUE;
    }
    }
    return FALSE;
}

void RunMainApplication() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    std::string folder = BrowseFolder();
    if (!folder.empty()) {
        ListExeFiles(folder, exeFiles);
        if (!exeFiles.empty()) {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
        } else {
            MessageBox(NULL, "No .exe files found in the selected folder.", "Info", MB_OK | MB_ICONINFORMATION);
        }
    } else {
        MessageBox(NULL, "No folder selected.", "Info", MB_OK | MB_ICONINFORMATION);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    ShowSplash(hInstance, nCmdShow);
    RunMainApplication();
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}

#ifdef __cplusplus
}
#endif
