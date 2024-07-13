#include <windows.h>
#include <gdiplus.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ShowSplashScreen(HDC hdc);

const int splashScreenWidth = 540;
const int splashScreenHeight = 250;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
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
        return 0;
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
