#pragma once

#include <string>
#include <memory>
#include <functional>


class UrlPlayer
{
public:
    typedef std::function<void (UrlPlayer&)> EosCallback;

    UrlPlayer(const EosCallback& eosCallback);
    ~UrlPlayer();

    bool isPlaying() const;
    bool play(const std::string& url);
    void stop();

private:
    void onEos();

private:
    struct Private;
    std::unique_ptr<Private> _p;
};
