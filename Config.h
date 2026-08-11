#pragma once

#include <optional>

#include <spdlog/common.h>

#include "Signalling/Config.h"

#include "RtStreaming/WebRTCConfig.h"


struct StreamSource
{
    enum class Type {
        WebRTSP,
        Onvif,
        Url,
    };

    Type type;

    std::optional<WsServerConfig> localServer; // for RECORD
    std::string recordToken;

    std::optional<WsClientConfig> client;
    std::string uri;

    bool trackMotion; // for ONVIF sources
    std::chrono::seconds motionPreviewDuration = std::chrono::seconds(15);
};

struct VideoOutput
{
    bool showStats = false;
    bool sync = true;
};

struct Config
{
    spdlog::level::level_enum logLevel = spdlog::level::info;
    spdlog::level::level_enum lwsLogLevel = spdlog::level::warn;

    std::shared_ptr<WebRTCConfig> webRTCConfig = std::make_shared<WebRTCConfig>();

    std::optional<StreamSource> source;

    VideoOutput videoOutput;
};
