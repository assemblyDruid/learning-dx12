#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // hresult
#include <stdint.h>
#include <wrl.h>

// DX includes
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// STL
#include <iostream>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>

// Type short-hands
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define r32 float
#define r64 double



enum class LogType
{
    kInfo = 0,
    kWarning,
    kError
};


class LogManager
{
public:
    static void Log(const std::string& msg)
    {
        std::cout << log_prefix_ << msg << std::endl;
    }

    static void Log(const std::string& msg, LogType log_type)
    {
        switch(log_type)
        {
            case LogType::kInfo:
            {
                std::cout << log_prefix_ << msg << std::endl;
                break;
            }
            case LogType::kWarning:
            {
                std::cout << warning_prefix_ << msg << std::endl;
                break;
            }
            case LogType::kError:
            {
                std::cerr << error_prefix_ << msg << std::endl;
                break;
            }
        }
    }

private:
    inline static std::string log_prefix_ = "[ log ] ";
    inline static std::string info_prefix_ = log_prefix_ + "[ info ] ";
    inline static std::string warning_prefix_ = log_prefix_ + "[ warning ] ";
    inline static std::string error_prefix_ = log_prefix_ + "[ error ] ";
};


inline void ThrowWithMessage(const char* msg = "")
{
        LogManager::Log(msg, LogType::kError);
        throw std::exception(msg);
}


inline void Assert(HRESULT hr)
{
    if (FAILED(hr))
    {
        ThrowWithMessage();
    }
}


inline void Assert(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        ThrowWithMessage(msg);
    }
}


inline void Assert(bool cond, const char* msg)
{
    if (!cond)
    {
        ThrowWithMessage(msg);
    }
}


class DXTools
{
public:
    DXTools()
        : use_warp_(false)
        , dx_is_initialized_(false)
        , device_(nullptr)
        , command_queue_(nullptr)
        , swap_chain_(nullptr)
        , command_list_(nullptr)
        , rtv_descriptor_heap_(nullptr)
        , fence_(nullptr)
        , fence_event_(nullptr)
        , rtv_descriptor_size_(0)
        , current_back_buffer_index_(0)
        , is_v_sync_(true)
        , is_tearing_supported_(false)
        , is_full_screen_(false)
    {
        EnableDebugLayer(); // no-op on release builds
        GetAdapter();

        dx_is_initialized_ = true;
    }

    void SetUseWarp(bool use_warp) { use_warp_ = use_warp; }
    bool GetUseWarp() { return use_warp_; }

    void EnableDebugLayer()
    {
#if __DXDEBUG__ == 1
        Microsoft::WRL::ComPtr<ID3D12Debug> dx_debug_;
        Assert(D3D12GetDebugInterface(IID_PPV_ARGS(&dx_debug_)),
               "Could not enable debug layer.\n");
        dx_debug_->EnableDebugLayer();
        LogManager::Log("DX debug enabled.", LogType::kInfo);
#endif
    }

private:

    Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter()
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_factory;
        UINT create_factory_flags = 0;

#if __DXDEBUG__ == 1
        create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        Assert(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

        Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter_1;
        Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter_4;

        if (use_warp_)
        {
            Assert(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter_1)));
            Assert(dxgi_adapter_1.As(&dxgi_adapter_4));
        }
        else
        {
            DXGI_ADAPTER_DESC1 dxgi_adapter_desc_1;
            dxgi_adapter_1->GetDesc1(&dxgi_adapter_desc_1);
        }
    }

    static const u8 kNumFrames_ = 3;
    bool use_warp_;
    u32 dx_is_initialized_;

    // D3D graphics components
    Microsoft::WRL::ComPtr<ID3D12Device2> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D12Resource> back_buffers_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocators_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap_;

    // Sync components
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    u64 fence_value_;
    u64 frame_fence_values_[kNumFrames_];
    HANDLE fence_event_;

    UINT rtv_descriptor_size_;
    UINT current_back_buffer_index_;

    // VSync components
    bool is_v_sync_;
    bool is_tearing_supported_;
    bool is_full_screen_;
};


class WindowTools
{
public:
    WindowTools(std::string window_class_name, std::string window_title)
        : client_width_(720)
        , client_height_(1280)
        , window_handle_(nullptr)
        , window_rect_({ 0 })
        , window_class_name_(window_class_name)
        , window_title_(window_title)
        , window_x_(0)
        , window_y_(0) {}

    void SetClientWidth(u32 client_width)
    {
        static u8 times_set = 0;
        if (!times_set)
        {
            client_width_ = client_width;
            times_set++;
        }
        else if (times_set != 1)
        {
            LogManager::Log("The client width has already been set!",
                            LogType::kWarning);
        }

    }

    void SetClientHeight(u32 client_height) { client_height_ = client_height; }

    u32 GetClientWidth() { return client_width_; }
    u32 GetClientHeight() { return client_height_; }


private:
    static LRESULT CALLBACK WindowProceedure(HWND, UINT, WPARAM, LPARAM);
    void WindowSetup()
    {
        // Window class setup
        const wchar_t* wide_class_name = std::wstring(window_class_name_.begin(),
                                                    window_class_name_.end()).c_str();
        const wchar_t* wide_window_title = std::wstring(window_title_.begin(),
                                                      window_title_.end()).c_str();;
        WNDCLASSEXW window_class = { };
        window_class.cbSize = sizeof(WNDCLASSEX);
        window_class.style  = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = &WindowTools::WindowProceedure;
        window_class.hInstance = GetModuleHandle(NULL);
        window_class.hIcon = LoadIcon(window_class.hInstance, NULL);
        window_class.hCursor = LoadCursor(window_class.hInstance, IDC_ARROW);
        window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        window_class.lpszClassName = wide_class_name;
            window_class.hIconSm = LoadIcon(window_class.hInstance, NULL);

        // Window instance setup
        u32 screen_height = GetSystemMetrics(SM_CXSCREEN);
        u32 screen_width = GetSystemMetrics(SM_CYSCREEN);
        RECT window_rect = { 0, 0,
                             static_cast<LONG>(client_width_),
                             static_cast<LONG>(client_height_) };
        AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
        u32 window_width = window_rect.right - window_rect.left;
        u32 window_height = window_rect.bottom - window_rect.top;

        window_x_ = std::max<u32>(0, (screen_width - window_width) / 2);
        window_y_ = std::max<u32>(0, (screen_height - window_height) / 2);

        window_handle_ = CreateWindowExW(NULL,
                                         wide_class_name,
                                         wide_window_title,
                                         WS_OVERLAPPEDWINDOW,
                                         window_x_,
                                         window_y_,
                                         window_width,
                                         window_height,
                                         NULL,
                                         NULL,
                                         window_class.hInstance,
                                         nullptr);
        Assert(window_handle_, "Could not create window handle!\n");
    }

    std::string window_title_;
    const std::string window_class_name_;

    u32  client_width_;
    u32  client_height_;
    HWND window_handle_;
    RECT window_rect_;

    u32 window_x_;
    u32 window_y_;
};
