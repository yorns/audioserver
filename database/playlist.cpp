#include "playlist.h"
#include "common/logger.h"
#include "common/Constants.h"
#include <boost/filesystem.hpp>
#include "common/filesystemadditions.h"

using namespace Database;
using namespace LoggerFramework;
using namespace Common;

namespace filesys =  boost::filesystem;

std::string Playlist::getUniqueID() const
{
    return m_uniqueID;
}

const std::vector<std::string> &Playlist::getPlaylist() const
{
    return m_playlist;
}

std::string Playlist::getName() const
{
    return m_humanReadableName;
}

bool Playlist::addToList(std::string &&audioUID) {
    m_playlist.emplace_back(audioUID);
    setChanged(Changed::isChanged);
    return true;
}

bool Playlist::delFromList(const std::string &audioUID) {
    auto it = std::find_if(std::begin(m_playlist), std::end(m_playlist),
                           [&audioUID](const std::string& id){ return audioUID == id; });

    if (it != std::end(m_playlist)) {
        logger(Level::info) << "removing file <"<<*it<<">\n";
        m_playlist.erase(it);
        setChanged(Changed::isChanged);
        return true;
    }

    return false;
}

Playlist::Playlist(const std::string &_internalPlaylistName, Persistent persistent, Changed changed)
    : m_uniqueID(_internalPlaylistName),
      m_changed(changed),
      m_persistent(persistent)
{}

Playlist::Playlist(const std::string &_internalPlaylistName, std::vector<std::string> &&playlist, Persistent persistent, Changed changed)
    : m_uniqueID(_internalPlaylistName), m_playlist(std::move(playlist)),
      m_changed(changed), m_persistent(persistent)
{}

Changed Playlist::changed() const
{
    return m_changed;
}

void Playlist::setChanged(const Changed &changed)
{
    m_changed = changed;
}

Persistent Playlist::persistent() const
{
    return m_persistent;
}

void Playlist::setPersistent(const Persistent &persistent)
{
    m_persistent = persistent;
}



bool Playlist::read()
{
    std::string fullFilename = FileSystemAdditions::getFullQualifiedDirectory(FileType::Playlist)+ '/' + getUniqueID() + ".m3u";

    logger(LoggerFramework::Level::debug) << "reading playlist file <"<<fullFilename<<">\n";

    std::ifstream stream(fullFilename);

    std::string line;
    std::string playlistName;

    if (!stream.good())
        return false;

    // read human readable playlist name or only first line
    if (stream.good() && std::getline(stream, line)) {
        if (line.length() > 2 && line[0] == '#' && line[1] == ' ') {
            setName(line.substr(2));
        } else {
            filesys::path fullName(line);
            if (!fullName.stem().empty()) {
                std::string audioUniqueID { fullName.stem().string() };
                addToList(std::move(audioUniqueID));
                logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
            }
        }
    }

    logger(LoggerFramework::Level::debug) << "playlist name: "<<getUniqueID()<<"\n";

    while (stream.good() && std::getline(stream, line)) {
        filesys::path fullName(line);
        if (!fullName.stem().empty()) {
            std::string audioUniqueID { fullName.stem().string() };
            addToList(std::move(audioUniqueID));
            logger(LoggerFramework::Level::debug) << "  reading: <" << fullName.stem() << ">\n";
        }
    }

    return true;

}

bool Playlist::write()
{
    bool success { false };
    if ( persistent() == Persistent::isPermanent &&
         changed() == Changed::isChanged ) {
        logger(LoggerFramework::Level::debug) << "writeChangedPlaylist <" << getUniqueID()
                                              << "> realname <" << getName() << ">"
                                              << " - try write ... \n";

        std::string filename = FileSystemAdditions::getFullQualifiedDirectory(Common::FileType::Playlist)
                + "/" + getUniqueID() + ".m3u";

        std::ofstream ofs ( filename, std::ios_base::out );

        if (ofs.good()) {
            ofs << "# " << getName() << "\n";

            for (const auto &entry : getPlaylist()) {
                ofs << "../" << ServerConstant::audioPath << "/" << entry << ".mp3\n";
            }
            success = true;
            logger(LoggerFramework::Level::debug) << " done \n";

        } else {
            logger(LoggerFramework::Level::warning) << "writing to file <" << filename << "> failed \n";
            success = false;
        }
    } else {
        success = true;
        if (persistent() == Persistent::isTemporal)
            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " is temporal and will not persisted\n";
        else
            logger(LoggerFramework::Level::debug) << "Playlist " << getName() << " has not been changed\n";
    }

    if (success)
        setChanged(Changed::isUnchanged);

    return success;
}
