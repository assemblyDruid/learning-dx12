// Windows includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Local
#include <toolkit.h>


int main(int argc, char** argv)
{
    DXTools dx = DXTools();
    WindowTools window = WindowTools();

    // Parse command line
    if (argc && argv)
    {
        for (size_t cmd_arg_idx = 0; cmd_arg_idx < argc; cmd_arg_idx++)
        {
            // Get cmd arg strign
            std::string cmd_arg_str(argv[cmd_arg_idx]);
            if (!cmd_arg_str.length()) break;

            // Check for width argument
            if (strcmp(argv[cmd_arg_idx], "-w") == 0 ||
                strcmp(argv[cmd_arg_idx], "--width") == 0)
            {
                u32 client_width = std::stol(cmd_arg_str, nullptr, 10);
                if (client_width)
                {
                    window.SetClientWidth(client_width);
                }
            }

            // Check for height argument
            if (strcmp(argv[cmd_arg_idx], "-h") == 0 ||
                strcmp(argv[cmd_arg_idx], "--height") == 0)
            {
                u32 client_height = std::stol(cmd_arg_str, nullptr, 10);
                if (client_height)
                {
                    window.SetClientHeight(client_height);
                }
            }

            // Check for windows advanced rasterization platform
            if (strcmp(argv[cmd_arg_idx], "-warp") == 0 ||
                strcmp(argv[cmd_arg_idx], "--warp") == 0)
            {
                dx.SetUseWarp(true);
            }
        }
    }

    std::cout << "[ success ]" << std::endl;
    return 0;
}
