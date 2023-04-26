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
            Log()->info("Config \"{}\" not found", configFile);
            continue;
        }

        config_t config;
        config_init(&config);
        ConfigDestroy ConfigDestroy(&config);

        Log()->info("Loading config \"{}\"", configFile);
        if(!config_read_file(&config, configFile.c_str())) {
            Log()->error("Fail load config. {}. {}:{}",
                config_error_text(&config),
                configFile,
                config_error_line(&config));
            return false;
        }

        int wsPort= 0;
        if(CONFIG_TRUE == config_lookup_int(&config, "ws-port", &wsPort)) {
            loadedConfig.port = static_cast<unsigned short>(wsPort);
        } else {
            loadedConfig.port = 0;
        }

        int loopbackOnly = false;
        if(CONFIG_TRUE == config_lookup_bool(&config, "loopback-only", &loopbackOnly)) {
            loadedConfig.bindToLoopbackOnly = loopbackOnly != false;
        }

        const char* stunServer = nullptr;
        if(CONFIG_TRUE == config_lookup_string(&config, "stun-server", &stunServer)) {
            if(0 == g_ascii_strncasecmp(stunServer, "stun://", 7)) {
                loadedConfig.webRTCConfig->iceServers.emplace_back(stunServer);
            } else {
                Log()->error("STUN server URL should start with \"stun://\"");
            }
        }

        const char* turnServer = nullptr;
        if(CONFIG_TRUE == config_lookup_string(&config, "turn-server", &turnServer)) {
           if(0 == g_ascii_strncasecmp(turnServer, "turn://", 7)) {
                loadedConfig.webRTCConfig->iceServers.emplace_back(turnServer);
            } else {
                Log()->error("TURN server URL should start with \"turn://\"");
           }
        }

        config_setting_t* debugConfig = config_lookup(&config, "debug");
        if(debugConfig && CONFIG_TRUE == config_setting_is_group(debugConfig)) {
            int logLevel = 0;
            if(CONFIG_TRUE == config_setting_lookup_int(debugConfig, "log-level", &logLevel)) {
                if(logLevel > 0) {
                    loadedConfig.logLevel =
                        static_cast<spdlog::level::level_enum>(
                            spdlog::level::critical - std::min<int>(logLevel, spdlog::level::critical));
                }
            }
            int lwsLogLevel = 0;
            if(CONFIG_TRUE == config_setting_lookup_int(debugConfig, "lws-log-level", &lwsLogLevel)) {
                if(lwsLogLevel > 0) {
                    loadedConfig.lwsLogLevel =
                        static_cast<spdlog::level::level_enum>(
                            spdlog::level::critical - std::min<int>(lwsLogLevel, spdlog::level::critical));
                }
            }
        }

        config_setting_t* sourceConfig = config_lookup(&config, "source");
        if(sourceConfig && CONFIG_TRUE == config_setting_is_group(sourceConfig)) {
            std::optional<StreamSource::Type> sourceType;
            const char* type = nullptr;
            config_setting_lookup_string(sourceConfig, "type", &type);
            if(nullptr == type || 0 == strcmp(type, "regular"))
                sourceType = StreamSource::Type::Regular;
            else if(0 == strcmp(type, "test"))
                sourceType = StreamSource::Type::Test;
            else if(0 == strcmp(type, "onvif"))
                sourceType = StreamSource::Type::ONVIF;
            else if(0 == strcmp(type, "record"))
                sourceType = StreamSource::Type::Record;

            const char* recordToken = "";
            config_setting_lookup_string(sourceConfig, "record-token", &recordToken);

            const char* description = nullptr;
            config_setting_lookup_string(sourceConfig, "description", &description);

            const char* forceH264ProfileLevelId = nullptr;
            config_setting_lookup_string(sourceConfig, "force-h264-profile-level-id", &forceH264ProfileLevelId);

            const char* uri = "";
            config_setting_lookup_string(sourceConfig, "url", &uri);
            config_setting_lookup_string(sourceConfig, "uri", &uri);

            int trackMotionEvent = false;
            config_setting_lookup_bool(sourceConfig, "track-motion-event", &trackMotionEvent);
            int motionPreviewTime = 0;
            config_setting_lookup_int(sourceConfig, "motion-preview-time", &motionPreviewTime);

            loadedConfig.source =
                StreamSource {
                    *sourceType,
                    uri,
                    recordToken,
                    forceH264ProfileLevelId ?
                        std::string(forceH264ProfileLevelId) :
                        std::string() };
        }
    }

    bool success = true;

    if(!loadedConfig.source) {
        Log()->error("\"source\" config is missing");
    }

    switch(loadedConfig.source->type) {
    case StreamSource::Type::Test:
        if(loadedConfig.source->uri.empty()) {
            Log()->error("\"uri\" config value is mandatory for \"test\" source");
            success = false;
        }
        break;
    case StreamSource::Type::Regular:
        if(!loadedConfig.source->uri.empty()) {
            Log()->error("\"uri\" config value is mandatory for \"regular\" source");
            success = false;
        }
        break;
    case StreamSource::Type::Record:
        break;
    case StreamSource::Type::ONVIF:
        if(!loadedConfig.source->uri.empty()) {
            Log()->error("\"uri\" config value is mandatory for \"onvif\" source");
            success = false;
        }
    }

    if(success) {
        *config = loadedConfig;
    }

    return success;
}

int main(int argc, char *argv[])
{
    Config config {};
    config.bindToLoopbackOnly = false;
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
