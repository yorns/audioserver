#ifndef SERVER_ID3READER_H
#define SERVER_ID3READER_H

#include <id3/tag.h>
#include <map>
#include <string>
#include <locale>
#include <codecvt>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include "Id3Info.h"

class Id3Reader {

    std::map<ID3_FieldID, std::string> id3_fieldId_to_name {
            { ID3FN_NOFIELD,    " No field "},
            { ID3FN_TEXTENC,        " Text encoding (unicode or ASCII) "},
            { ID3FN_TEXT,           " Text field "},
            { ID3FN_URL,            " A URL "},
            { ID3FN_DATA,           " Data field "},
            { ID3FN_DESCRIPTION,    " Description field "},
            { ID3FN_OWNER,          " Owner field "},
            { ID3FN_EMAIL,          " Email field "},
            { ID3FN_RATING,         " Rating field "},
            { ID3FN_FILENAME,       " Filename field "},
            { ID3FN_LANGUAGE,       " Language field "},
            { ID3FN_PICTURETYPE,    " Picture type field "},
            { ID3FN_IMAGEFORMAT,    " Image format field "},
            { ID3FN_MIMETYPE,       " Mimetype field "},
            { ID3FN_COUNTER,        " Counter field "},
            { ID3FN_ID,             " Identifier/Symbol field "},
            { ID3FN_VOLUMEADJ,      " Volume adjustment field "},
            { ID3FN_NUMBITS,        " Number of bits field "},
            { ID3FN_VOLCHGRIGHT,    " Volume chage on the right channel "},
            { ID3FN_VOLCHGLEFT,     " Volume chage on the left channel "},
            { ID3FN_PEAKVOLRIGHT,   " Peak volume on the right channel "},
            { ID3FN_PEAKVOLLEFT,    " Peak volume on the left channel "},
            { ID3FN_TIMESTAMPFORMAT," SYLT Timestamp Format "},
            { ID3FN_CONTENTTYPE,    " SYLT content type "},
            { ID3FN_LASTFIELDID,     " Last field placeholder "}
    };

    std::map<ID3_TextEnc, std::string> id3_textEnc_to_name{
            {ID3TE_NONE, "ID3TE_NONE"},
            {ID3TE_ISO8859_1, "ID3TE_ISO8859_1"},
            {ID3TE_UTF16, "ID3TE_UTF16"},
            {ID3TE_UTF16BE, "ID3TE_UTF16BE"},
            {ID3TE_UTF8, "ID3TE_UTF8"},
            {ID3TE_NUMENCODINGS, "ID3TE_NUMENCODINGS"}
    };

    std::string readTextChaos(ID3_Field& field);


public:


    Id3Info getInfo(const std::string& mp3File);

};

#endif //SERVER_ID3READER_H
