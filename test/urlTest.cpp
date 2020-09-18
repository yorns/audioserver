#include "common/logger.h"
#include "common/Extractor.h"

int main() {

    {
        logger(LoggerFramework::Level::info) << "Test 1: extract command\n";
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/command?param1=1&param2=whateverYouThink",
                                                               "http://myurl/at/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 2:\n";
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/command?param1=1&param2=whateverYouThink",
                                                               "http://myurl/at/whatever/");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 3:\n";
        auto extracted = utility::Extractor::getUrlInformation("http:///myurl/at/whatever/command?param1=1&param2=whateverYouThink",
                                                               "/myurl/at/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 4:\n";
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/cmd",
                                                               "myurl/at/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "cmd");
        assert ( ! extracted->hasParameter("param1") );
        assert ( ! extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "");
        assert ( extracted->getValueOfParameter("param2") == "");
        assert ( extracted->getValueOfParameter("param3") == "");
    }


    {
        logger(LoggerFramework::Level::info) << "Test 5:\n";
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/cmd",
                                                               "myurl/at/");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "whatever/cmd");
        assert ( ! extracted->hasParameter("param1") );
        assert ( ! extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "");
        assert ( extracted->getValueOfParameter("param2") == "");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 6:\n";
        auto extracted = utility::Extractor::getUrlInformation("whatever/cmd", "whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "cmd");
        assert ( ! extracted->hasParameter("param1") );
        assert ( ! extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "");
        assert ( extracted->getValueOfParameter("param2") == "");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 7:\n";
        auto extracted = utility::Extractor::getUrlInformation("/whatever/command?param1=1&param2=whateverYouThink",
                                                               "/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 8:\n";
        auto extracted = utility::Extractor::getUrlInformation("whatever/command?param1=1&param2=whateverYouThink",
                                                               "http://whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter(std::string_view("param1")) );
        assert ( extracted->hasParameter(std::string_view("param2")) );
        assert ( ! extracted->hasParameter(std::string_view("param3")) );
        assert ( extracted->getValueOfParameter(std::string_view("param1")) == "1");
        assert ( extracted->getValueOfParameter(std::string_view("param2")) == "whateverYouThink");
        assert ( extracted->getValueOfParameter(std::string_view("param3")) == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 9:\n";
        auto extracted = utility::Extractor::getUrlInformation("/whatever/command?param1=1&param2",
                                                               "/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter(std::string_view("param1")) );
        assert ( extracted->hasParameter(std::string_view("param2")) );
        assert ( extracted->getValueOfParameter(std::string_view("param1")) == "1");
        assert ( extracted->getValueOfParameter(std::string_view("param2")) == "");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 10:\n";
        auto extracted = utility::Extractor::getUrlInformation("/whatever/command?param1&param2=whateverYouThink",
                                                               "/whatever");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "command");
        assert ( extracted->hasParameter(std::string_view("param1")) );
        assert ( extracted->hasParameter(std::string_view("param2")) );
        assert ( extracted->getValueOfParameter(std::string_view("param1")) == "");
        assert ( extracted->getValueOfParameter(std::string_view("param2")) == "whateverYouThink");
    }

    {
        logger(LoggerFramework::Level::info) << "Test 11:\n";
        auto extracted = utility::Extractor::getUrlInformation("/img/image.jpg",
                                                               "/img");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "image.jpg" );
        assert ( extracted->getBase() == "/img" );
    }

    {
        logger(LoggerFramework::Level::info) << "Test 12:\n";
        auto extracted = utility::Extractor::getUrlInformation("/playlist?show=",
                                                               "/playlist");

        assert ( extracted.has_value() );
        assert ( extracted->getCommand() == "");
        assert ( extracted->hasParameter(std::string_view("show")) );
        assert ( extracted->getValueOfParameter(std::string_view("show")) == "");
    }


    return 0;
}
