#include <Controller.h>

static Dx*          dx      = Dx::GetInstance();
static WindowTools* window  = WindowTools::GetInstance();
static bool         running = true;

//
// Global window proc definition
LRESULT CALLBACK
WindowProcedure(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
    if (window->IsInitialized() && dx->IsInitialized())
    {
        switch (u_msg)
        {
        case WM_PAINT:
        {
            dx->Update();
            dx->Render();
            break;
        }  // case WM_PAINT

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            bool alt_key_down = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            switch (w_param)
            {
            case 'V':
            {
                dx->ToggleVSync();
                break;
            }  // case 'V'

            case VK_ESCAPE:
            {
                PostQuitMessage(0);
                break;
            }  // case VK_ESCAPE

            case VK_RETURN:
            {
                if (alt_key_down)
                {
                case VK_F11:
                {
                    window->ToggleFullScreen();
                    dx->ResizeSwapChain(window->GetWindowWidth(),
                                        window->GetWindowHeight());
                    break;
                }
                }
            }  // case VK_RETURN
            }
        }  // case WM_SYSKEYDOWN, WM_KEYDOWN

        // Avoid system notification sound for unhandled case
        case WM_SYSCHAR:
        {
            break;
        }  // case WM_SYSCHAR

        case WM_SIZE:
        {
            window->UpdateWindowDemensions();
            dx->ResizeSwapChain(window->GetWindowWidth(), window->GetWindowHeight());
            break;
        }  // case WM_SIZE

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            running = false;
            break;
        }  // WM_DESTROY

        default:
        {
            return DefWindowProc(hwnd, u_msg, w_param, l_param);
        }  // DEFAULT
        }  // switch

        return 0;
    }  // if

    return DefWindowProc(hwnd, u_msg, w_param, l_param);
}
// Global window proc definition
//

Controller::Controller()
{
}

Controller*
Controller::GetInstance()
{
    static Controller instance;
    return &instance;
}

void
Controller::Init(u32                window_width,
                 u32                window_height,
                 bool               use_warp,
                 const std::string& window_class_name,
                 const std::string& window_title)
{
    if (!DirectX::XMVerifyCPUSupport())
    {
        LogManager::Log("Could not verify CPU support for DirectX math.\n",
                        LogType::kError);
    }

    window->Init(WindowProcedure,
                 window_width,
                 window_height,
                 window_class_name,
                 window_title);
    dx->Init(window->GetWindowHandle(), window_width, window_height, use_warp);
}

bool
Controller::IsRunning()
{
    return running;
}

void
Controller::GameLoop()
{
    MSG message = {};
    while (running)
    {
        if (message.message == WM_QUIT)
            running = false;
        else if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    dx->TearDown();
}
