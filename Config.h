#pragma once

#include <deque>
#include <map>
#include <optional>

#include <spdlog/common.h>

#include "Signalling/Config.h"

#include "RtStreaming/WebRTCConfig.h"


struct StreamSource
{
    enum class Type {
        Test,
        Regular,
        ONVIF,
        Record,
    };

    Type type;
    std::string uri;
    std::string recordToken;
    std::string forceH264ProfileLevelId;
    bool trackMotionEvent;
    unsigned motionPreviewTime;
};

struct Config : public signalling::Config
{
    spdlog::level::level_enum logLevel = spdlog::level::info;
    spdlog::level::level_enum lwsLogLevel = spdlog::level::warn;

    std::shared_ptr<WebRTCConfig> webRTCConfig = std::make_shared<WebRTCConfig>();

    std::optional<StreamSource> source;
};
