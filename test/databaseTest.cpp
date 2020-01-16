/*
 * mkdir /tmp/tmp
 * mkdir /tmp/playlist
 */

#include <assert.h>
#include <boost/filesystem.hpp>

#include "database/SimpleDatabase.h"
#include "common/NameGenerator.h"

boost::beast::string_view ServerConstant::base_path{"/tmp"};

int main()
{
    boost::filesystem::path playlist_path{ServerConstant::base_path.to_string() + "/" + "playlist"};
    boost::filesystem::remove_all( playlist_path);
    boost::filesystem::create_directory(playlist_path);

    boost::filesystem::path mp3_path{ServerConstant::base_path.to_string() + "/" + "mp3"};
    boost::filesystem::remove_all(mp3_path);
    boost::filesystem::create_directory(mp3_path);

    SimpleDatabase database;

    database.loadAllPlaylists("playlist");

    assert (database.showAllPlaylists().size() == 0);

    std::string uniqueId = database.createPlaylist("hoppelpoppel");

    assert (database.showAllPlaylists().size() == 1);
    assert (database.showPlaylist(uniqueId).size() == 0);

    database.writeChangedPlaylists();

    database.addToPlaylist("hoppelpoppel", "firstSong.mp3");

    assert (database.showPlaylist(uniqueId).size() == 1);

    database.writeChangedPlaylists();

    auto addToDatabase =[&database](std::string&& album, std::string&& title, std::string&& performer) {
        Id3Info info;
        auto generationInfo = NameGenerator::create("bubub", "mp3");
        info.uid = generationInfo.unique_id;
        info.album_name = album;
        info.titel_name = title;
        info.performer_name = performer;
        database.addToDatabase(std::move(info));
    };

    addToDatabase("Album1", "title1", "nubu");
    addToDatabase("Album1", "title2", "nubu");
    addToDatabase("Album1", "title3", "nubu");

    addToDatabase("Blbum1", "title1", "nubu");
    addToDatabase("Blbum1", "title2", "nubu");
    addToDatabase("Blbum1", "title3", "nubu");

    addToDatabase("Album2", "title1", "naba");
    addToDatabase("Album2", "title2", "naba");
    addToDatabase("Album2", "title3", "naba");

    auto album1 = database.findAlbum("Album1");

    assert (album1.size() == 1);
    assert (album1[0].album_name == "Album1");

    auto album2 = database.findAlbum("Album2");

    assert (album2.size() == 1);
    assert (album2[0].album_name == "Album2");

    auto blbum1 = database.findAlbum("Blbum1");

    assert (blbum1.size() == 1);
    assert (blbum1[0].album_name == "Blbum1");

    auto small = database.findAlbum("1");
    assert (small.size() == 2);
    assert (small[0].album_name == "Album1");
    assert (small[1].album_name == "Blbum1");

    auto small2 = database.findAlbum("lbum");
    assert (small2.size() == 3);
    assert (small2[0].album_name == "Album1");
    assert (small2[1].album_name == "Blbum1");
    assert (small2[2].album_name == "Album2");

    auto performer1 = database.findAlbum("nubu");
    assert (performer1.size() == 2);
    assert (performer1[0].album_name == "Album1");
    assert (performer1[1].album_name == "Blbum1");

    auto performer2 = database.findAlbum("n");
    assert (performer2.size() == 3);
    assert (performer2[0].album_name == "Album1");
    assert (performer2[1].album_name == "Blbum1");
    assert (performer2[2].album_name == "Album2");

    return 0;
}
