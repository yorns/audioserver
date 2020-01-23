#ifndef SERVER_NAMEGENERATOR_H
#define SERVER_NAMEGENERATOR_H

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace NameGenerator {

struct GenerationName {
    std::string filename;
    std::string unique_id;
};

extern GenerationName create(const std::string& prefix, const std::string& suffix);

}

#endif //SERVER_NAMEGENERATOR_H
