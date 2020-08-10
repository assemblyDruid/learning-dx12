#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // hresult
#include <stdint.h>
#include <wrl.h>

// DX includes
#include <d3d12.h>
#include <dxgi.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// STL
#include <iostream>
#include <algorithm>
#include <cassert>
#include <chrono>

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


inline void Assert(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

inline void Assert(HRESULT hr, const char* msg)
{
    if (FAILED(hr))
    {
        throw std::exception(msg);
    }
}


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
            }
            case LogType::kWarning:
            {
                std::cout << warning_prefix_ << msg << std::endl;
            }
            case LogType::kError:
            {
                std::cerr << error_prefix_ << msg << std::endl;
            }
        }
    }

private:
    inline static std::string log_prefix_ = "[ log ] ";
    inline static std::string info_prefix_ = log_prefix_ + "[ info ] ";
    inline static std::string warning_prefix_ = log_prefix_ + "[ warning ] ";
    inline static std::string error_prefix_ = log_prefix_ + "[ error ] ";
};


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
#endif
    }

private:
    static const u8 kNumFrames_ = 3;
    bool use_warp_;
    u32 dx_is_initialized_;

    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D12Resource> back_buffers_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocators_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap_;

    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    u64 fence_value_;
    u64 frame_fence_values_[kNumFrames_];
    HANDLE fence_event_;

    UINT rtv_descriptor_size_;
    UINT current_back_buffer_index_;

    bool is_v_sync_;
    bool is_tearing_supported_;
    bool is_full_screen_;
};


class WindowTools
{
public:
    WindowTools()
        : client_width_(720)
        , client_height_(1280)
        , window_handle_(nullptr)
        , window_rect({ 0 }) {}

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
    u32 client_width_;
    u32 client_height_;
    HWND window_handle_;
    RECT window_rect;
};
