#ifndef SERVER_NAMEGENERATOR_H
#define SERVER_NAMEGENERATOR_H

#include <string>
#include <boost/uuid/uuid.hpp>

namespace Common {

namespace NameGenerator {

struct GenerationName {
    std::string filename;
    std::string unique_id;

};

extern GenerationName create(const std::string& prefix, const std::string& suffix);
extern boost::uuids::uuid createUuid();


}

}

#endif //SERVER_NAMEGENERATOR_H
