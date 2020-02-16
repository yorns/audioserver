#include <string>
#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <nlohmann/json.hpp>

#include "common/logger.h"

using namespace LoggerFramework;

int main()
{
    globalLevel = Level::debug;

    boost::asio::io_context m_context;

    boost::asio::local::stream_protocol::socket m_socket(m_context);
    std::string m_stringBuffer;
    boost::asio::dynamic_string_buffer m_readBuffer(m_stringBuffer);

    std::string m_accessPoint {"/tmp/mpvsocket"};

    logger(Level::info) << "connect to access <"<<m_accessPoint<<">\n";
    m_socket.connect(m_accessPoint);

    {
        // write command
        std::string command = R"({ "command": ["loadlist", "/home/yorn/tmp/playlist/f7f10f6b-6527-4b6a-91b3-c2da1db14ebb.m3u"] })";
        boost::asio::write(m_socket, boost::asio::buffer(command + '\n'));

        // read answer
        std::size_t size = boost::asio::read_until(m_socket, m_readBuffer, '\n');
        std::string line = m_stringBuffer.substr(0,size);
        m_readBuffer.consume(size);

        // do json analyis of answer
        try {
            nlohmann::json json = nlohmann::json::parse(line);
            if (json.find("error") != json.end()) {
                if ( json["error"] == "success" ) {
                    logger(Level::info) << "playlist was loaded correct\n";
                }
                else {
                    logger(Level::error) << "playlist loaded incorrectly: " << json["error"] << "\n";
                }
            }
        } catch (nlohmann::json::exception& ex) {
            logger(Level::warning) << "reading json failed: " << ex.what() << "\n";
            logger(Level::debug) << "<" << line << ">\n";
        }

    }

    while (1) {
        std::size_t size = boost::asio::read_until(m_socket, m_readBuffer, '\n');
        std::string line = m_stringBuffer.substr(0,size);
        m_readBuffer.consume(size);

        logger(Level::info) << line;
    }

    return EXIT_SUCCESS;
}
