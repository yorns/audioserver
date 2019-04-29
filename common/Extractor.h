#ifndef SERVER_EXTRACTOR_H
#define SERVER_EXTRACTOR_H

#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <boost/optional.hpp>

namespace utility {

    struct Extractor {

        struct UrlInfo {
            std::string command;
            std::string parameter;
            std::string value;
        };

        typedef boost::optional<UrlInfo> UrlInformation;

        static UrlInformation getUrlInformation(const std::string& url) {

            const std::regex pattern{"./([^\\?]*)\\?([^=]*)=([^&]*)$"};

            std::smatch match{};
            if (std::regex_search(url, match, pattern)) {
                UrlInfo info;
                try {
                    info.command = match[1].str();
                    info.parameter = match[2].str();
                    info.value = match[3].str();
                } catch (...) {
                    return boost::none;
                }
                return info;
            }

            return boost::none;
        }
    };

}

#endif //SERVER_EXTRACTOR_H
