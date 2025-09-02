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
