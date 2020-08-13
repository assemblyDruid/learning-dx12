#pragma once

// Local
#include <toolkit.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WindowTools
{
public:
    static WindowTools*
    GetInstance()
    {
        static WindowTools instance;
        return &instance;
    }

    WindowTools(const WindowTools&) = delete;

    void
    operator=(const WindowTools&) = delete;

    void
    Init();

    void
    SetWindowClassName(const char* window_class_name);

    void
    SetWindowTitle(const char* window_title);

    void
    SetWindowWidth(u32 window_width);

    void
    SetWindowHeight(u32 window_height);

    u32
    GetWindowWidth();

    u32
    GetWindowHeight();

    HWND
    GetWindowHandle();

private:
    WindowTools();

    static LRESULT CALLBACK
    WindowProcedure(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

    bool is_initialized_;

    std::string window_title_;
    std::string window_class_name_;

    u32  window_width_;
    u32  window_height_;
    HWND window_handle_;
    RECT window_rect_;

    u32 window_x_;
    u32 window_y_;
};
