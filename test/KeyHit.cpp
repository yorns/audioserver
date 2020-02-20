#include "KeyHit.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "common/logger.h"

using namespace LoggerFramework;

void KeyHit::setTerminal() {
    if (tcgetattr(0, &m_term) < 0)
        perror("tcsetattr()");
    m_term.c_lflag &= ~ICANON;
    m_term.c_lflag &= ~ECHO;
    m_term.c_cc[VMIN] = 1;
    m_term.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &m_term) < 0)
        perror("tcsetattr TCSANOW");
}

void KeyHit::resetTerminal() {
    m_term.c_lflag |= ICANON;
    m_term.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &m_term) < 0)
        perror("tcsetattr TCSADRAIN");
}

int KeyHit::setNonblocking(int fd)
{
    int flags;

    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void KeyHit::readLine()
{
    char buf(0);
    if (read(0, &buf, 1) < 0) {
        if (errno != EAGAIN)
            perror("read()");
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (m_keyFunc) m_keyFunc(buf);
}

void KeyHit::start() {
    m_thread = std::thread([this]() {

        /* set terminal */
        setTerminal();
        setNonblocking(0);

        while (!m_stop)
            readLine();

        /* reset terminal */
        resetTerminal();

        m_stopped = true;
    });
}

KeyHit::KeyHit() {
    memset(&m_term, 0, sizeof(m_term));
}

KeyHit::~KeyHit() {
    while (!m_stopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (m_thread.joinable())
        m_thread.join();
    logger(Level::info) << "keyhit ended\n";
}

void KeyHit::setKeyReceiver(KeyFunc keyFunc) { m_keyFunc = keyFunc; }

void KeyHit::stop() {
    // is called from another context
    m_stop = true;
    logger(Level::info) << "keyhit stop called\n";
}
