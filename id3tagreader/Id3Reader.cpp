#include "Id3Reader.h"
#include <boost/filesystem.hpp>

// TODO: use https://taglib.org/

std::string Id3Reader::readTextChaos(ID3_Field &field) {
    ID3_TextEnc encoding {field.GetEncoding()};
    std::size_t size{0};
    std::string text;

    if (encoding == ID3_TextEnc::ID3TE_UNICODE) {
        std::array<unicode_t, 1024> buffer_uc;
        size = field.Get(buffer_uc.data(), buffer_uc.size());
        std::wstring wstr((char*)buffer_uc.data(), ((char*)buffer_uc.data())+size);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        text = convert.to_bytes(wstr);
    }

    if (encoding == ID3_TextEnc::ID3TE_UTF8) {
        std::array<char, 1024> buffer;
        size = field.Get(buffer.data(), buffer.size());
        text = std::string(buffer.data());
    }

    if (encoding == ID3_TextEnc::ID3TE_ISO8859_1)
    {
        std::array<char, 1024> buffer;
        field.Get(buffer.data(), buffer.size());
        std::string str(buffer.data());//, buffer.data()+size);
        text = boost::locale::conv::to_utf<char>(str,"Latin1");
    }

    if (encoding == ID3_TextEnc::ID3TE_UTF16) {
        std::cerr << "ERROR: UTF16 multilanguage is not supported by id3lib. Conversion may be weird\n";
        std::array<unicode_t, 1024> buffer_uc;
        size = field.Get(buffer_uc.data(), buffer_uc.size());
        std::wstring wstr((char*)buffer_uc.data(), ((char*)buffer_uc.data())+size);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        text = convert.to_bytes(wstr);
    }

    return text;
}

Id3Info Id3Reader::getInfo(const std::string &mp3File) {

    Id3Info id3Info;

    boost::filesystem::path path { mp3File };
    id3Info.uid = path.stem().string();
    id3Info.track_no = 1;
    id3Info.all_tracks_no = 1;

    std::cout << "reading file <"<<mp3File<<">\n";//> -> <"<< id3Info.uid <<">\n";

    ID3_Tag myTag(mp3File.c_str());

    ID3_Tag::Iterator* iter = myTag.CreateIterator();
    ID3_Frame* myFrame = NULL;

    // run through all frames of this file
    while (NULL != (myFrame = iter->GetNext()))
    {

        std::cerr << "\nreading Frame ID: " <<(int) myFrame->GetID() << " [" << myFrame->GetTextID() <<" : "
                  << myFrame->GetDescription() <<"] size: "
                  << myFrame->GetDataSize() << " number of fields: "<< myFrame->NumFields() << "\n";

        std::array<unicode_t, 1024> buffer_uc;
        std::array<char, 1024> buffer_char;
        std::string text;
        std::size_t size;

        /* I guess the API is not made for the user, but only runs through the data and fills some structs
         * (API follows input data paradigm) */
        /* there are implicit dependencies, what is a bad design in id3v2 */

        /* so get that all together: if there is a text, find out, what encryption it is */
        ID3_TextEnc textEncoding { ID3_TextEnc::ID3TE_ISO8859_1 /* use latin1 as default */ };

        /* Contains() refers to frames :D */
        if (myFrame->Contains(ID3_FieldID::ID3FN_TEXTENC)) {
            /* no clue what this thing is for, as it does not give any hint to anything */
        }

        if (myFrame->Contains(ID3_FieldID::ID3FN_TEXT)) {
            text = readTextChaos(myFrame->Field(ID3_FieldID::ID3FN_TEXT));
            std::cerr << "extract textfield: "<<text<<"\n";
        }

        if (myFrame->GetID() == ID3_FrameID::ID3FID_TITLE) {
            id3Info.titel_name = text;
            std::cout << "Titel: "<<text<<"\n";
        }

        if (myFrame->GetID() == ID3_FrameID::ID3FID_ALBUM) {
            id3Info.album_name = text;
            std::cout << "Album: "<<text<<"\n";
        }

        if (myFrame->GetID() == ID3_FrameID::ID3FID_LEADARTIST) {
            id3Info.performer_name = text;
            std::cout << "Performer: "<<text<<"\n";
        }

        if (myFrame->GetID() == ID3_FrameID::ID3FID_TRACKNUM) {
            std::cerr << "TrackInfo: "<<text<<"\n";
            std::smatch match{};
            const std::regex pattern1{"(\\d*)/(\\d*)"};
            if (std::regex_search(text, match, pattern1)) {
                try {
                    id3Info.track_no = boost::lexical_cast<uint32_t>(match[1]);
                    id3Info.all_tracks_no = boost::lexical_cast<uint32_t>(match[2]);
                } catch (...) {}
            }
        }


    }
    delete iter;

    return id3Info;

}
