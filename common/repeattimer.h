#ifndef REPEATTIMER_H
#define REPEATTIMER_H

#include <functional>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include <common/logger.h>

class RepeatTimer {

    boost::asio::steady_timer m_timer;
    std::chrono::milliseconds m_duration;
    std::function<void()> m_handler;

public:
    RepeatTimer(boost::asio::io_context& context,
                const std::chrono::milliseconds& duration)
        : m_timer(context), m_duration(duration) {
        logger(LoggerFramework::Level::debug)
                << "set Repeat Timer timeout to " << m_duration.count() << "\n";
    }

    void start() {
        m_timer.expires_after(m_duration);
        m_timer.async_wait([this](const boost::system::error_code& error){
            if(!error && m_handler) { m_handler(); start(); } });
    }

    void stop() { m_timer.cancel(); }

    void setHandler(std::function<void()>&& handler)
    { m_handler = std::move(handler); }

};

#endif // REPEATTIMER_H
