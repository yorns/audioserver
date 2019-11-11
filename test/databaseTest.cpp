#include <assert.h>
#include <boost/filesystem.hpp>

#include "../SimpleDatabase.h"

int main()
{
    boost::filesystem::path playlist_path{"playlist"};
    boost::filesystem::remove_all(playlist_path);
    boost::filesystem::create_directory(playlist_path);

    boost::filesystem::path mp3_path{"mp3"};
    boost::filesystem::remove_all(mp3_path);
    boost::filesystem::create_directory(mp3_path);


    SimpleDatabase database;

    database.loadAllPlaylists("playlist");

    assert (database.showAllPlaylists().size() == 0);

    database.createPlaylist("hoppelpoppel");

    assert (database.showAllPlaylists().size() == 1);
    assert (database.showPlaylist("hoppelpoppel").size() == 0);

    database.writeChangedPlaylists();

    database.addToPlaylist("hoppelpoppel", "firstSong.mp3");

    assert (database.showPlaylist("hoppelpoppel").size() == 1);

    database.writeChangedPlaylists();

    return 0;
}