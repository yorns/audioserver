#ifndef SERVER_EXTRACTOR_H
#define SERVER_EXTRACTOR_H

#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <iostream>
#include <boost/optional.hpp>
#include "urlDecode.h"

namespace utility {


    static std::string urlConvert(const std::string& url) {

        if (url.empty())
            return std::string("");

        std::vector<char> tmp1(url.length()+2,0);
        std::vector<char> tmp2(url.length()+2,0);
        std::memcpy(tmp1.data(), url.data(), url.length());

        //std::cout << "url("<<url.length()<<"): "<< url << "\n";

        int length = decodeURIComponent(tmp1.data(), tmp2.data());

        return std::string(tmp2.data(), length);
    }


    struct Extractor {

        struct UrlInfo {
            std::string command;
            std::string parameter;
            std::string value;
        };

        typedef boost::optional<UrlInfo> UrlInformation;

        static UrlInformation getUrlInformation(const std::string& url) {

            const std::regex pattern{".*/([^\\?]*)\\?([^=]*)=([^&]*)$"};

            std::smatch match{};
            if (std::regex_search(url, match, pattern)) {
                UrlInfo info;
                try {
                    //std::cout << "do extract <"<<url<<">\n";
                    info.command = match[1].str();
                    info.parameter = match[2].str();
                    std::string m = match[3].str();
                    info.value = urlConvert(m);
                    //std::cout << "convert <"<<m<<"> to <"<<info.value <<">\n";

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
