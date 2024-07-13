# secureblock

Given the need for wide compatibility across most Windows machines, I recommend using the Win32 API with GDI+.
This approach doesn't require additional libraries or frameworks to be installed by the end-user, as GDI+ is part of the Windows operating system starting from Windows XP.
This method ensures maximum compatibility and ease of deployment.

Here is the fully corrected and functional code using GDI+ to display a PNG image with transparency:

(see code)

Explanation:

1. GDI+ Initialization and Shutdown:

GDI+ is initialized at the start of the application and shut down before exiting.

2. Window Creation:

A layered window is created using the WS_EX_LAYERED extended style to support transparency.

3. Layered Window Attributes:

The SetLayeredWindowAttributes function is used to set the transparency of the window using the alpha channel of the PNG image.

4. Painting the Image:

The ShowSplashScreen function uses GDI+ to load and draw the PNG image on the window's device context (HDC), ensuring transparency is handled correctly.

5. Define WinMain:

The WinMain function calls wWinMain, passing the necessary parameters. This ensures the linker finds the correct entry point for a Windows GUI application.
Use Wide Character Versions of Functions:

Continue using RegisterClassW, CreateWindowExW, and MessageBoxW for wide-character support.

6. Center the Image:

To center the image on the screen, you need to calculate the center coordinates based on the screen dimensions and the image dimensions.
You can use the GetSystemMetrics function to get the screen dimensions and then adjust the window position accordingly.

7. Calculate Screen and Window Dimensions:

Use GetSystemMetrics(SM_CXSCREEN) and GetSystemMetrics(SM_CYSCREEN) to get the screen width and height.
Define the window width and height (set to 400x400 for this example).

8. Enter the Window:

Calculate the windowPosX and windowPosY to center the window on the screen.


This approach ensures that the program will function correctly on most Windows machines without requiring additional libraries to be installed by the user.
It leverages the built-in capabilities of GDI+ for handling images with transparency, providing broad compatibility and ease of deployment.

Compilation command:

g++ -o splash splash.cpp -lgdiplus -lgdi32 -luser32 -mwindows