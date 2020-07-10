#ifndef SERVER_EXTRACTOR_H
#define SERVER_EXTRACTOR_H

#include <string>
#include <vector>
#include <tuple>
#include <regex>
#include <optional>
#include <algorithm>
#include "urlDecode.h"
#include <boost/tokenizer.hpp>

namespace utility {

struct Parameter {
    std::string name;
    std::string value;
};

    static std::string urlConvert(const std::string& url) {

        if (url.empty())
            return std::string("");

        std::vector<char> tmp1(url.length()+20,0);
        std::vector<char> tmp2(url.length()+20,0);
        std::memcpy(tmp1.data(), url.data(), url.length());

        auto length = static_cast<std::size_t>(decodeURIComponent(tmp1.data(), tmp2.data()));

        return std::string(tmp2.data(), length);
    }


    static std::string findCommand(const std::string& name, const std::string& startValue = "/", const std::string& endValue = "?") {
        auto lastPos = name.find_first_of(endValue);
        if (lastPos != std::string::npos)
            lastPos -=1;
        auto firstPos = name.find_last_of(startValue,lastPos);
        if (lastPos > firstPos) {
            return name.substr(firstPos+1, lastPos-firstPos);
        }
        return "";
    }

    static std::vector<Parameter> findParameters(const std::string& _name, const std::string& startValue = "?", const std::string& separator = "&") {

        auto startPos = _name.find_first_of(startValue);

        if (startPos == std::string::npos)
            return  {};

        auto name = _name.substr(startPos+1);

        // try to use tockenizer again. This is no real solution (no .size() .at(), constructor does not work with arbitrary input (e.g. _name.substr(startPos+1) gives nonsens))
        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
        boost::char_separator<char> sep{separator.c_str()};

        std::vector<Parameter> parameterList;

        tokenizer tok{name, sep};
        for (const auto &t : tok) {
            std::string tmp = t;
            auto pos = tmp.find_first_of("=");
            if (pos != std::string::npos) {
                Parameter parameter;
                parameter.name = tmp.substr(0,pos);
                parameter.value = urlConvert(tmp.substr(pos+1));
                parameterList.emplace_back(std::move(parameter));
            }
        }

        return parameterList;
    }

    struct Extractor {

        struct UrlInfo {
            std::string command;
            std::vector<Parameter> parameterList;

            bool parsed {false};

            bool parse(const std::string& url) {
                command = findCommand(url);
                parameterList = findParameters(url);
                parsed = true;
                return !command.empty();
            }

            bool hasParameter(const std::string& parameterName) const {
                return std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; })
                        != std::end(parameterList);
            }

            bool hasParameter(std::string_view parameterName) const {
                return std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; })
                        != std::end(parameterList);
            }

            bool hasParameter(const char* parameterName) const {
                return std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; })
                        != std::end(parameterList);
            }

            std::string getValueOfParameter(const std::string& parameterName) const {
                auto it = std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(parameterList))
                    return it->value;

                return "";

            }

            std::string getValueOfParameter(std::string_view parameterName) const {
                auto it = std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(parameterList))
                    return it->value;

                return "";

            }

            std::string getValueOfParameter(const char* parameterName) const {
                auto it = std::find_if(std::begin(parameterList), std::end(parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(parameterList))
                    return it->value;

                return "";

            }

        };

        using UrlInformation = std::optional<UrlInfo>;

        static UrlInformation getUrlInformation(const std::string& url) {

            UrlInfo info;
            if (info.parse(url))
                return info;

            return std::nullopt;
        }
    };

}

#endif //SERVER_EXTRACTOR_H
