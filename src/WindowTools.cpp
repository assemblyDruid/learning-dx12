#include <WindowTools.h>

class Controller;

WindowTools::WindowTools()
    // : is_initialized_(false)
    : is_full_screen_(false)
    , window_width_(720)
    , window_height_(1280)
    , window_handle_(nullptr)
    , window_rect_({0})
    , window_x_(0)
    , window_y_(0)
{
}

void WindowTools::Init(
    LRESULT(CALLBACK* pfn_window_procedure)(HWND, UINT, WPARAM, LPARAM),
    u32                window_width,
    u32                window_height,
    const std::string& window_class_name,
    const std::string& window_title)
{
    window_width_  = window_width;
    window_height_ = window_height;

    // Window class setup
    std::wstring wide_class_name(window_class_name.begin(), window_class_name.end());
    std::wstring wide_window_title(window_title.begin(), window_title.end());

    WNDCLASSEXW window_class   = {};
    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = pfn_window_procedure;
    window_class.hInstance     = GetModuleHandle(NULL);
    window_class.hIcon         = LoadIcon(window_class.hInstance, NULL);
    window_class.hCursor       = LoadCursor(window_class.hInstance, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = wide_class_name.c_str();
    window_class.hIconSm       = LoadIcon(window_class.hInstance, NULL);

    // Window instance setup
    u32  screen_height = GetSystemMetrics(SM_CXSCREEN);
    u32  screen_width  = GetSystemMetrics(SM_CYSCREEN);
    RECT window_rect   = {
        0, 0, static_cast<LONG>(window_width_), static_cast<LONG>(window_height_)};

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    u32 adjusted_window_width  = window_rect.right - window_rect.left;
    u32 adjusted_window_height = window_rect.bottom - window_rect.top;

    window_x_ = std::max<u32>(0, (screen_width - adjusted_window_width) / 2);
    window_y_ = std::max<u32>(0, (screen_height - adjusted_window_height) / 2);

    // Register the window class
    Require(RegisterClassExW(&window_class) != 0, "Could not register window class!\n");

    // [ cfarvin::REVISIT ] Should we use the adjusted_ window demensions here,
    // or not? Create the window
    window_handle_ = CreateWindowExW(NULL,
                                     wide_class_name.c_str(),
                                     wide_window_title.c_str(),
                                     WS_OVERLAPPEDWINDOW,
                                     window_x_,
                                     window_y_,
                                     adjusted_window_width,
                                     adjusted_window_height,
                                     NULL,
                                     NULL,
                                     window_class.hInstance,
                                     nullptr);
    Assert(window_handle_, "Could not create window handle!\n");

    is_initialized_ = true;
    ShowWindow(window_handle_, SW_SHOW);
}

bool WindowTools::IsInitialized()
{
    return is_initialized_;
}

u32 WindowTools::GetWindowWidth()
{
    return window_width_;
}

u32 WindowTools::GetWindowHeight()
{
    return window_height_;
}

HWND WindowTools::GetWindowHandle()
{
    return window_handle_;
}

void WindowTools::UpdateWindowDemensions()
{
    GetWindowRect(window_handle_, &window_rect_);
    window_width_  = window_rect_.right - window_rect_.left;
    window_height_ = window_rect_.bottom - window_rect_.top;
}

void WindowTools::SetFullScreen(bool full_screen)
{
    if (full_screen != is_full_screen_)
    {
        is_full_screen_ = full_screen;

        if (full_screen)
        {
            // Store old window rect value
            UpdateWindowDemensions();

            // Remove _all_ window decorations
            UINT window_style =
                WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                        WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            SetWindowLongW(window_handle_, GWL_STYLE, window_style);

            // Query for the nearest display device
            HMONITOR monitor =
                MonitorFromWindow(window_handle_, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitor_info = {};
            monitor_info.cbSize        = sizeof(MONITORINFOEX);
            GetMonitorInfo(monitor, &monitor_info);

            // Set window position
            SetWindowPos(window_handle_,
                         HWND_TOP,
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_FRAMECHANGED | SW_MAXIMIZE);

            ShowWindow(window_handle_, SW_MAXIMIZE);
        }
        else
        {
            // Restore window decorators
            SetWindowLongW(window_handle_, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            // Restore window position
            SetWindowPos(window_handle_,
                         HWND_NOTOPMOST,
                         window_rect_.left,
                         window_rect_.top,
                         window_rect_.right - window_rect_.left,
                         window_rect_.bottom - window_rect_.top,
                         SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ShowWindow(window_handle_, SW_NORMAL);
        }
    }
}

void WindowTools::ToggleFullScreen()
{
    SetFullScreen(!is_full_screen_);
}
