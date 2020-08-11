#pragma once

#include <Toolkit.h>

void
LogManager::Log(const std::string& msg)
{
    std::cout << "[ log ] " << msg << std::endl;
}

void
LogManager::Log(const std::string& msg, LogType log_type)
{
    switch (log_type)
    {
        case LogType::kInfo:
        {
            std::cout << "[ info ] " << msg << std::endl;
            break;
        }
        case LogType::kWarning:
        {
            std::cout << "[ warning ] " << msg << std::endl;
            break;
        }
        case LogType::kError:
        {
            std::cerr << "[ error ] " << msg << std::endl;
            break;
        }
    }
}
