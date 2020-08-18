#include <Dx.h>

//
//
// DxContent
//
//
bool
DxContent::LoadContent()
{
}

void
DxContent::UnloadContent()
{
}

void
DxContent::OnUpdate()
{
}

void
DxContent::OnRender()
{
}

void
DxContent::OnKeyPressed()
{
}

void
DxContent::OnMouseWheel()
{
}

void
DxContent::OnResize()
{
}

void
DxContent::TransitionResource(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
    Microsoft::WRL::ComPtr<ID3D12Resource>             resource,
    D3D12_RESOURCE_STATES                              before_state,
    D3D12_RESOURCE_STATES                              after_state)
{
    Assert(command_list, "Cannot transition a resource for an invalid command list.\n");
    Assert(resource, "Cannot transition an invalid resource.\n");

    CD3DX12_RESOURCE_BARRIER resource_barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), before_state, after_state);
    command_list_->ResourceBarrier(1, &resource_barrier);
}

void
DxContent::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
                    D3D12_CPU_DESCRIPTOR_HANDLE                        rtv,
                    FLOAT*                                             clear_color)
{
    command_list_->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
}

void
DxContent::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
                      D3D12_CPU_DESCRIPTOR_HANDLE                        dsv,
                      FLOAT                                              depth)
{
}

void
DxContent::UpdateBufferResource(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
    ID3D12Resource**                                   desination_resource,
    ID3D12Resource                                     intermediate_resource,
    size_t                                             num_elements,
    size_t                                             element_size,
    const void*                                        buffer_data,
    D3D12_RESOURCE_FLAGS                               flags)
{
}

void
DxContent::ResizeDepthBuffer()
{
}

//
//
// DxCommandQueue
//
//
DxCommandQueue::DxCommandQueue()
    : fence_value_(0)
    , is_initialized_(false)
{
}

void
DxCommandQueue::Init(Microsoft::WRL::ComPtr<ID3D12Device2> device,
                     D3D12_COMMAND_LIST_TYPE               type)
{
    CreateCommandQueue(type);
    CreateFence();
    CreateFenceEventHandle();

    is_initialized_ = true;
}

void
DxCommandQueue::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE list_type)
{
    Assert(device_,
           "KCannot create the command queue before a valid device "
           "has been created.");

    // Fill out queue description
    D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
    command_queue_desc.Type                     = list_type;  // [ cfarvin::TODO ]
    command_queue_desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask                 = 0;

    // Create queue
    Require(
        device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_)),
        "Could not create the command queue.");
}

void
DxCommandQueue::CreateFence()
{
    Assert(device_, "Cannot create a fence without first creating a device.\n");
    Require(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
            "Could not create a fence.\n");
}

void
DxCommandQueue::CreateFenceEventHandle()
{
    fence_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    Assert(fence_event_, "Could not create fence event.");
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>
DxCommandQueue::CreateCommandList(
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator)
{
    Assert(device_, "Cannot create a command list without first creating a device.\n");
    Assert(type_, "Cannot create a command list with an invalid type.\n");
    Assert(command_allocator,
           "Cannot create a command list without first creating a command alloctor.\n");

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list;
    Require(device_->CreateCommandList(0,
                                       type_,
                                       command_allocator.Get(),
                                       nullptr,
                                       IID_PPV_ARGS(&command_list)),
            "Could not create a command list.\n");

    // [ cfarvin::REVISIT ]
    // Close, so that the list can be reset immediately by the caller.
    // Require(command_list->Close());

    return command_list;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>
DxCommandQueue::GetCommandList()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>     command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list;

    // Obtain a command allocator
    const CommandAllocatorEntry* entry = &(command_allocator_queue_.front());
    if (!command_allocator_queue_.empty() && IsFenceReady(entry->fence_value))
    {
        command_allocator = entry->command_allocator;
        command_allocator_queue_.pop();
        Require(command_allocator->Reset(), "Could not reset a command allocator.\n");
    }
    else
    {
        command_allocator = CreateCommandAllocator();
    }

    // Obtain a command list
    if (!command_list_queue_.empty())
    {
        command_list = command_list_queue_.front();
        command_list_queue_.pop();
        Require(command_list->Reset(command_allocator.Get(), nullptr),
                "Could not reset a command list.\n");
    }
    else
    {
        command_list = CreateCommandList(command_allocator);
    }

    Require(command_list->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator),
                                                  command_allocator.Get()),
            "Could not associate [ command allocator, command list ].\n");

    return command_list;
}

u64
DxCommandQueue::ExecuteCommandList(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list)
{
    Assert(command_list, "Cannot execute an invalid command list.\n");
    Assert(command_queue_,
           "Cannot execute a command list without a valid command queue.\n");

    // Get the command allocator from the private data of the command list
    command_list->Close();
    ID3D12CommandAllocator* command_allocator;
    UINT                    data_size = sizeof(command_allocator);
    Require(command_list->GetPrivateData(__uuidof(ID3D12CommandAllocator),
                                         &data_size,
                                         &command_allocator));

    // Execute the command list
    ID3D12CommandList* const command_lists[] = {command_list.Get()};
    command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists);

    // Enqueue
    u64 await_value = Signal();
    command_allocator_queue_.emplace(
        CommandAllocatorEntry{await_value, command_allocator});
    command_list_queue_.push(command_list);

    command_allocator->Release();

    return await_value;
}

u64
DxCommandQueue::Signal()
{
    Assert(command_queue_, "Cannot signal a fence without a valid command queue.\n");
    Assert(fence_, "Cannot signal an invalid fence.\n");

    u64 cpu_wait_value = ++fence_value_;
    Require(command_queue_->Signal(fence_.Get(), cpu_wait_value));

    return cpu_wait_value;
}

bool
DxCommandQueue::IsFenceReady(u64 await_value)
{
    return fence_->GetCompletedValue() >= await_value;
}

void
DxCommandQueue::AwaitFenceValue(u64 await_value, std::chrono::milliseconds time_out)
{
    Assert(fence_, "Cannot await an invalid fence.\n");
    Assert(fence_event_, "Cannot await an invalid fence event.\n");
    if (!IsFenceReady(await_value))
    {
        Require(fence_->SetEventOnCompletion(await_value, fence_event_));
        WaitForSingleObject(fence_event_, static_cast<DWORD>(time_out.count()));
    }
}

void
DxCommandQueue::AwaitGpuIdle()
{
    Assert(command_queue_, "Cannot await based on an invalid command queue.\n");
    Assert(fence_, "Cannot await an invalid fence.\n");
    Assert(fence_event_, "Cannot await based on an invalid fence event handle.\n");

    u64 cpu_wait_value = DxCommandQueue::Signal();
    DxCommandQueue::AwaitFenceValue(cpu_wait_value);
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue>
DxCommandQueue::CreateCommandQueue()
{
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator>
DxCommandQueue::CreateCommandAllocator()
{
    Assert(device_,
           "Cannot create a command allocator without first creating a device.\n");
    Assert(type_, "Cannot create a command allocator with an invalid type.\n");
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    Require(device_->CreateCommandAllocator(type_, IID_PPV_ARGS(&command_allocator)));

    return command_allocator;
}

//
//
// Dx
//
//
Dx*
Dx::GetInstance()
{
    static Dx instance;
    return &instance;
}

Dx::Dx()
    : is_initialized_(false)
    , device_(nullptr)
    , swap_chain_(nullptr)
    , rtv_descriptor_heap_(nullptr)
    , rtv_descriptor_size_(0)
    , current_back_buffer_index_(0)
    , is_v_sync_(true)
    , is_tearing_supported_(false)
#if __DXDEBUG__
    , info_queue_(nullptr)
#endif  // __DXDEBUG__
    , clock_()
    , fps_buffer_()
    , frame_count_(0)
    , elapsed_seconds_(0)
{
}

void
Dx::TearDown()
{
    // [ cfarvin::REVISIT ]
    // command_queue_manager_.AwaitGpuIdle();
    // CloseHandle(fence_event_);
}

void
Dx::Init(HWND window_handle, u32 window_width, u32 window_height, bool use_warp)
{
    // [ cfarvin::REVISIT ]
    EnableDebugLayer();  // no-op on release builds
    CreateDevice(GetAdapter(use_warp));
    CreateDescriptorHeap();
    GetScreenTearSupport();
    CreateSwapChain(window_handle, window_width, window_height);
    UpdateRenderTargetViews();

    is_initialized_ = true;
}

bool
Dx::IsInitialized()
{
    return is_initialized_;
}

void
Dx::EnableDebugLayer()
{
#if __DXDEBUG__ == 1
    Microsoft::WRL::ComPtr<ID3D12Debug>  tmp_dx_debug;
    Microsoft::WRL::ComPtr<ID3D12Debug1> dx_debug;
    Require(D3D12GetDebugInterface(IID_PPV_ARGS(&tmp_dx_debug)),
            "Could not enable debug layer.\n");
    Require(tmp_dx_debug->QueryInterface(IID_PPV_ARGS(&dx_debug)));
    dx_debug->EnableDebugLayer();
    dx_debug->SetEnableGPUBasedValidation(true);
    LogManager::Log("DX debug enabled.", LogType::kInfo);
#endif  // __DXDEBUG__ == 1
}

Microsoft::WRL::ComPtr<IDXGIAdapter4>
Dx::GetAdapter(bool use_warp)
{
    Microsoft::WRL::ComPtr<IDXGIFactory7> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter;

    // Create the DXGI factory
    {
        UINT factory_creation_flags = 0;
#if __DXDEBUG__ == 1
        factory_creation_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif  // __DXDEBUG__ == 1
        Require(CreateDXGIFactory2(factory_creation_flags, IID_PPV_ARGS(&factory)),
                "Could not create dxgi factory.");
    }

    // Find the best adapter
    {
        DXGI_ADAPTER_DESC3 adapter_description;
        if (use_warp)
        {
            Require(factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter)),
                    "Could not enumerate warp adapters.");
            dxgi_adapter->GetDesc3(&adapter_description);
            Require((bool)(static_cast<size_t>(adapter_description.Flags) &
                           static_cast<size_t>(DXGI_ADAPTER_FLAG_SOFTWARE)),
                    "Warp adapter description did not reflect expected flags.");
        }
        else
        {
            Require(
                factory->EnumAdapterByGpuPreference(0,
                                                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                    IID_PPV_ARGS(&dxgi_adapter)),
                "Could not eumerate GPUs by preference.");
            dxgi_adapter->GetDesc3(&adapter_description);
            Require((bool)!(static_cast<size_t>(adapter_description.Flags) &
                            static_cast<size_t>(DXGI_ADAPTER_FLAG_SOFTWARE)),
                    "Adapter description did not reflect expected flags.");
        }

        // Ensure that the device _can_ be created with the acquired adapter,
        // without creating it yet.
        Require(D3D12CreateDevice(dxgi_adapter.Get(),
                                  D3D_FEATURE_LEVEL_12_0,
                                  __uuidof(ID3D12Device),
                                  nullptr),
                "Could not create the test device.");
    }

    return dxgi_adapter;
}

void
Dx::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter)
{
    Assert(dxgi_adapter, "An invalid dxgi adapter was provided.");
    Require(D3D12CreateDevice(dxgi_adapter.Get(),
                              D3D_FEATURE_LEVEL_12_0,
                              IID_PPV_ARGS(&device_)));

#if __DXDEBUG__ == 1
    Require(device_.As(&info_queue_),
            "Could not query info queue interface from device.");
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif  // __DXDEBUG__ == 1
}

void
Dx::GetScreenTearSupport()
{
    // Note: As of time of writing, Microsoft reccommends creating a
    // IDXGIFactory4 first and querying the IDXGIFactory5 interface afterwards.
    // This is a workaround for unimplented features. The implementation below
    // is only slightly modified from a MSDN example.
    Microsoft::WRL::ComPtr<IDXGIFactory4> tmp_dxgi_factory;
    Microsoft::WRL::ComPtr<IDXGIFactory5> dxgi_factory;
    Require(CreateDXGIFactory1(IID_PPV_ARGS(&tmp_dxgi_factory)),
            "Could not create a dxgi factory.");
    Require(tmp_dxgi_factory.As(&dxgi_factory));

    bool    allow_tearing_feature_info = false;
    HRESULT result =
        dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                          &allow_tearing_feature_info,
                                          sizeof(allow_tearing_feature_info));
    is_tearing_supported_ = SUCCEEDED(result) && allow_tearing_feature_info;
}

void
Dx::CreateSwapChain(HWND window_handle, u32 window_width, u32 window_height)
{
    Microsoft::WRL::ComPtr<IDXGIFactory7> factory;

    // Create the DXGI factory
    {
        UINT factory_creation_flags = 0;
#if __DXDEBUG__ == 1
        factory_creation_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif  // __DXDEBUG__ == 1
        Require(CreateDXGIFactory2(factory_creation_flags, IID_PPV_ARGS(&factory)),
                "Could not create dxgi factory.");
    }

    DXGI_SWAP_CHAIN_DESC1 swap_chain_description = {};
    // Fill in the swap chain description
    {
        swap_chain_description.Width       = window_width;
        swap_chain_description.Height      = window_height;
        swap_chain_description.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_description.Stereo      = FALSE;
        swap_chain_description.SampleDesc  = {1, 0};
        swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_description.BufferCount = kNumFrames_;
        swap_chain_description.Scaling     = DXGI_SCALING_STRETCH;
        swap_chain_description.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_description.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_description.Flags =
            is_tearing_supported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain1> tmp_swap_chain;
    Require(factory->CreateSwapChainForHwnd(command_queue_.Get(),
                                            window_handle,
                                            &swap_chain_description,
                                            nullptr,
                                            nullptr,
                                            &tmp_swap_chain));

    // Set swap chain
    Require(tmp_swap_chain.As(&swap_chain_));
    current_back_buffer_index_ = swap_chain_->GetCurrentBackBufferIndex();
}

void
Dx::CreateDescriptorHeap()
{
    Assert(device_,
           "Cannot create the descriptor heap until the device has "
           "been created.\n");

    // Fill in the descriptor heap description
    D3D12_DESCRIPTOR_HEAP_DESC heap_description = {};
    {
        heap_description.NumDescriptors = kNumFrames_;
        heap_description.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
    Require(
        device_->CreateDescriptorHeap(&heap_description, IID_PPV_ARGS(&descriptor_heap)));

    rtv_descriptor_heap_ = descriptor_heap;
}

void
Dx::UpdateRenderTargetViews()
{
    // Note: sizes of descriptor heaps are vendor specific.
    Assert(device_, "A valid device is required to update render target views.\n");
    rtv_descriptor_size_ =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Note: CD3DX12_CPU_DESCRIPTOR_HANDLE expands D3DX12_CPU_DESCRIPTOR_HANDLE
    // in d3dx12.h, improving iterability
    Assert(rtv_descriptor_heap_,
           "A valid descriptor heap is requried to update render target views.\n");
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
        rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

    Assert(swap_chain_,
           "A valid swap chain is required to update render target views.\n");
    for (UINT frame_idx = 0; frame_idx < kNumFrames_; frame_idx++)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer;
        Require(swap_chain_->GetBuffer(frame_idx, IID_PPV_ARGS(&back_buffer)),
                "Could not get buffer from swap chain.\n");

        device_->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_handle);
        back_buffers_[frame_idx] = back_buffer;
        rtv_handle.Offset(rtv_descriptor_size_);
    }
}

// [ cfarvin::REVISIT ]
// void Dx::CreateFence()
// {
//     Assert(device_, "Cannot create a fence without first creating a device.\n");
//     Require(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)),
//             "Could not create a fence.\n");
// }

// void Dx::CreateEventHandle()
// {
//     fence_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
//     Assert(fence_event_, "Could not create fence event.");
// }

// [ cfarvin::REVISIT ]
// u64 Dx::Signal()
// {
//     Assert(command_queue_, "Cannot signal a fence without a valid command queue.\n");
//     Assert(fence_, "Cannot signal an invalid fence.\n");

//     u64 cpu_wait_value = ++fence_value_;
//     Require(command_queue_->Signal(fence_.Get(), cpu_wait_value));

//     return cpu_wait_value;
// }

// [ cfarvin::REVISIT ]
// void Dx::AwaitFence(u64 await_value, std::chrono::milliseconds time_out)
// {
//     Assert(fence_, "Cannot await an invalid fence.\n");
//     Assert(fence_event_, "Cannot await an invalid fence event.\n");
//     if (fence_->GetCompletedValue() < await_value)
//     {
//         Require(fence_->SetEventOnCompletion(await_value, fence_event_));
//         WaitForSingleObject(fence_event_, static_cast<DWORD>(time_out.count()));
//     }
// }

// [ cfarvin::REVISIT ]
// void Dx::AwaitGpuIdle()
// {
//     Assert(command_queue_, "Cannot await based on an invalid command queue.\n");
//     Assert(fence_, "Cannot await an invalid fence.\n");
//     Assert(fence_event_, "Cannot await based on an invalid fence event handle.\n");

//     u64 cpu_wait_value = Dx::Signal();
//     Dx::AwaitFence(cpu_wait_value);
// }

void
Dx::Update()
{
    //
    // Begin frame rate measurements
    static auto t0 = clock_.now();
    //
    //

    //
    // End frame rate measurements
    frame_count_++;
    auto t1    = clock_.now();
    auto delta = t1 - t0;
    t0         = t1;
    elapsed_seconds_ += (delta.count() * 1e-9);  // nanoseconds to seconds
    if (elapsed_seconds_ > 1.0)
    {
        r64 fps = frame_count_ / elapsed_seconds_;
        // sprintf_s(fps_buffer_, 512, "fps: %f\n", fps);
        // puts(fps_buffer_);
        std::cout << "fps: " << fps << std::endl;

        frame_count_     = 0;
        elapsed_seconds_ = 0;
    }
    //
    //
}

void
Dx::Render()
{
    Assert(command_allocators_[0], "Cannot render with invalid command allocators.\n");
    Assert(back_buffers_[0], "Cannot render with invalid back buffers.\n");
    Assert(command_list_, "Cannot render with an invalid command list.\n");

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator =
        command_allocators_[current_back_buffer_index_];
    Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer =
        back_buffers_[current_back_buffer_index_];

    // Prepare to record next frame command list
    {
        command_allocator->Reset();
        command_list_->Reset(command_allocator.Get(), nullptr);
    }

    // Transition to render target state
    {
        // [ cfarvin::REVISIT
        //DxContent::TransitionResource
    }

    // Clear the render target
    {
        r32                           clear_color[] = {0.4f, 0.6f, 0.9f, 1.0f};
        CD3DX12_CPU_DESCRIPTOR_HANDLE render_target_view(
            rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
            current_back_buffer_index_,
            rtv_descriptor_size_);
        command_list_->ClearRenderTargetView(render_target_view, clear_color, 0, nullptr);
    }

    // Transition to present state
    {
        CD3DX12_RESOURCE_BARRIER resource_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            back_buffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,  // "before" state
            D3D12_RESOURCE_STATE_PRESENT);       // "after" state
        command_list_->ResourceBarrier(1, &resource_barrier);
    }

    // Execute command lists
    {
        Require(command_list_->Close());
        ID3D12CommandList* const command_lists[] = {command_list_.Get()};
        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists);
    }

    // Present
    {
        UINT sync_interval = is_v_sync_ ? 1 : 0;
        UINT present_flags =
            is_tearing_supported_ && !is_v_sync_ ? DXGI_PRESENT_ALLOW_TEARING : 0;
        Require(swap_chain_->Present(sync_interval, present_flags));

        // Query new fence value & store for current back buffer
        u64 cpu_await_value                             = Dx::Signal();
        frame_fence_values_[current_back_buffer_index_] = cpu_await_value;

        // Update back buffer index & wait for this frame to be presented
        current_back_buffer_index_ = swap_chain_->GetCurrentBackBufferIndex();
        AwaitFence(cpu_await_value);
    }
}

void
Dx::ResizeSwapChain(u32 window_width, u32 window_height)
{
    AwaitGpuIdle();

    for (size_t frame_idx = 0; frame_idx < kNumFrames_; frame_idx++)
    {
        // Release back buffer references
        back_buffers_[frame_idx].Reset();

        // Set all fence values to that of the current frame
        frame_fence_values_[frame_idx] = frame_fence_values_[current_back_buffer_index_];
    }

    // Resize swap chain
    DXGI_SWAP_CHAIN_DESC swap_chain_description = {};
    Require(swap_chain_->GetDesc(&swap_chain_description));
    Require(swap_chain_->ResizeBuffers(kNumFrames_,
                                       window_width,
                                       window_height,
                                       swap_chain_description.BufferDesc.Format,
                                       swap_chain_description.Flags));

    current_back_buffer_index_ = swap_chain_->GetCurrentBackBufferIndex();
    UpdateRenderTargetViews();
}

void
Dx::ToggleVSync()
{
    is_v_sync_ = !is_v_sync_;
}

// [ cfarvin::REVISIT ]
// void Dx::CreateCommandAllocatorsAndList(D3D12_COMMAND_LIST_TYPE type)
// {
//     // Command allocators
//     for (size_t frame_idx = 0; frame_idx < kNumFrames_; frame_idx++)
//     {
//         command_allocators_[frame_idx] = CreateCommandAllocator(type);
//     }

//     // Command list
//     CreateCommandList(command_allocators_[current_back_buffer_index_], type);
// }
