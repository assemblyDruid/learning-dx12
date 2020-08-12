#pragma once

// Local
#include <toolkit.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WindowTools
{
public:
    WindowTools(std::string window_class_name, std::string window_title);

    void
    Init();

    void
    SetClientWidth(u32 client_width);

    void
    SetClientHeight(u32 client_height);

    u32
    GetClientWidth();

    u32
    GetClientHeight();

    HWND
    GetWindowHandle();

private:
    static LRESULT CALLBACK
    WindowProcedure(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

    std::string       window_title_;
    const std::string window_class_name_;

    u32  client_width_;
    u32  client_height_;
    HWND window_handle_;
    RECT window_rect_;

    u32 window_x_;
    u32 window_y_;
};
