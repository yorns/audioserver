#ifndef SERVER_NAMEGENERATOR_H
#define SERVER_NAMEGENERATOR_H

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace NameGenerator {

    static std::string create(const std::string& prefix, const std::string& suffix) {

        boost::uuids::random_generator generator;
        boost::uuids::uuid name = generator();
        std::stringstream file_name_str;
        file_name_str << prefix << name << suffix;

        return file_name_str.str();
    }
}

#endif //SERVER_NAMEGENERATOR_H
