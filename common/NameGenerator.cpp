#include "NameGenerator.h"

NameGenerator::GenerationName NameGenerator::create(const std::string &prefix, const std::string &suffix) {

    GenerationName retName;
    boost::uuids::random_generator generator;
    boost::uuids::uuid name = generator();
    retName.unique_id = boost::lexical_cast<std::string>(name);
    retName.filename = prefix + "/" + retName.unique_id + suffix;

    return retName;
}