#ifndef DATABASESEARCHITEM_H
#define DATABASESEARCHITEM_H

namespace Database {

enum class SearchItem {
    unknown,
    title,
    album,
    performer,
    uid,
    overall,
    album_and_interpret
};

}
#endif // DATABASESEARCHITEM_H
