#ifndef SERVER_EXTRACTOR_H
#define SERVER_EXTRACTOR_H

#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <optional>
#include "urlDecode.h"

namespace utility {


    static std::string urlConvert(const std::string& url) {

        if (url.empty())
            return std::string("");

        std::vector<char> tmp1(url.length()+20,0);
        std::vector<char> tmp2(url.length()+20,0);
        std::memcpy(tmp1.data(), url.data(), url.length());

        auto length = static_cast<std::size_t>(decodeURIComponent(tmp1.data(), tmp2.data()));

        return std::string(tmp2.data(), length);
    }


    struct Extractor {

        struct UrlInfo {
            std::string command;
            std::string parameter;
            std::string value;
        };

        using UrlInformation = std::optional<UrlInfo>;

        static UrlInformation getUrlInformation(const std::string& url) {

            const std::regex pattern{".*/([^\\?]*)\\?([^=]*)=([^&]*)$"};

            std::smatch match{};
            if (std::regex_search(url, match, pattern)) { /* no regex string_view with c++17 */
                UrlInfo info;
                try {
                    info.command = match[1].str();
                    info.parameter = match[2].str();
                    std::string m = match[3].str();
                    info.value = urlConvert(m);
                } catch (...) {
                    return std::nullopt;
                }
                return UrlInformation(info);
            }

            return std::nullopt;
        }
    };

}

#endif //SERVER_EXTRACTOR_H
