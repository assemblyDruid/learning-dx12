#pragma once

// Local
#include <Toolkit.h>
#include <WindowTools.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // hresult, hwnd
#include <wrl.h>

// DX includes
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

// STL
#include <chrono>
#include <queue>

//
//
// DxContent
//
//
class DxContent
{
    bool
    LoadContent();

    void
    UnloadContent();

    void
    OnUpdate();

    void
    OnRender();

    void
    OnKeyPressed();

    void
    OnMouseWheel();

    void
    OnResize();

private:
    void
    TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
                       Microsoft::WRL::ComPtr<ID3D12Resource>             resource,
                       D3D12_RESOURCE_STATES                              before_state,
                       D3D12_RESOURCE_STATES                              after_state);

    void
    ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
             D3D12_CPU_DESCRIPTOR_HANDLE                        rtv,
             FLOAT*                                             clear_color);

    void
    ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
               D3D12_CPU_DESCRIPTOR_HANDLE                        dsv,
               FLOAT                                              depth = 1.0f);

    void
    UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
                         ID3D12Resource**     desination_resource,
                         ID3D12Resource       intermediate_resource,
                         size_t               num_elements,
                         size_t               element_size,
                         const void*          buffer_data,
                         D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void
    ResizeDepthBuffer();
};

//
//
// DxCommandQueue
//
//
class DxCommandQueue
{
public:
    DxCommandQueue();

    void
    Init(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);

    // [ cfarvin::REVISIT ]
    // virtual ~DxCommandQueue();

    void
    CreateCommandQueue(D3D12_COMMAND_LIST_TYPE list_type);

    void
    CreateFence();

    void
    CreateFenceEventHandle();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>
    GetCommandList();

    u64
    ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list);

    u64
    Signal();

    bool
    IsFenceReady(u64 fence_value);

    void
    AwaitFenceValue(
        u64                       fence_value,
        std::chrono::milliseconds time_out = (std::chrono::milliseconds::max)());

    void
    AwaitGpuIdle();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue>
    CreateCommandQueue();

protected:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
    CreateCommandAllocator();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>
    CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator);

private:
    struct CommandAllocatorEntry
    {
        u64                                            fence_value;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    };

    std::queue<CommandAllocatorEntry> command_allocator_queue_;
    std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> command_list_queue_;
    D3D12_COMMAND_LIST_TYPE                                       type_;
    Microsoft::WRL::ComPtr<ID3D12Device2>                         device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>                    command_queue_;
    Microsoft::WRL::ComPtr<ID3D12Fence>                           fence_;
    HANDLE                                                        fence_event_;
    u64                                                           fence_value_;

    bool is_initialized_;
};

//
//
// Dx
//
//
class Dx
{
public:
    static Dx*
    GetInstance();

    Dx(const Dx&) = delete;

    void
    operator=(const Dx&) = delete;

    void
    Init(HWND window_handle, u32 window_width, u32 window_height, bool use_warp);

    bool
    IsInitialized();

    void
    Update();

    void
    Render();

    void
    ToggleVSync();

    void
    ResizeSwapChain(u32 window_widht, u32 window_height);

    void
    TearDown();

private:
    Dx();

    void
    EnableDebugLayer();

    Microsoft::WRL::ComPtr<IDXGIAdapter4>
    GetAdapter(bool use_warp);

    void
    CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter);

    // void CreateCommandQueue(
    //     D3D12_COMMAND_LIST_TYPE list_type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    void
    GetScreenTearSupport();

    void
    CreateSwapChain(HWND window_handle, u32 window_width, u32 window_height);

    void
    CreateDescriptorHeap();

    void
    UpdateRenderTargetViews();

    // [ cfarvin::revisit ]
    // Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
    //     D3D12_COMMAND_LIST_TYPE type);

    // [ cfarvin::REVISIT ]
    // void CreateCommandList(
    //     Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
    //     D3D12_COMMAND_LIST_TYPE                        type);

    // void CreateFence();

    // void CreateEventHandle();

    // [ cfarvin::REVISIT ]
    // u64 Signal();

    // void AwaitFence(
    //     u64                       await_value,
    //     std::chrono::milliseconds time_out = (std::chrono::milliseconds::max)());

    // void AwaitGpuIdle();

    void
    SetFullScreen(bool full_screen);

    void
    ToggleFullScreen();

    // [ cfarvin::REVISIT ]
    // void CreateCommandAllocatorsAndList(D3D12_COMMAND_LIST_TYPE type);

    static const u8 kNumFrames_ = 3;
    bool            use_warp_;
    bool            is_initialized_;

    // D3D graphics components
    Microsoft::WRL::ComPtr<ID3D12Device2> device_;
    // [ cfarvin::REVISIT ]
    // Microsoft::WRL::ComPtr<ID3D12CommandQueue>        command_queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain_;
    Microsoft::WRL::ComPtr<ID3D12Resource>  back_buffers_[kNumFrames_];
    // [ cfarvin::REVISIT ]
    // Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;
    // Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    command_allocators_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap_;

    DxCommandQueue dx_command_queue_;

    // [ cfarvin::REVISIT ]
    // Sync components
    // Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    // u64                                 fence_value_;
    // u64                                 frame_fence_values_[kNumFrames_];
    // HANDLE                              fence_event_;

    // [ cfarvin::REVISIT ]
    // DxCommandQueue command_queue_manager_;

    UINT rtv_descriptor_size_;
    UINT current_back_buffer_index_;

    // VSync components
    bool is_v_sync_;
    bool is_tearing_supported_;

    // Frame rate measurements
    std::chrono::high_resolution_clock clock_;
    char                               fps_buffer_[512];
    u64                                frame_count_;
    r64                                elapsed_seconds_;

#if __DXDEBUG__ == 1
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue_;
#endif  // __DXDEBUG__ == 1
};
