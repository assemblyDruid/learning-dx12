// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Local
#include <Controller.h>
#include <Dx.h>
#include <WindowTools.h>
#include <toolkit.h>

// STL
#include <string>
#include <vector>

int
main(int argc, char** argv)
{
    Controller* controller = Controller::GetInstance();

    // Default values
    u32         _window_width      = 1280;
    u32         _window_height     = 720;
    bool        _use_warp          = false;
    std::string _window_class_name = "DxSample";
    std::string _window_title      = "DxSample";

    // Parse command line
    if (argc && argv)
    {
        std::vector<std::string> invalid_options;
        for (size_t cmd_arg_idx = 1; cmd_arg_idx < static_cast<size_t>(argc);
             cmd_arg_idx++)
        {
            // Get cmd arg string
            if (!strlen(argv[cmd_arg_idx]))
                break;

            bool valid_option_found = false;

            // Check for width argument
            if (strcmp(argv[cmd_arg_idx], "-w") == 0 ||
                strcmp(argv[cmd_arg_idx], "--width") == 0)
            {
                valid_option_found = true;
                if (cmd_arg_idx < argc)
                    cmd_arg_idx++;
                u32 window_width = std::stol(argv[cmd_arg_idx], nullptr, 10);
                if (window_width)
                {
                    _window_width = window_width;
                }
            }

            // Check for height argument
            if (strcmp(argv[cmd_arg_idx], "-h") == 0 ||
                strcmp(argv[cmd_arg_idx], "--height") == 0)
            {
                valid_option_found = true;
                if (cmd_arg_idx < argc)
                    cmd_arg_idx++;
                u32 window_height = std::stol(argv[cmd_arg_idx], nullptr, 10);
                if (window_height)
                {
                    _window_width = window_height;
                }
            }

            // Check for windows advanced rasterization platform
            if (strcmp(argv[cmd_arg_idx], "-warp") == 0 ||
                strcmp(argv[cmd_arg_idx], "--warp") == 0)
            {
                valid_option_found = true;
                _use_warp          = true;
            }

            // Log invalid options
            if (!valid_option_found)
            {
                invalid_options.push_back(argv[cmd_arg_idx]);
            }
        }

        size_t invalid_options_size = invalid_options.size();
        if (invalid_options_size)
        {
            std::cerr << "The following options were not recognized:" << std::endl;
            for (size_t invalid_idx = 0; invalid_idx < invalid_options.size();
                 invalid_idx++)
            {
                std::cerr << '\t' << invalid_idx << ": " << invalid_options[invalid_idx]
                          << std::endl;
            }
            return -1;
        }
    }

    // Set DPI awareness
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Apply command line options
    controller->Init(_window_width,
                     _window_height,
                     _use_warp,
                     _window_class_name,
                     _window_title);

    controller->GameLoop();

    std::cout << "[ success ]" << std::endl;
    return 0;
}
