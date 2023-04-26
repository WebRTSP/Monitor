#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>


static std::shared_ptr<spdlog::logger> Logger;

void InitMonitorLogger(spdlog::level::level_enum level)
{
    spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::stdout_sink_st>();

    Logger = std::make_shared<spdlog::logger>("Monitor", sink);

    Logger->set_level(level);
}

const std::shared_ptr<spdlog::logger>& MonitorLog()
{
    if(!Logger)
#ifndef NDEBUG
        InitMonitorLogger(spdlog::level::debug);
#else
        InitMonitorLogger(spdlog::level::info);
#endif

    return Logger;
}
