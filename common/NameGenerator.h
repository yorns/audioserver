#ifndef SERVER_NAMEGENERATOR_H
#define SERVER_NAMEGENERATOR_H

#include <string>

namespace NameGenerator {

struct GenerationName {
    std::string filename;
    std::string unique_id;
};

extern GenerationName create(const std::string& prefix, const std::string& suffix);

}

#endif //SERVER_NAMEGENERATOR_H
