#pragma once

// Local
#include <toolkit.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// STL
#include <iostream>

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
    Init(LRESULT(CALLBACK* pfn_window_procedure)(HWND, UINT, WPARAM, LPARAM),
         u32                window_width,
         u32                window_height,
         const std::string& window_class_name,
         const std::string& window_title);

    bool
    IsInitialized();

    u32
    GetWindowWidth();

    u32
    GetWindowHeight();

    HWND
    GetWindowHandle();

    void
    UpdateWindowDemensions();

    void
    SetFullScreen(bool full_screen);

    void
    ToggleFullScreen();

private:
    WindowTools();

    static inline bool is_initialized_ = false;
    bool               is_full_screen_;

    u32  window_width_;
    u32  window_height_;
    HWND window_handle_;
    RECT window_rect_;

    u32 window_x_;
    u32 window_y_;
};
