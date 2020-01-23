cmake_minimum_required(VERSION 3.3)

project(audioServer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

message("server build")

if (BOOST_DIR)
  set(BOOST_ROOT ${BOOST_DIR})
endif (BOOST_DIR)

find_package( Threads REQUIRED)
find_package( Boost 1.69.0 REQUIRED COMPONENTS filesystem system regex locale)

include(createdirs)
include(FindPkgConfig)

pkg_search_module(TagLib REQUIRED taglib)

set(PLAYERINTERFACE
    ${CMAKE_SOURCE_DIR}/playerinterface/Player.h
    ${CMAKE_SOURCE_DIR}/playerinterface/Player.cpp
    ${CMAKE_SOURCE_DIR}/playerinterface/mpvplayer.h
    ${CMAKE_SOURCE_DIR}/playerinterface/mpvplayer.cpp
    )
# ${CMAKE_SOURCE_DIR}/playerinterface/MPlayer.h
# ${CMAKE_SOURCE_DIR}/playerinterface/MPlayer.cpp

set(WEBSERVER
    ${CMAKE_SOURCE_DIR}/common/Constants.h
    ${CMAKE_SOURCE_DIR}/webserver/http_server.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Listener.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Listener.h
    ${CMAKE_SOURCE_DIR}/webserver/Session.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Session.h
    ${CMAKE_SOURCE_DIR}/webserver/databaseaccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/databaseaccess.h
    ${CMAKE_SOURCE_DIR}/webserver/playlistaccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/playlistaccess.h
    ${CMAKE_SOURCE_DIR}/webserver/playeraccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/playeraccess.h
    )

#${CMAKE_SOURCE_DIR}/webserver/RequestHandler.cpp
#${CMAKE_SOURCE_DIR}/webserver/RequestHandler.h

set(ID3INTERFACE
    ${CMAKE_SOURCE_DIR}/id3tagreader/Id3Info.h
    ${CMAKE_SOURCE_DIR}/id3tagreader/id3TagReader.cpp
    ${CMAKE_SOURCE_DIR}/id3tagreader/id3TagReader.h
    )
#${CMAKE_SOURCE_DIR}/id3tagreader/Id3Reader.cpp
#${CMAKE_SOURCE_DIR}/id3tagreader/Id3Reader.h

set(DATABASE
    ${CMAKE_SOURCE_DIR}/database/SimpleDatabase.cpp
    ${CMAKE_SOURCE_DIR}/database/SimpleDatabase.h
    )

set(COMMONCOMPONENTS
    ${CMAKE_SOURCE_DIR}/common/logger.h
    ${CMAKE_SOURCE_DIR}/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/common/Extractor.h
    ${CMAKE_SOURCE_DIR}/common/mime_type.h
    ${CMAKE_SOURCE_DIR}/common/NameGenerator.h
    ${CMAKE_SOURCE_DIR}/common/NameGenerator.cpp
    ${CMAKE_SOURCE_DIR}/common/urlDecode.h
    ${CMAKE_SOURCE_DIR}/common/urlDecode.c
    )

add_executable(${PROJECT_NAME}
               ${PLAYERINTERFACE}
               ${WEBSERVER}
               ${ID3INTERFACE}
               ${DATABASE}
               ${COMMONCOMPONENTS}
               )

target_link_libraries(${PROJECT_NAME}
        Threads::Threads
        ${Boost_LIBRARIES}
        ${TagLib_LIBRARIES}
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
        ${CMAKE_SOURCE_DIR}
	${Boost_INCLUDE_DIRS}
        ${TagLib_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "audioServer")
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic )
target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g)

add_executable( mp3CoverExtractor ${CMAKE_SOURCE_DIR}/extractId3Cover/saveID3TagPicture.cpp ${COMMONCOMPONENTS})
set_target_properties(mp3CoverExtractor PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

target_include_directories(
        mp3CoverExtractor
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${TagLib_INCLUDE_DIRS}
)

target_link_libraries(
        mp3CoverExtractor
        Threads::Threads
        ${Boost_LIBRARIES}
        ${TagLib_LIBRARIES}
)

add_executable( databaseTest ${DATABASE} ${ID3INTERFACE} ${COMMONCOMPONENTS} test/databaseTest.cpp)
set_target_properties( databaseTest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(
        databaseTest
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${TagLib_INCLUDE_DIRS}
)
target_link_libraries(
        databaseTest
        Threads::Threads
        ${Boost_LIBRARIES}
        ${TagLib_LIBRARIES}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

