#include "RecordSession.h"

#include <glib.h>

#include "RtspParser/RtspParser.h"

#include "Log.h"


static const auto Log = MonitorLog;

RecordSession::RecordSession(
    const Config* config,
    const CreatePeer& createPeer,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse) noexcept :
    ServerSession(config->webRTCConfig, createPeer, sendRequest, sendResponse),
    _config(config)
{
}

RecordSession::RecordSession(
    const Config* config,
    const CreatePeer& createPeer,
    const CreatePeer& createRecordPeer,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse) noexcept :
    ServerSession(config->webRTCConfig, createPeer, createRecordPeer, sendRequest, sendResponse),
    _config(config)
{
}

RecordSession::~RecordSession() {
}

bool RecordSession::recordEnabled(const std::string& uri) noexcept
{
    return _config->source && _config->source->type == StreamSource::Type::Record;
}

bool RecordSession::authorizeRecorder(const std::unique_ptr<rtsp::Request>& requestPtr) noexcept
{
    if(!_config->source)
        return false;

    const StreamSource& source = _config->source.value();

    if(source.type != StreamSource::Type::Record)
        return false;

    if(source.token.empty())
        return true;

    const std::pair<rtsp::Authentication, std::string> authPair =
        rtsp::ParseAuthentication(*requestPtr);

    if(authPair.first != rtsp::Authentication::Bearer) // FIXME? only Bearer supported atm
        return false;

    return authPair.second == source.token;
}

bool RecordSession::authorize(const std::unique_ptr<rtsp::Request>& requestPtr) noexcept
{
    switch(requestPtr->method) {
    case rtsp::Method::OPTIONS:
        break;
    case rtsp::Method::LIST:
    case rtsp::Method::DESCRIBE:
        return false;
    case rtsp::Method::SETUP:
        break;
    case rtsp::Method::PLAY:
    case rtsp::Method::SUBSCRIBE:
        return false;
    case rtsp::Method::RECORD:
        return authorizeRecorder(requestPtr);
    case rtsp::Method::TEARDOWN:
        break;
    case rtsp::Method::GET_PARAMETER:
    case rtsp::Method::SET_PARAMETER:
        return false;
    }

    return ServerSession::authorize(requestPtr);
}
