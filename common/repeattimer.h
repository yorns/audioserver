#ifndef REPEATTIMER_H
#define REPEATTIMER_H

#include <functional>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include <common/logger.h>

class RepeatTimer {

    boost::asio::io_context& m_context;
    boost::asio::steady_timer m_timer;
    std::function<void()> simpleFunc;
    std::chrono::milliseconds m_duration;

public:
    RepeatTimer(boost::asio::io_context& context, const std::chrono::milliseconds& duration)
        : m_context(context), m_timer(m_context), m_duration(duration) {
        logger(Level::info) << "set timeout to " << m_duration.count() << "\n";
    }

    void start() {
        m_timer.expires_from_now(m_duration);
        m_timer.async_wait([this](const boost::system::error_code& error){
            if(!error && simpleFunc) {simpleFunc(); start(); } });
    }

    void stop() {
        m_timer.cancel();
    }

    void setHandler(std::function<void()>&& handler) { simpleFunc = std::move(handler); }

};

#endif // REPEATTIMER_H
