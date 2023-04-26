#include <deque>

#include <glib.h>

#include <libwebsockets.h>

#include <CxxPtr/CPtr.h>
#include <CxxPtr/GlibPtr.h>
#include "CxxPtr/libconfigDestroy.h"

#include "Helpers/ConfigHelpers.h"
#include "Helpers/LwsLog.h"

#include "RtStreaming/GstRtStreaming/Log.h"
#include "RtStreaming/GstRtStreaming/LibGst.h"

#include "Signalling/Log.h"

#include "Log.h"
#include "Monitor.h"


static const auto Log = MonitorLog;


static bool LoadConfig(Config* config)
{
    const std::deque<std::string> configDirs = ::ConfigDirs();
    if(configDirs.empty())
        return false;

    Config loadedConfig = *config;

    for(const std::string& configDir: configDirs) {
        const std::string configFile = configDir + "/monitor.conf";
        if(!g_file_test(configFile.c_str(), G_FILE_TEST_IS_REGULAR)) {
            Log()->info("Config file \"{}\" not found", configFile);
            continue;
        }

        config_t config;
        config_init(&config);
        ConfigDestroy ConfigDestroy(&config);

        Log()->info("Loading config from file \"{}\"", configFile);
        if(!config_read_file(&config, configFile.c_str())) {
            Log()->error("Failed to load config file. {}. {}:{}",
                config_error_text(&config),
                configFile,
                config_error_line(&config));
            return false;
        }

        config_setting_t* webrtcConfig = config_lookup(&config, "webrtc");
        if(webrtcConfig && config_setting_is_group(webrtcConfig) != CONFIG_FALSE) {
            const char* stunServer = nullptr;
            if(config_setting_lookup_string(webrtcConfig, "stun-server", &stunServer) != CONFIG_FALSE) {
                if(0 == g_ascii_strncasecmp(stunServer, "stun://", 7)) {
                    loadedConfig.webRTCConfig->iceServers.emplace_back(stunServer);
                } else {
                    Log()->error("STUN server URL should start with \"stun://\"");
                }
            }

            const char* turnServer = nullptr;
            if(config_setting_lookup_string(webrtcConfig, "turn-server", &turnServer) != CONFIG_FALSE) {
               if(0 == g_ascii_strncasecmp(turnServer, "turn://", 7)) {
                    loadedConfig.webRTCConfig->iceServers.emplace_back(turnServer);
                } else {
                    Log()->error("TURN server URL should start with \"turn://\"");
               }
            }

            int minRtpPort;
            int rtpPortsCount;
            if(config_setting_lookup_int(webrtcConfig, "min-rtp-port", &minRtpPort) != CONFIG_FALSE) {
                if(
                    minRtpPort < std::numeric_limits<uint16_t>::min() ||
                    minRtpPort > std::numeric_limits<uint16_t>::max())
                {
                    Log()->error(
                        "min-rtp-port should be in [{}, {}]",
                        std::numeric_limits<uint16_t>::min(),
                        std::numeric_limits<uint16_t>::max());
                } else {
                    loadedConfig.webRTCConfig->minRtpPort = minRtpPort;

                    if(config_setting_lookup_int(webrtcConfig, "rtp-ports-count", &rtpPortsCount) != CONFIG_FALSE) {
                        if(minRtpPort < 1 || minRtpPort > std::numeric_limits<uint16_t>::max()) {
                            Log()->error(
                                "rtp-ports-count should be in [{}, {}]",
                                1,
                                std::numeric_limits<uint16_t>::max() - minRtpPort + 1);
                        } else {
                            loadedConfig.webRTCConfig->maxRtpPort = minRtpPort + rtpPortsCount - 1;
                        }
                    }
                }
            }

            int relayTransportOnly = FALSE;
            if(config_setting_lookup_bool(webrtcConfig, "relay-transport-only", &relayTransportOnly) != CONFIG_FALSE) {
                loadedConfig.webRTCConfig->useRelayTransport = relayTransportOnly != FALSE;
            }
        }

        config_setting_t* debugConfig = config_lookup(&config, "debug");
        if(debugConfig && config_setting_is_group(debugConfig) != CONFIG_FALSE) {
            int logLevel = 0;
            if(config_setting_lookup_int(debugConfig, "log-level", &logLevel) != CONFIG_FALSE) {
                if(logLevel > 0) {
                    loadedConfig.logLevel =
                        static_cast<spdlog::level::level_enum>(
                            spdlog::level::critical - std::min<int>(logLevel, spdlog::level::critical));
                }
            }
            int lwsLogLevel = 0;
            if(config_setting_lookup_int(debugConfig, "lws-log-level", &lwsLogLevel) != CONFIG_FALSE) {
                if(lwsLogLevel > 0) {
                    loadedConfig.lwsLogLevel =
                        static_cast<spdlog::level::level_enum>(
                            spdlog::level::critical - std::min<int>(lwsLogLevel, spdlog::level::critical));
                }
            }
        }

        config_setting_t* sourceConfig = config_lookup(&config, "source");
        if(sourceConfig && config_setting_is_group(sourceConfig) != CONFIG_FALSE) {
            const char* token = "";
            config_setting_lookup_string(sourceConfig, "token", &token);

            int wsPort = signalling::DEFAULT_WS_PORT;
            if(config_lookup_int(&config, "port", &wsPort) != CONFIG_FALSE) {
                if(
                    wsPort < std::numeric_limits<uint16_t>::min() ||
                    wsPort > std::numeric_limits<uint16_t>::max()
                ) {
                    Log()->error(
                        "\"port\" value is invalid. In should be in [{}, {}]",
                        std::numeric_limits<uint16_t>::min(),
                        std::numeric_limits<uint16_t>::max());
                    wsPort = 0;
                }
            }

            int loopbackOnly = FALSE;
            config_lookup_bool(&config, "loopback-only", &loopbackOnly);

            std::optional<signalling::Config> serverConfig;
            if(wsPort) {
                serverConfig = signalling::Config {
                    .bindToLoopbackOnly = loopbackOnly != FALSE,
                    .port = static_cast<unsigned short>(wsPort),
                };
            }

            loadedConfig.source =
                StreamSource {
                    StreamSource::Type::Record,
                    serverConfig,
                    token
                };
        }
    }

    bool success = true;

    if(!loadedConfig.source) {
        Log()->error("\"source\" config is missing");
        success = false;
    }

    if(success) {
        *config = loadedConfig;
        Log()->info("Config loaded");
    }

    return success;
}

int main(int argc, char *argv[])
{
    Config config {};
    if(!LoadConfig(&config))
        return -1;

    InitLwsLogger(config.lwsLogLevel);
    InitWsServerLogger(config.logLevel);
    InitServerSessionLogger(config.logLevel);
    InitGstRtStreamingLogger(config.logLevel);
    InitMonitorLogger(config.logLevel);

    LibGst libGst;

    return MonitorMain(config);
}
