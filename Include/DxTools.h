#pragma once

// Local
#include <Toolkit.h>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <windows.h>   // hresult
#include <wrl.h>

// DX includes
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

class DXTools
{
public:
    DXTools();
    void
    Init();
    void
    SetUseWarp(bool use_warp);
    bool
    GetUseWarp();

private:
    void
    EnableDebugLayer();

    Microsoft::WRL::ComPtr<IDXGIAdapter4>
    GetAdapter();

    void
    CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter);

    void
    CreateCommandQueue(D3D12_COMMAND_LIST_TYPE list_type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    static const u8 kNumFrames_ = 3;
    bool            use_warp_;
    u32             dx_is_initialized_;

    // D3D graphics components
    Microsoft::WRL::ComPtr<ID3D12Device2>             device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>        command_queue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain>            swap_chain_;
    Microsoft::WRL::ComPtr<ID3D12Resource>            back_buffers_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    command_allocators_[kNumFrames_];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>      rtv_descriptor_heap_;

    // Sync components
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    u64                                 fence_value_;
    u64                                 frame_fence_values_[kNumFrames_];
    HANDLE                              fence_event_;

    UINT rtv_descriptor_size_;
    UINT current_back_buffer_index_;

    // VSync components
    bool is_v_sync_;
    bool is_tearing_supported_;
    bool is_full_screen_;

#if __DXDEBUG__ == 1
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> info_queue_;
#endif   // __DXDEBUG__ == 1
};
