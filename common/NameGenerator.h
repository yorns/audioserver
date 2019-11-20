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

    static GenerationName create(const std::string& prefix, const std::string& suffix) {

        GenerationName retName;
        boost::uuids::random_generator generator;
        boost::uuids::uuid name = generator();
        retName.unique_id = boost::lexical_cast<std::string>(name);
        retName.filename = prefix + "/" + retName.unique_id + suffix;

        return retName;
    }
}

#endif //SERVER_NAMEGENERATOR_H
