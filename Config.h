#pragma once

#include <deque>
#include <map>
#include <optional>

#include <spdlog/common.h>

#include "Signalling/Config.h"
#include "Client/Config.h"

#include "RtStreaming/WebRTCConfig.h"


struct StreamSource
{
    enum class Type {
        Record,
        WebRTSP,
    };

    Type type;

    std::optional<signalling::Config> localServer; // for RECORD
    std::optional<client::Config> client;

    std::string uri;
    std::string token;
};

struct Config
{
    spdlog::level::level_enum logLevel = spdlog::level::info;
    spdlog::level::level_enum lwsLogLevel = spdlog::level::warn;

    std::shared_ptr<WebRTCConfig> webRTCConfig = std::make_shared<WebRTCConfig>();

    std::optional<StreamSource> source;
};
