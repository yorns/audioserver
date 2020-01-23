#include "NameGenerator.h"

NameGenerator::GenerationName NameGenerator::create(const std::__cxx11::string &prefix, const std::__cxx11::string &suffix) {

    GenerationName retName;
    boost::uuids::random_generator generator;
    boost::uuids::uuid name = generator();
    retName.unique_id = boost::lexical_cast<std::string>(name);
    retName.filename = prefix + "/" + retName.unique_id + suffix;

    return retName;
}
