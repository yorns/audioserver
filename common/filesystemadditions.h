#ifndef FILESYSTEMADDITIONS_H
#define FILESYSTEMADDITIONS_H

#include <string>
#include <vector>
#include "FileType.h"
#include "Constants.h"

namespace Common {

struct FileNameType {
    std::string name;
    std::string extension;
};

namespace FileSystemAdditions {

    std::vector<FileNameType> getAllFilesInDir(FileType fileType);
    bool removeFile(FileType fileType, const std::string& uniqueID);
    std::string getFullQualifiedDirectory(FileType fileType);

}

}

#endif // FILESYSTEMADDITIONS_H
