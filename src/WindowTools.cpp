#include <WindowTools.h>

WindowTools::WindowTools(std::string window_class_name, std::string window_title)
    : client_width_(720)
    , client_height_(1280)
    , window_handle_(nullptr)
    , window_rect_({ 0 })
    , window_class_name_(window_class_name)
    , window_title_(window_title)
    , window_x_(0)
    , window_y_(0)
{
}

void
WindowTools::SetClientWidth(u32 client_width)
{
    client_width_ = client_width;
}

void
WindowTools::SetClientHeight(u32 client_height)
{
    client_height_ = client_height;
}

u32
WindowTools::GetClientWidth()
{
    return client_width_;
}

u32
WindowTools::GetClientHeight()
{
    return client_height_;
}

LRESULT CALLBACK
        WindowTools::WindowProcedure(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
    return DefWindowProc(hwnd, u_msg, w_param, l_param);
}

void
WindowTools::WindowSetup()
{
    // Window class setup
    const wchar_t* wide_class_name =
        std::wstring(window_class_name_.begin(), window_class_name_.end()).c_str();
    const wchar_t* wide_window_title =
        std::wstring(window_title_.begin(), window_title_.end()).c_str();
    ;
    WNDCLASSEXW window_class   = {};
    window_class.cbSize        = sizeof(WNDCLASSEX);
    window_class.style         = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc   = &WindowTools::WindowProcedure;
    window_class.hInstance     = GetModuleHandle(NULL);
    window_class.hIcon         = LoadIcon(window_class.hInstance, NULL);
    window_class.hCursor       = LoadCursor(window_class.hInstance, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = wide_class_name;
    window_class.hIconSm       = LoadIcon(window_class.hInstance, NULL);

    // Window instance setup
    u32  screen_height = GetSystemMetrics(SM_CXSCREEN);
    u32  screen_width  = GetSystemMetrics(SM_CYSCREEN);
    RECT window_rect   = { 0, 0, static_cast<LONG>(client_width_),
                         static_cast<LONG>(client_height_) };

    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    u32 window_width  = window_rect.right - window_rect.left;
    u32 window_height = window_rect.bottom - window_rect.top;

    window_x_ = std::max<u32>(0, (screen_width - window_width) / 2);
    window_y_ = std::max<u32>(0, (screen_height - window_height) / 2);

    window_handle_ = CreateWindowExW(NULL, wide_class_name, wide_window_title, WS_OVERLAPPEDWINDOW,
                                     window_x_, window_y_, window_width, window_height, NULL, NULL,
                                     window_class.hInstance, nullptr);
    Assert(window_handle_, "Could not create window handle!\n");
}
