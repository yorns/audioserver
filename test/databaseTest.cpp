/*
 * mkdir /tmp/tmp
 * mkdir /tmp/playlist
 */

#include <assert.h>
#include <boost/filesystem.hpp>

#define WITH_UNITTEST

#include "database/SimpleDatabase.h"
#include "common/NameGenerator.h"

using namespace Database;

void test_database_base()
{
    boost::filesystem::path playlist_path{std::string(ServerConstant::base_path) + "/" + "playlist"};
    boost::filesystem::remove_all( playlist_path);
    boost::filesystem::create_directory(playlist_path);

    boost::filesystem::path mp3_path{std::string(ServerConstant::base_path) + "/" + "mp3"};
    boost::filesystem::remove_all(mp3_path);
    boost::filesystem::create_directory(mp3_path);

    SimpleDatabase database;

    database.loadDatabase();

    assert (database.getAllPlaylists().size() == 0);

    std::string playlistRealName { "hoppelpoppel" };

    auto pl = database.createPlaylist(playlistRealName, Persistent::isPermanent);

    assert (pl.has_value());

    assert (database.getAllPlaylists().size() == 1);
    assert (database.getAllPlaylists()[0].first == playlistRealName);

    database.writeChangedPlaylists();

    database.addToPlaylistName("hoppelpoppel", "firstSong");

    assert (database.getPlaylistByUID(*pl) && database.getPlaylistByUID(*pl)->size() == 1);
    assert (database.getPlaylistByName(playlistRealName) && database.getPlaylistByName(playlistRealName)->size() == 1);

    assert (database.setCurrentPlaylistUniqueId(*pl));

    assert (database.getCurrentPlaylistUniqueID());
    assert (*database.getCurrentPlaylistUniqueID() == *pl);
    assert (database.convertPlaylist(*pl, Database::NameType::uniqueID) && *database.convertPlaylist(*pl, Database::NameType::uniqueID) == playlistRealName);

    database.writeChangedPlaylists();

}

void test_id3_repository() {

    boost::filesystem::path playlist_path{std::string(ServerConstant::base_path) + "/" + "playlist"};
    boost::filesystem::remove_all( playlist_path);
    boost::filesystem::create_directory(playlist_path);

    boost::filesystem::path mp3_path{std::string(ServerConstant::base_path) + "/" + "mp3"};
    boost::filesystem::remove_all(mp3_path);
    boost::filesystem::create_directory(mp3_path);

    SimpleDatabase database;

    database.loadDatabase();

    auto addToDatabase =[&database](std::string&& album, std::string&& title, std::string&& performer) {
        Id3Info info;
        auto generationInfo = Common::NameGenerator::create("bubub", "mp3");
        info.uid = generationInfo.unique_id;
        info.album_name = album;
        info.title_name = title;
        info.performer_name = performer;
        database.testInsert(std::move(info));
    };

    addToDatabase("Album1", "title1", "nubu");
    addToDatabase("Album1", "title2", "nubu");
    addToDatabase("Album1", "title3", "nubu");

    addToDatabase("Blbum1", "title1", "nubu");
    addToDatabase("Blbum1", "title2", "nabu");
    addToDatabase("Blbum1", "title3", "nuba");

    addToDatabase("Album2", "title1", "naba");
    addToDatabase("Album2", "title2", "naba");

    {
        auto album = database.searchAudioItems("Album1", Database::SearchItem::album, SearchAction::exact);

        assert (album.size());
        assert (album.size() == 3);
        assert (album[0].title_name == "title1");
        assert (album[1].title_name == "title2");
        assert (album[2].title_name == "title3");
    }
    {
        auto album = database.searchAudioItems("Album2", Database::SearchItem::album, SearchAction::exact);

        assert(album.size());
        assert (album.size() == 2);
        assert (album[0].title_name == "title1");
        assert (album[1].title_name == "title2");
    }
    {
        auto album = database.searchAudioItems("Blbum1", Database::SearchItem::album, SearchAction::exact);

        assert (album.size());
        assert (album.size() == 3);
        assert (album[0].performer_name == "nubu");
        assert (album[1].performer_name == "nabu");
        assert (album[2].performer_name == "nuba");
    }
    {
        auto album = database.searchAudioItems("1", Database::SearchItem::album, SearchAction::exact);

        assert (album.size());
        assert (album.size() == 6);
        assert (album[0].title_name == "title1");
        assert (album[1].title_name == "title2");
        assert (album[2].title_name == "title3");
        assert (album[3].performer_name == "nubu");
        assert (album[4].performer_name == "nabu");
        assert (album[5].performer_name == "nuba");
    }
    {
        auto small2 = database.searchAudioItems("1", Database::SearchItem::album, Database::SearchAction::alike);
        assert (small2.size());
        assert (small2.size() == 2);
    }

//    auto small2 = database.findAlbum("lbum");
//    assert (small2.size() == 3);
//    assert (small2[0].album_name == "Album1");
//    assert (small2[1].album_name == "Blbum1");
//    assert (small2[2].album_name == "Album2");

//    auto performer1 = database.findAlbum("nubu");
//    assert (performer1.size() == 2);
//    assert (performer1[0].album_name == "Album1");
//    assert (performer1[1].album_name == "Blbum1");

//    auto performer2 = database.findAlbum("n");
//    assert (performer2.size() == 3);
//    assert (performer2[0].album_name == "Album1");
//    assert (performer2[1].album_name == "Blbum1");
//    assert (performer2[2].album_name == "Album2");

}

int main() {

    globalLevel = Level::debug;
    ServerConstant::base_path = ServerConstant::sv{"/tmp"};

    assert(utility::urlConvert("F%C3%BCnf") == "Fünf");

    test_database_base();
    test_id3_repository();

    return 0;
}
