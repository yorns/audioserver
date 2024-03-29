#ifndef FILETYPE_H
#define FILETYPE_H

namespace Common {

enum class FileType {
    AudioMp3,
    AudioJson,
    PlaylistJson,
    PlaylistM3u,
    Covers,
    CoversRelative,
    Html,
    Cache,
    Tag,
    Credential
};

}

#endif // FILETYPE_H
