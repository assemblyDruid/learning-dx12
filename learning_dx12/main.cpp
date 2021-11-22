// DirectX12 Helper Headers
// Must be included before other windows or DX headers.
// Published by Microsoft but not a part of the DX12 SDK.
#include "d3dx12.h"

// Windows Headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>      // Microsoft::WRL::ComPtr<>
#include <shellapi.h> // CommandLineToArgW

// DirectX 12 Headers
#include <d3d12.h>
#include <dxgi1_6.h>     // Microsoft (D)irect(X) (G)raphics (I)nfrastructure
#include <d3dcompiler.h> // Link w/ d3dcompiler.lib
#include <DirectXMath.h>

// STL Headers
#include <cassert>
#include <chrono>
#include <stdio.h>
#include <stdint.h>

#define MAX_FRAMES 10

bool RUNNING = true;
bool ERRORS = false;
bool is_initialized = false;

// The number of swap chain back bufffers
const uint8_t num_frames = 3;

// Option to use the WARP adapter (software raster).
bool use_warp = false;

// Window dimensions.
uint32_t client_width = 1280;
uint32_t client_height = 720;

// Window Handle
HWND window_handle;

// Window Rectangle
RECT window_rect;

// DX12 Objects
using namespace Microsoft::WRL;
ComPtr<ID3D12Device2>             device;
ComPtr<ID3D12CommandQueue>        command_queue;                  // Collection of command lists.
ComPtr<ID3D12Resource>            back_buffers[num_frames];       // Backing memory for swap chain images.
ComPtr<ID3D12GraphicsCommandList> command_list;                   // Stored graphics commands.
ComPtr<ID3D12CommandAllocator>    command_allocators[num_frames]; // Backing memory for command lists.
ComPtr<ID3D12DescriptorHeap>      rtv_descripteeor_heap;          // An array of views in device memory.
ComPtr<IDXGISwapChain4>           swap_chain;                     // A structure of back buffers to display.
UINT                              rtv_descriptor_heap_size;       // IHV determined size of 
UINT                              current_back_buffer_index;      // Current index of back buffers in swap chain.

// Synchronization Ojbects
ComPtr<ID3D12Fence> fence;
uint64_t            fence_value = 0;                     // The next fence value to signal the command queue.
uint64_t            frame_fence_values[num_frames] = {}; // Fence value for potentially in-flight frames.
HANDLE              fence_event;                         // OS event object which notifies that a specific fence value has been reached.

void
RegisterWindowClass()
{

}

void
EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debug_interface;

    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface))))
    {
        puts("[ error ] Unable to enable debug interface.");
        ERRORS = true;
    }
    else
    {
        debug_interface->EnableDebugLayer();
        puts("[ info ] Debug layers enabled.");
    }
#endif
}

void
CLArgMissing(const char* const flag)
{
    printf("Command line argument requires a corresponding value: %s\n", flag);
}

void
ParseCLArgs(const int argc, const char** const argv)
{
    if (nullptr == argv)
    {
        return;
    }

    for (size_t arg_index = 1; arg_index < argc; arg_index++)
    {
        if (0 == strcmp(argv[arg_index], "-w") || 0 == strcmp(argv[arg_index], "--width"))
        {
            if (argc <= (arg_index + 1))
            {
                CLArgMissing("[ -w, --width ]");
                ERRORS = true;
                return;
            }
            else
            {
                client_width = (uint32_t)strtol(argv[++arg_index], nullptr, 10);
            }
        }
        else if (0 == strcmp(argv[arg_index], "-h") || 0 == strcmp(argv[arg_index], "--height"))
        {
            if (argc <= (arg_index + 1))
            {
                CLArgMissing("[ -h, --height ]");
                ERRORS = true;
                return;
            }
            else
            {
                client_height = (uint32_t)strtol(argv[++arg_index], nullptr, 10);
            }
        }
        else if (0 == strcmp(argv[arg_index], "-warp") || 0 == strcmp(argv[arg_index], "--warp"))
        {
            use_warp = true;
        }
        else
        {
            printf("[ error ] Invalid command line argument: %s\n", argv[arg_index]);
            ERRORS = true;
        }
    }
}

int
main(int argc, char** argv)
{
    ParseCLArgs(argc, (const char** const)argv);
    if (ERRORS)
    {
        return -1;
    }

    EnableDebugLayer();
    if (ERRORS)
    {
        return -1;
    }

    uint16_t frames_rendered = 0;
    while (RUNNING)
    {
        frames_rendered++;
        if (MAX_FRAMES >= frames_rendered)
        {
            RUNNING = false;
            break;
        }
    }

    if (ERRORS)
    {
        puts("[ fail ]");
        return -1;
    }

    puts("[ success ]");
    return 0;
}