#include "common/Extractor.h"

int main() {

    {
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/command?param1=1&param2=whateverYouThink");

        assert ( extracted.has_value() );
        assert ( extracted->command == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        auto extracted = utility::Extractor::getUrlInformation("http://myurl/at/whatever/cmd");

        assert ( extracted.has_value() );
        assert ( extracted->command == "cmd");
        assert ( ! extracted->hasParameter("param1") );
        assert ( ! extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "");
        assert ( extracted->getValueOfParameter("param2") == "");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        auto extracted = utility::Extractor::getUrlInformation("whatever/cmd");

        assert ( extracted.has_value() );
        assert ( extracted->command == "cmd");
        assert ( ! extracted->hasParameter("param1") );
        assert ( ! extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "");
        assert ( extracted->getValueOfParameter("param2") == "");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        auto extracted = utility::Extractor::getUrlInformation("/whatever/command?param1=1&param2=whateverYouThink");

        assert ( extracted.has_value() );
        assert ( extracted->command == "command");
        assert ( extracted->hasParameter("param1") );
        assert ( extracted->hasParameter("param2") );
        assert ( ! extracted->hasParameter("param3") );
        assert ( extracted->getValueOfParameter("param1") == "1");
        assert ( extracted->getValueOfParameter("param2") == "whateverYouThink");
        assert ( extracted->getValueOfParameter("param3") == "");
    }

    {
        auto extracted = utility::Extractor::getUrlInformation("/whatever/command?param1=1&param2=whateverYouThink");

        assert ( extracted.has_value() );
        assert ( extracted->command == "command");
        assert ( extracted->hasParameter(std::string_view("param1")) );
        assert ( extracted->hasParameter(std::string_view("param2")) );
        assert ( ! extracted->hasParameter(std::string_view("param3")) );
        assert ( extracted->getValueOfParameter(std::string_view("param1")) == "1");
        assert ( extracted->getValueOfParameter(std::string_view("param2")) == "whateverYouThink");
        assert ( extracted->getValueOfParameter(std::string_view("param3")) == "");
    }


    return 0;
}
