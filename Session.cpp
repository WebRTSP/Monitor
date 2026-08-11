#include "Session.h"


Session::Session(
    const Config* config,
    const CreatePeer& createPeer,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse) noexcept :
    ClientSession(config->webRTCConfig, createPeer, sendRequest, sendResponse),
    _config(config)
{
    setUri(config->source->uri);
}

void Session::sendRequest(rtsp::Request& request) noexcept
{
    if(_config->source &&
        request.uri == _config->source->uri &&
        !_config->source->accessToken.empty())
    {
        SetBearerAuthorization(&request, _config->source->accessToken);
    }

    ClientSession::sendRequest(request);
}
