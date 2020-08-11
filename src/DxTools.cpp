#include <DxTools.h>

DXTools::DXTools()
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
DXTools::Init()
{
    EnableDebugLayer();   // no-op on release builds
    CreateDevice(GetAdapter());
    CreateCommandQueue();

    dx_is_initialized_ = true;
}

void
DXTools::SetUseWarp(bool use_warp)
{
    use_warp_ = use_warp;
}

bool
DXTools::GetUseWarp()
{
    return use_warp_;
}

void
DXTools::EnableDebugLayer()
{
#if __DXDEBUG__ == 1
    Microsoft::WRL::ComPtr<ID3D12Debug> dx_debug_;
    Assert(D3D12GetDebugInterface(IID_PPV_ARGS(&dx_debug_)), "Could not enable debug layer.\n");
    dx_debug_->EnableDebugLayer();
    LogManager::Log("DX debug enabled.", LogType::kInfo);
#endif   // __DXDEBUG__ == 1
}

Microsoft::WRL::ComPtr<IDXGIAdapter4>
DXTools::GetAdapter()
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
            Assert((bool)(adapter_description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE),
                   "Warp adapter description did not reflect expected flags.");
        }
        else
        {
            Assert(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                       IID_PPV_ARGS(&dxgi_adapter)),
                   "Could not eumerate GPUs by preference.");
            dxgi_adapter->GetDesc3(&adapter_description);
            Assert((bool)!(adapter_description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE),
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
DXTools::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgi_adapter)
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
DXTools::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE list_type)
{
    Assert(device_, "KCannot create the command queue before a valid device has been created.");

    // Fill out queue description
    D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
    command_queue_desc.Type                     = list_type;   // [ cfarvin::TODO ]
    command_queue_desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask                 = 0;

    // Create queue
    Assert(device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_)));
}
