#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <filesystem>
#include <commctrl.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "resource.h"

// Function to open a folder selection dialog
std::string BrowseFolder() {
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    TCHAR path[MAX_PATH];

    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.pszDisplayName = path;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        // Get the name of the folder and display it
        SHGetPathFromIDList(pidl, path);
        std::string folderPath(path);

        // Free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return folderPath;
    }
    return "";
}

// Function to format file size in KB with comma separators
std::string FormatFileSize(uintmax_t size) {
    size_t sizeInKB = size / 1024;
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << sizeInKB << " KB";
    return ss.str();
}

// Function to format file date and time in "MM/DD/YYYY HH:MM AM/PM" format
std::string FormatFileDate(std::filesystem::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
    std::tm* localTime = std::localtime(&cftime);
    std::stringstream ss;
    ss << std::put_time(localTime, "%m/%d/%Y %I:%M %p");
    return ss.str();
}

// Function to recursively list .exe files in the folder
void ListExeFiles(const std::string& folder, std::vector<std::filesystem::directory_entry>& exeFiles) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
        if (entry.path().extension() == ".exe") {
            exeFiles.push_back(entry);
        }
    }
}

// Function to add firewall rules using netsh
void AddFirewallRules(const std::vector<std::string>& exeFiles) {
    for (const auto& file : exeFiles) {
        std::string fileName = std::filesystem::path(file).filename().string();
        std::string inRule = "netsh advfirewall firewall add rule name=\"" + fileName + " IN\" dir=in action=block protocol=any program=\"" + file + "\"";
        std::string outRule = "netsh advfirewall firewall add rule name=\"" + fileName + " OUT\" dir=out action=block protocol=any program=\"" + file + "\"";

        system(inRule.c_str());
        system(outRule.c_str());
    }
}

// Global variables
HWND hwndList;
std::vector<std::filesystem::directory_entry> exeFiles;

// Dialog procedure
INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        hwndList = GetDlgItem(hwndDlg, IDC_LIST1);

        // Initialize ListView with checkboxes
        ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

        // Insert columns into ListView
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

        // Add items to ListView
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

        // Set label text
        HWND hwndLabel = GetDlgItem(hwndDlg, IDC_STATIC);
        std::string labelText = "\nFound " + std::to_string(exeFiles.size()) + " files.\n\nIf any of the below files shouldn’t be blocked, please uncheck them before proceeding.\n";
        SetWindowText(hwndLabel, labelText.c_str());

        // Set label font
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

// Main function
int main() {
    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    std::string folder = BrowseFolder();
    if (!folder.empty()) {
        ListExeFiles(folder, exeFiles);
        if (!exeFiles.empty()) {
            // Initialize common controls
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx(&icex);

            // Create and show the dialog box
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
        } else {
            MessageBox(NULL, "No .exe files found in the selected folder.", "Info", MB_OK | MB_ICONINFORMATION);
        }
    } else {
        MessageBox(NULL, "No folder selected.", "Info", MB_OK | MB_ICONINFORMATION);
    }

    return 0;
}
