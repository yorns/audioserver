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

    static std::string_view toJson(std::string_view url) {

        if (url.empty())
            return std::string_view("");

        std::vector<char> tmp1(url.length()+20,0);
        auto tmp2 = new std::vector<char>(url.length()+20,0); // ugly, memory leak
        std::memcpy(tmp1.data(), url.data(), url.length());

        auto length = static_cast<std::size_t>(decodeURIComponent(tmp1.data(), tmp2->data()));

        auto ret = std::string_view(tmp2->data(), length);
        return ret;
    }

    static std::string_view findAccessPrefix(std::string_view name) {
      static const std::string_view identifier { "://" };
      auto posAccessPrefix = name.find(identifier);

        if ( posAccessPrefix != std::string::npos ) {
          return name.substr(0, posAccessPrefix + identifier.length());
      }
        return "";
    }

    static std::string_view findCommand(std::string_view name, std::string_view base,
                                   std::string_view accessPrefix, std::string_view endValue = "?") {
        
        auto prefixLenght = accessPrefix.length();
        auto baseLength = base.length();
        auto basePrefix = findAccessPrefix(base);
        auto basePrefixLength = basePrefix.length();

        if (prefixLenght && basePrefixLength && basePrefix != accessPrefix) {
            return "";
        }

        // "/my/thing/command/area?parameter=value" with base = "/my/thing"
        // is 
        // "/arg?parameter=value"
        // or
        // "/my/otherThing/command/area?parameter=value"
        if (name.length() > (prefixLenght + baseLength - basePrefixLength) &&
                name.substr(prefixLenght, baseLength - basePrefixLength) == base.substr(basePrefixLength)) {
            
            // "/my/thing/command/area?parameter=value" with base = "/my/thing"
            //                        ^
            auto endPos = name.find_last_of(endValue);
            // npos = 23
            // with "/my/thing/command/area" with base = "/my/thing"
            // npos == std::string::npos

            auto firstPos = prefixLenght + baseLength - basePrefixLength;
            std::size_t length (std::string::npos);
            
            std::size_t posAdd{0}; // add one, as first new one sign is one after found
            if (name.at(firstPos) == '/') // maybe base is without "/" then add another one
                ++posAdd;

            if (endPos != std::string::npos && (endPos+1) >= (firstPos + posAdd)) {
                length = endPos - firstPos - posAdd;
            }
            auto command = name.substr(firstPos + posAdd, length);

            return command;
        }
        return "";
    }

    static std::vector<Parameter> findParameters(std::string_view _name, const char startValue = '?',
                                                 const char* separatorList = "&") {

        auto startPos = _name.find_first_of(startValue);

        if (startPos == std::string::npos)
            return  {};

        auto name = _name.substr(startPos+1);

        // try to use tockenizer again. This is no real solution (no .size() .at(), constructor does not work with arbitrary input (e.g. _name.substr(startPos+1) gives nonsens))
        typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
        boost::char_separator<char> sep{separatorList};

        std::vector<Parameter> parameterList;

        tokenizer tok{name, sep};

        for (const auto &t : tok) {
            std::string tmp = t;
            auto pos = tmp.find_first_of("=");
            if (pos != std::string::npos) {
                Parameter parameter;
                parameter.name = tmp.substr(0,pos);
                auto value = tmp.substr(pos+1);
                parameter.value = toJson(value);
                parameterList.emplace_back(std::move(parameter));
            } else if (tmp.size() > 0) {
                Parameter parameter;
                parameter.name = tmp;
                parameter.value = "";
                parameterList.emplace_back(std::move(parameter));
            }
        }

        return parameterList;
    }

    struct Extractor {

        struct UrlInfo {
            std::string_view m_accessPrefix;
            std::string_view m_base;
            std::string_view m_command;
            std::vector<Parameter> m_parameterList;

            bool m_parsed {false};

            bool parse(std::string_view url, std::string_view base) {
                m_base = base;
                m_accessPrefix = findAccessPrefix(url);
                m_command = findCommand(url, m_base, m_accessPrefix);
                m_parameterList = findParameters(url);
                m_parsed = true;
                return !(m_command.empty() && m_parameterList.empty() && m_base.empty())  ;
            }

            std::string_view getCommand() const  { return m_command; }
            std::string_view getAccessPrefix() const { return m_accessPrefix; }
            std::string_view getBase() const { return m_base; }

            bool hasParameter(std::string_view parameterName) const {
                return std::find_if(std::begin(m_parameterList), std::end(m_parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; })
                        != std::end(m_parameterList);
            }

            std::string_view getValueOfParameter(const std::string& parameterName) const {
                auto it = std::find_if(std::begin(m_parameterList), std::end(m_parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(m_parameterList))
                    return it->value;

                return "";

            }

            std::string_view getValueOfParameter(std::string_view parameterName) const {
                auto it = std::find_if(std::begin(m_parameterList), std::end(m_parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(m_parameterList))
                    return it->value;

                return "";

            }

            std::string_view getValueOfParameter(const char* parameterName) const {
                auto it = std::find_if(std::begin(m_parameterList), std::end(m_parameterList),
                                    [&parameterName](const Parameter& param){ return param.name == parameterName; });
                if (it != std::end(m_parameterList))
                    return it->value;

                return "";

            }

        };

        using UrlInformation = std::optional<UrlInfo>;

        static UrlInformation getUrlInformation(std::string_view url, std::string_view base) {

            UrlInfo info;
            if (info.parse(url, base))
                return info;

            return std::nullopt;
        }
    };

}

#endif //SERVER_EXTRACTOR_H
