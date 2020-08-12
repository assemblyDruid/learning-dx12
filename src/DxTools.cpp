#include <DxTools.h>

DxTools::DxTools()
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
#if __DXDEBUG__
    , info_queue_(nullptr)
#endif   // __DXDEBUG__
{
}

void
DxTools::Init(HWND hwnd, u32 window_width, u32 window_height)
{
    EnableDebugLayer();   // no-op on release builds
    CreateDevice(GetAdapter());
    CreateCommandQueue();
    GetScreenTearSupport();
    CreateSwapChain(hwnd, window_width, window_height);

    dx_is_initialized_ = true;
}

void
DxTools::SetUseWarp(bool use_warp)
{
    use_warp_ = use_warp;
}

bool
DxTools::GetUseWarp()
{
    return use_warp_;
}

void
DxTools::EnableDebugLayer()
{
#if __DXDEBUG__ == 1
    Microsoft::WRL::ComPtr<ID3D12Debug>  tmp_dx_debug;
    Microsoft::WRL::ComPtr<ID3D12Debug1> dx_debug;
    Assert(D3D12GetDebugInterface(IID_PPV_ARGS(&tmp_dx_debug)), "Could not enable debug layer.\n");
    Assert(tmp_dx_debug->QueryInterface(IID_PPV_ARGS(&dx_debug)));
    dx_debug->EnableDebugLayer();
    dx_debug->SetEnableGPUBasedValidation(true);
    LogManager::Log("DX debug enabled.", LogType::kInfo);
#endif   // __DXDEBUG__ == 1
}

Microsoft::WRL::ComPtr<IDXGIAdapter4>
DxTools::GetAdapter()
{
    Microsoft::WRL::ComPtr<IDXGIFactory7> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter;

    // Create the DXGI factory
    {
        UINT factory_creation_flags = 0;
#if __DXDEBUG__ == 1
        factory_creation_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif   // __DXDEBUG__ == 1
        Assert(CreateDXGIFactory2(factory_creation_flags, IID_PPV_ARGS(&factory)),
               "Could not create dxgi factory.");
    }

    // Find the best adapter
    {
        DXGI_ADAPTER_DESC3 adapter_description;
        if (use_warp_)
        {
            Assert(factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter)),
                   "Could not enumerate warp adapters.");
            dxgi_adapter->GetDesc3(&adapter_description);
            Assert((bool)(static_cast<size_t>(adapter_description.Flags) &
                          static_cast<size_t>(DXGI_ADAPTER_FLAG_SOFTWARE)),
                   "Warp adapter description did not reflect expected flags.");
        }
        else
        {
            Assert(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                       IID_PPV_ARGS(&dxgi_adapter)),
                   "Could not eumerate GPUs by preference.");
            dxgi_adapter->GetDesc3(&adapter_description);
            Assert((bool)!(static_cast<size_t>(adapter_description.Flags) &
                           static_cast<size_t>(DXGI_ADAPTER_FLAG_SOFTWARE)),
                   "Adapter description did not reflect expected flags.");
        }

        // Ensure that the device _can_ be created with the acquired adapter, without creating it
        // yet.
        Assert(D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device),
                                 nullptr),
               "Could not create the test device.");
    }

    return dxgi_adapter;
}

void
DxTools::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter)
{
    Assert(dxgi_adapter, "An invalid dxgi adapter was provided.");
    Assert(D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_)));

#if __DXDEBUG__ == 1
    Assert(device_.As(&info_queue_), "Could not query info queue interface from device.");
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif   // __DXDEBUG__ == 1
}

void
DxTools::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE list_type)
{
    Assert(device_, "KCannot create the command queue before a valid device has been created.");

    // Fill out queue description
    D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
    command_queue_desc.Type                     = list_type;   // [ cfarvin::TODO ]
    command_queue_desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask                 = 0;

    // Create queue
    Assert(device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_)),
           "Could not create the command queue.");
}

void
DxTools::GetScreenTearSupport()
{
    // Note: As of time of writing, Microsoft reccommends creating a IDXGIFactory4 first and
    // querying the IDXGIFactory5 interface afterwards. This is a workaround for unimplented
    // features. The implementation below is only slightly modified from a MSDN example.
    Microsoft::WRL::ComPtr<IDXGIFactory4> tmp_dxgi_factory;
    Microsoft::WRL::ComPtr<IDXGIFactory5> dxgi_factory;
    Assert(CreateDXGIFactory1(IID_PPV_ARGS(&tmp_dxgi_factory)), "Could not create a dxgi factory.");
    Assert(tmp_dxgi_factory.As(&dxgi_factory));

    bool    allow_tearing_feature_info = false;
    HRESULT result        = dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                       &allow_tearing_feature_info,
                                                       sizeof(allow_tearing_feature_info));
    is_tearing_supported_ = SUCCEEDED(result) && allow_tearing_feature_info;
}

void
DxTools::CreateSwapChain(HWND hwnd, u32 window_width, u32 window_height)
{
    Microsoft::WRL::ComPtr<IDXGIFactory7> factory;

    // Create the DXGI factory
    {
        UINT factory_creation_flags = 0;
#if __DXDEBUG__ == 1
        factory_creation_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif   // __DXDEBUG__ == 1
        Assert(CreateDXGIFactory2(factory_creation_flags, IID_PPV_ARGS(&factory)),
               "Could not create dxgi factory.");
    }

    DXGI_SWAP_CHAIN_DESC1 swap_chain_description = {};
    // Fill in the swap chain description
    {
        swap_chain_description.Width       = window_width;
        swap_chain_description.Height      = window_height;
        swap_chain_description.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_description.Stereo      = FALSE;
        swap_chain_description.SampleDesc  = { 1, 0 };
        swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_description.BufferCount = kNumFrames_;
        swap_chain_description.Scaling     = DXGI_SCALING_STRETCH;
        swap_chain_description.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_description.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_description.Flags =
            is_tearing_supported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain1> tmp_swap_chain;
    Assert(factory->CreateSwapChainForHwnd(command_queue_.Get(), hwnd, &swap_chain_description,
                                           nullptr, nullptr, &tmp_swap_chain));

    // Set swap chain
    Assert(tmp_swap_chain.As(&swap_chain_));
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
DxTools::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 num_descriptors)
{
    Assert(device_, "Cannot create the descriptor heap until the device has been created.\n");

    // Fill in the descriptor heap description
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    {
        heap_description.NumDescriptors = num_descriptors;
        heap_description.Type           = type;
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    Assert(device_->CreateDescriptorHeap(&heap_description, IID_PPV_ARGS(&descriptor_heap)));

    return descriptor_heap;
}

void
DxTools::UpdateRenderTargetViews()
{
    // Note: sizes of descriptor heaps are vendor specific.
    Assert(device_, "A valid device is required to update render target views.\n");
    UINT handle_increment_size =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Note: CD3DX12_CPU_DESCRIPTOR_HANDLE expands D3DX12_CPU_DESCRIPTOR_HANDLE in d3dx12.h,
    // improving iterability
    Assert(rtv_descriptor_heap_,
           "A valid descriptor heap is requried to update render target views.\n");
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
        rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

    Assert(swap_chain_, "A valid swap chain is required to update render target views.\n");
    for (UINT frame_idx = 0; frame_idx < kNumFrames_; frame_idx++)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer;
        Assert(swap_chain_->GetBuffer(frame_idx, IID_PPV_ARGS(&back_buffer)),
               "Could not get buffer from swap chain.\n");

        device_->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);
        back_buffers_[frame_idx] = back_buffer;
        rtv_handle.Offset(handle_increment_size);
    }
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
DxTools::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    Assert(device_, "Cannot create a command allocator without first creating a device.\n");
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    Assert(device_->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator)));

    return command_allocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>
DxTools::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
                           D3D12_COMMAND_LIST_TYPE                        type)
{
    Assert(device_, "Cannot create a command list without first creating a device.\n");
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    Assert(device_->CreateCommandList(0, type, command_allocator.Get(), nullptr,
                                      IID_PPV_ARGS(&command_list)));

    // Close, so that the list can be reset immediately by the caller.
    Assert(command_list->Close());

    return command_list;
}

void
DxTools::CreateFence()
{
    Assert(device_, "Cannot create a fence without first creating a device.\n");
    Assert(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
           "Could not create a fence.\n");
}

void
DxTools::CreateEventHandle()
{
    fence_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    Assert(fence_event_, "Could not create fence event.");
}

// u64
// DxTools::Signal()
// {
//     Assert(command_queue_, "Cannot signal a fence without a valid command queue.\n");
//     Assert(fence_, "Cannot signal an invalid fence .\n");
// }
