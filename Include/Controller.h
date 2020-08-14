#pragma once

// Local
#include <DxTools.h>
#include <Toolkit.h>
#include <WindowTools.h>

class Controller
{
public:
    static Controller* GetInstance();

    Controller(const Controller&) = delete;

    void operator=(const Controller&) = delete;

    void Init(u32                window_width,
              u32                window_height,
              bool               use_warp,
              const std::string& window_class_name,
              const std::string& window_title);

    static LRESULT CALLBACK WindowProcedure(HWND   hwnd,
                                            UINT   u_msg,
                                            WPARAM w_param,
                                            LPARAM l_param);

    bool IsRunning();

    void GameLoop();

private:
    Controller();
};
