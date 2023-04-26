#include "Monitor.h"

#include <CxxPtr/GlibPtr.h>
#include <CxxPtr/libwebsocketsPtr.h>

#include "Signalling/WsServer.h"

#include "RtStreaming/GstRtStreaming/GstTestStreamer.h"
#include "RtStreaming/GstRtStreaming/GstReStreamer.h"

#include "RtStreaming/GstRtStreaming/GstTestStreamer2.h"
#include "RtStreaming/GstRtStreaming/GstReStreamer2.h"
#include "RtStreaming/GstRtStreaming/GstRecordStreamer.h"
#include "RtStreaming/GstRtStreaming/GstClient.h"

#include "Log.h"
#include "Session.h"


static const auto Log = MonitorLog;

typedef std::map<std::string, std::unique_ptr<GstStreamingSource>> MountPoints;

static std::unique_ptr<WebRTCPeer>
CreatePeer(
    const Config* config,
    const std::string& uri)
{
    return nullptr;
}

static std::unique_ptr<WebRTCPeer>
CreateRecordPeer(
    const Config* config,
    const std::string& uri)
{
    return std::make_unique<GstClient>();
}

static std::unique_ptr<ServerSession> CreateSession(
    const Config* config,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse)
{
    std::unique_ptr<Session> session =
        std::make_unique<Session>(
            config,
            std::bind(CreatePeer, config, std::placeholders::_1),
            std::bind(CreateRecordPeer, config, std::placeholders::_1),
            sendRequest, sendResponse);

    return session;
}

static void OnRecorderConnected(const std::string& uri)
{
    Log()->info("Recorder connected to \"{}\" streamer", uri);
}

static void OnRecorderDisconnected(const std::string& uri)
{
    Log()->info("Recorder disconnected from \"{}\" streamer", uri);
}

int MonitorMain(const Config& config)
{
    GMainContextPtr contextPtr(g_main_context_new());
    GMainContext* context = contextPtr.get();
    g_main_context_push_thread_default(context);

    GMainLoopPtr loopPtr(g_main_loop_new(context, FALSE));
    GMainLoop* loop = loopPtr.get();

    lws_context_creation_info lwsInfo {};
    lwsInfo.gid = -1;
    lwsInfo.uid = -1;
    lwsInfo.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
#if defined(LWS_WITH_GLIB)
    lwsInfo.options |= LWS_SERVER_OPTION_GLIB;
    lwsInfo.foreign_loops = reinterpret_cast<void**>(&loop);
#endif

    LwsContextPtr lwsContextPtr(lws_create_context(&lwsInfo));
    lws_context* lwsContext = lwsContextPtr.get();

    signalling::WsServer server(
        config,
        loop,
        std::bind(
            CreateSession,
            &config,
            std::placeholders::_1,
            std::placeholders::_2));

    if(server.init(lwsContext))
        g_main_loop_run(loop);
    else
        return -1;

    return 0;
}
