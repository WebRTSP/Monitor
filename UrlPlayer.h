#pragma once

#include <string>
#include <memory>
#include <functional>


class UrlPlayer
{
public:
    typedef std::function<void (UrlPlayer&)> EosCallback;

    UrlPlayer(const EosCallback& eosCallback) noexcept;
    ~UrlPlayer();

    bool isPlaying() const noexcept;
    bool play(const std::string& url) noexcept;
    void stop() noexcept;

private:
    void onEos() noexcept;

private:
    struct Private;
    std::unique_ptr<Private> _p;
};
