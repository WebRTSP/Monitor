#pragma once


#include <cstdint>
#include <optional>
#include <string>
#include <memory>
#include <chrono>

#include "UrlPlayer.h"


class OnvifPlayer: private UrlPlayer
{
public:
    enum Error: int32_t {
        DEVICE_MEDIA_HAS_NO_PROFILES = 1,
        DEVICE_MEDIA_PROFILE_HAS_NO_STREAM_URI = 2,
        NOTIFICATION_MESSAGE_HAS_NO_DATA_ELEMENT = 3,
        NOTIFICATION_MESSAGE_DOES_NOT_CONTAIN_MOTION_EVENT = 4,
    };

    typedef std::function<void (OnvifPlayer&)> EosCallback;

    OnvifPlayer(
        const std::string& url,
        const std::optional<std::string>& username,
        const std::optional<std::string>& password,
        bool trackMotion,
        std::chrono::seconds motionPreviewDuration,
        bool showVideoStats,
        bool sync,
        const EosCallback&) noexcept;
    ~OnvifPlayer();

    void play() noexcept;

private:
    struct Private;
    std::unique_ptr<Private> _p;
};
