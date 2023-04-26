#pragma once

#include <map>
#include <unordered_map>

#include "Signalling/ServerSession.h"

#include "Config.h"


class Session : public ServerSession
{
public:
    Session(
        const Config*,
        const CreatePeer& createPeer,
        const rtsp::Session::SendRequest& sendRequest,
        const rtsp::Session::SendResponse& sendResponse) noexcept;
    Session(
        const Config*,
        const CreatePeer& createPeer,
        const CreatePeer& createRecordPeer,
        const rtsp::Session::SendRequest& sendRequest,
        const rtsp::Session::SendResponse& sendResponse) noexcept;
    ~Session();

protected:
    bool recordEnabled(const std::string& uri) noexcept override;
    bool authorizeRecorder(const std::unique_ptr<rtsp::Request>&) noexcept;
    bool authorize(const std::unique_ptr<rtsp::Request>&) noexcept override;

private:
    void startRecord(const std::string& uri) noexcept;

private:
    const Config *const _config;
};
