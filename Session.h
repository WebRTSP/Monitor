#pragma once

#include "Client/ClientSession.h"

#include "Config.h"


class Session: public ClientSession
{
public:
    Session(
        const Config*,
        const CreatePeer& createPeer,
        const SendRequest& sendRequest,
        const SendResponse& sendResponse) noexcept;

protected:
    FeatureState playSupportState(const std::string& /*uri*/) noexcept override
        { return FeatureState::Enabled; }
    FeatureState subscribeSupportState(const std::string& /*uri*/) noexcept override
        { return FeatureState::Enabled; }

private:
    const Config *const _config;
};
