#include "UrlPlayer.h"

#include <CxxPtr/GstPtr.h>

#include "Log.h"


struct UrlPlayer::Private
{
    Private(UrlPlayer* owner, const UrlPlayer::EosCallback& eosCallback);

    gboolean onBusMessage(GstMessage*);

    UrlPlayer *const owner;
    const UrlPlayer::EosCallback eosCallback;

    std::shared_ptr<spdlog::logger> log;
    GstElementPtr pipelinePtr;
};

UrlPlayer::Private::Private(UrlPlayer* owner, const UrlPlayer::EosCallback& eosCallback):
    owner(owner),
    eosCallback(eosCallback),
    log(MonitorLog())
{
}

gboolean UrlPlayer::Private::onBusMessage(GstMessage* message)
{
    switch(GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            owner->onEos();
            break;
        case GST_MESSAGE_ERROR: {
            gchar* debug = nullptr;
            GError* error = nullptr;
            gst_message_parse_error(message, &error, &debug);

            if(debug) {
                log->error("Got error from GStreamer pipeline:\n{}\n{}", error->message, debug);
            } else {
                log->error("Got error from GStreamer pipeline:\n{}", error->message);
            }

            if(debug) g_free(debug);
            if(error) g_error_free(error);

            owner->onEos();
            break;
        }
        default:
            break;
    }

    return TRUE;
}

UrlPlayer::UrlPlayer(const EosCallback& eosCallback):
    _p(std::make_unique<UrlPlayer::Private>(this, eosCallback))
{
}

UrlPlayer::~UrlPlayer()
{
}


void UrlPlayer::onEos()
{
    if(_p->eosCallback)
        _p->eosCallback(*this);
}

bool UrlPlayer::isPlaying() const
{
    return !!_p->pipelinePtr;
}

bool UrlPlayer::play(const std::string& url)
{
    stop();

    GstElementPtr pipelinePtr(gst_pipeline_new(nullptr));
    GstElement* pipeline = pipelinePtr.get();
    if(!pipeline) {
        _p->log->error("Failed to create pipeline element");
        return false;
    }

    GstElementPtr playbinPtr(gst_element_factory_make("playbin3", nullptr));
    GstElement* playbin = playbinPtr.get();
    if(!playbin) {
        _p->log->error("Failed to create \"playbin3\" element");
        return false;
    }

    gst_bin_add_many(GST_BIN(pipeline), playbinPtr.release(), nullptr);

    auto onBusMessageCallback =
        + [] (GstBus* bus, GstMessage* message, gpointer userData) -> gboolean
    {
        UrlPlayer* self = static_cast<UrlPlayer*>(userData);
        return self->_p->onBusMessage(message);
    };
    GstBusPtr busPtr(gst_pipeline_get_bus(GST_PIPELINE(pipeline)));
    gst_bus_add_watch(busPtr.get(), onBusMessageCallback, this);

    g_object_set(playbin, "uri", url.c_str(), nullptr);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    _p->pipelinePtr.swap(pipelinePtr);

    return true;
}

void UrlPlayer::stop()
{
    if(!_p->pipelinePtr)
        return;

    GstElement* pipeline = _p->pipelinePtr.get();
    gst_element_set_state(pipeline, GST_STATE_NULL);

    _p->pipelinePtr.reset();
}
