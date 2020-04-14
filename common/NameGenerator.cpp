#include "NameGenerator.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace Common {

NameGenerator::GenerationName NameGenerator::create(const std::string &prefix, const std::string &suffix) {

    GenerationName retName;
    boost::uuids::random_generator generator;
    boost::uuids::uuid name = generator();
    retName.unique_id = boost::lexical_cast<std::string>(name);
    if (!prefix.empty()) {
        retName.filename = prefix + "/" + retName.unique_id + suffix;
    }
    else {
        retName.filename = retName.unique_id + suffix;
    }

    return retName;
}


}
