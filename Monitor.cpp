#include "Monitor.h"

#include <CxxPtr/GlibPtr.h>
#include <CxxPtr/libwebsocketsPtr.h>

#include "Signalling/WsServer.h"

#include "WebRTSP/Client/WsClient.h"
#include "WebRTSP/Client/ClientSession.h"

#include "RtStreaming/GstRtStreaming/GstTestStreamer.h"
#include "RtStreaming/GstRtStreaming/GstReStreamer.h"

#include "RtStreaming/GstRtStreaming/GstTestStreamer2.h"
#include "RtStreaming/GstRtStreaming/GstReStreamer2.h"
#include "RtStreaming/GstRtStreaming/GstRecordStreamer.h"
#include "RtStreaming/GstRtStreaming/GstClient.h"

#include "Log.h"
#include "RecordSession.h"
#include "Session.h"


static const auto Log = MonitorLog;
typedef std::map<std::string, std::unique_ptr<GstStreamingSource>> MountPoints;

enum {
    MIN_RECONNECT_TIMEOUT = 3, // seconds
    MAX_RECONNECT_TIMEOUT = 10, // seconds
};

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

static std::unique_ptr<ServerSession> CreateServerSession(
    const Config* config,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse)
{
    std::unique_ptr<RecordSession> session =
        std::make_unique<RecordSession>(
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


static std::unique_ptr<WebRTCPeer>
CreateClientPeer(const Config* config)
{
    return std::make_unique<GstClient>();
}

static std::unique_ptr<rtsp::Session> CreateClientSession (
    const Config* config,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse) noexcept
{
    return std::make_unique<Session>(
        config,
        std::bind(CreateClientPeer, config),
        sendRequest, sendResponse);
}

static void ClientDisconnected(client::WsClient& client)
{
    const unsigned reconnectTimeout =
        g_random_int_range(MIN_RECONNECT_TIMEOUT, MAX_RECONNECT_TIMEOUT + 1);
    Log()->info("Scheduling reconnect within {} seconds...", reconnectTimeout);
    GSourcePtr timeoutSourcePtr(g_timeout_source_new_seconds(reconnectTimeout));
    GSource* timeoutSource = timeoutSourcePtr.get();
    g_source_set_callback(timeoutSource,
        [] (gpointer userData) -> gboolean {
            static_cast<client::WsClient*>(userData)->connect();
            return false;
        }, &client, nullptr);
    g_source_attach(timeoutSource, g_main_context_get_thread_default());
}

int MonitorMain(const Config& config)
{
    if(!config.source)
        return -1;

    GMainContextPtr contextPtr(g_main_context_new());
    GMainContext* context = contextPtr.get();
    g_main_context_push_thread_default(context);

    GMainLoopPtr loopPtr(g_main_loop_new(context, FALSE));
    GMainLoop* loop = loopPtr.get();

    if(config.source->localServer) {
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
            *config.source->localServer,
            loop,
            std::bind(
                CreateServerSession,
                &config,
                std::placeholders::_1,
                std::placeholders::_2));

        if(server.init(lwsContext)) {
            g_main_loop_run(loop);
            return 0;
        }
    } else if(config.source->client) {
        client::WsClient client(
            config.source->client.value(),
            loop,
            std::bind(
                CreateClientSession,
                &config,
                std::placeholders::_1,
                std::placeholders::_2),
            ClientDisconnected);

        if(client.init()) {
            client.connect();
            g_main_loop_run(loop);
            return 0;
        }
    }

    return -1;
}
