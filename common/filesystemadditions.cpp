#include "filesystemadditions.h"

#include "common/logger.h"
#include "common/Constants.h"

#include <boost/filesystem.hpp>
#include "common/Constants.h"

using namespace LoggerFramework;
namespace filesys =  boost::filesystem;

namespace Common {
namespace FileSystemAdditions {

std::vector<FileNameType> getAllFilesInDir(FileType fileType) {

    std::string dirPath { getFullQualifiedDirectory(fileType) };

    // Create a vector of string
    std::vector<FileNameType> listOfFiles;
    try {

        if (filesys::exists(dirPath) && filesys::is_directory(dirPath)) {

            std::system_error error_code;
            filesys::recursive_directory_iterator it = filesys::recursive_directory_iterator(dirPath,filesys::symlink_option::recurse);

            for(auto& p: it) {
                if (filesys::is_regular_file(p.path()))
                    listOfFiles.push_back(
                    {p.path().parent_path().string(),
                     p.path().stem().string(),
                     p.path().extension().string()});
            }

        }
    }
    catch (std::system_error &e) {
        logger(Level::error) << "Exception :: " << e.what();
        listOfFiles.clear();
    }
    return listOfFiles;
}

bool removeFile(FileType fileType, const std::string& uniqueID)
{
    std::string dirPath { getFullQualifiedDirectory(fileType) };
    std::string filename = dirPath + '/' + uniqueID;

    try {
        // Check if given path exists and points to a directory
        if (filesys::exists(filename) && filesys::is_regular_file(filename)) {

            boost::system::error_code ec;
            auto success = filesys::remove(filename, ec);

            if (!success)
                logger(Level::warning) << "delete of file <"<<filename<<"> failed: " << ec <<"\n";

            return success;

        }
    }
    catch (std::system_error &e) {
        logger(Level::error) << "Exception :: " << e.what();
    }

    return false;
}

std::string getFullQualifiedDirectory(FileType fileType) {
    switch (fileType) {
    case FileType::AudioMp3:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::audioPathMp3);
    case FileType::AudioJson:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::audioPathJson);
    case FileType::PlaylistJson:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::playlistPathJson);
    case FileType::PlaylistM3u:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::playlistPathM3u);
    case FileType::Html:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::htmlPath);
    case FileType::Covers:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::coverPath);
    case FileType::CoversRelative:
        return '/' + std::string(ServerConstant::coverPathWeb);
    case FileType::Cache:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::cachePath);
    case FileType::Tag:
        return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::tagPath);

    }

    return std::string(ServerConstant::base_path) + '/' + std::string(ServerConstant::tmpPath);
}

std::string getFullName(FileNameType file) {

    std::string fileDirSeparator;
    if (!file.dir.empty() && file.dir.back() != '/')
        fileDirSeparator = "/";

    return  file.dir + fileDirSeparator + file.name + file.extension;
}

}
}
