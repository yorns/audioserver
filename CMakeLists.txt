cmake_minimum_required(VERSION 3.3)

project(audioServer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

message("server build")

#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-O0 -g")        ## Optimize
#    set(CMAKE_EXE_LINKER_FLAGS "-s")  ## Strip binary
#endif()

if (BOOST_DIR)
  set(BOOST_ROOT ${BOOST_DIR})
endif (BOOST_DIR)

if (WITH_UNREADABLE)
    set(UNREADABLE WITH_UNREADABLE)
endif ()

find_package( Threads REQUIRED)
find_package( Boost 1.70.0 REQUIRED COMPONENTS filesystem system regex locale)

include(createdirs)
include(FindPkgConfig)

pkg_search_module(TagLib REQUIRED taglib)

set(PLAYERINTERFACE
    ${CMAKE_SOURCE_DIR}/playerinterface/Player.h
    ${CMAKE_SOURCE_DIR}/playerinterface/Player.cpp
    ${CMAKE_SOURCE_DIR}/playerinterface/mpvplayer.h
    ${CMAKE_SOURCE_DIR}/playerinterface/mpvplayer.cpp
    )

set(WEBSERVER
    ${CMAKE_SOURCE_DIR}/common/Constants.h
    ${CMAKE_SOURCE_DIR}/webserver/Listener.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Listener.h
    ${CMAKE_SOURCE_DIR}/webserver/sessionhandler.cpp
    ${CMAKE_SOURCE_DIR}/webserver/sessionhandler.h
    ${CMAKE_SOURCE_DIR}/webserver/Session.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Session.h
    ${CMAKE_SOURCE_DIR}/webserver/databaseaccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/databaseaccess.h
    ${CMAKE_SOURCE_DIR}/webserver/playlistaccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/playlistaccess.h
    ${CMAKE_SOURCE_DIR}/webserver/playeraccess.cpp
    ${CMAKE_SOURCE_DIR}/webserver/playeraccess.h
    ${CMAKE_SOURCE_DIR}/webserver/websocketsession.cpp
    ${CMAKE_SOURCE_DIR}/webserver/websocketsession.h
    )

set(ID3INTERFACE
    ${CMAKE_SOURCE_DIR}/id3tagreader/Id3Info.h
    ${CMAKE_SOURCE_DIR}/id3tagreader/id3TagReader.cpp
    ${CMAKE_SOURCE_DIR}/id3tagreader/id3TagReader.h
    )

set(DATABASE
    ${CMAKE_SOURCE_DIR}/database/NameType.h
    ${CMAKE_SOURCE_DIR}/database/searchitem.h
    ${CMAKE_SOURCE_DIR}/database/searchaction.h
    ${CMAKE_SOURCE_DIR}/database/playlist.cpp
    ${CMAKE_SOURCE_DIR}/database/playlist.h
    ${CMAKE_SOURCE_DIR}/database/id3repository.cpp
    ${CMAKE_SOURCE_DIR}/database/id3repository.h
    ${CMAKE_SOURCE_DIR}/database/playlistcontainer.cpp
    ${CMAKE_SOURCE_DIR}/database/playlistcontainer.h
    ${CMAKE_SOURCE_DIR}/database/SimpleDatabase.cpp
    ${CMAKE_SOURCE_DIR}/database/SimpleDatabase.h
    )

set(COMMONCOMPONENTS
    ${CMAKE_SOURCE_DIR}/common/Constants.cpp
    ${CMAKE_SOURCE_DIR}/common/Constants.h
    ${CMAKE_SOURCE_DIR}/common/logger.h
    ${CMAKE_SOURCE_DIR}/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/common/Extractor.h
    ${CMAKE_SOURCE_DIR}/common/mime_type.cpp
    ${CMAKE_SOURCE_DIR}/common/mime_type.h
    ${CMAKE_SOURCE_DIR}/common/NameGenerator.h
    ${CMAKE_SOURCE_DIR}/common/NameGenerator.cpp
    ${CMAKE_SOURCE_DIR}/common/urlDecode.h
    ${CMAKE_SOURCE_DIR}/common/urlDecode.c
    ${CMAKE_SOURCE_DIR}/common/repeattimer.cpp
    ${CMAKE_SOURCE_DIR}/common/repeattimer.h
    ${CMAKE_SOURCE_DIR}/common/filesystemadditions.cpp
    ${CMAKE_SOURCE_DIR}/common/filesystemadditions.h
    ${CMAKE_SOURCE_DIR}/common/FileType.h
    )

add_executable(${PROJECT_NAME}
               ${CMAKE_SOURCE_DIR}/audioserver/audioServer.cpp
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
target_compile_definitions(${PROJECT_NAME} PUBLIC BOOST_BEAST_USE_STD_STRING_VIEW ${UNREADABLE})

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

#
# playerCmdl
#
add_executable( playerCmdl ${PLAYERINTERFACE}
    ${CMAKE_SOURCE_DIR}/common/Constants.cpp
    ${CMAKE_SOURCE_DIR}/common/Constants.h
    ${CMAKE_SOURCE_DIR}/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/common/logger.h
    ${CMAKE_SOURCE_DIR}/common/repeattimer.cpp
    ${CMAKE_SOURCE_DIR}/common/repeattimer.h
    ${CMAKE_SOURCE_DIR}/common/filesystemadditions.cpp
    ${CMAKE_SOURCE_DIR}/common/filesystemadditions.h
    ${CMAKE_SOURCE_DIR}/database/playlist.cpp
    ${CMAKE_SOURCE_DIR}/database/playlist.h
    ${CMAKE_SOURCE_DIR}/test/KeyHit.cpp
    ${CMAKE_SOURCE_DIR}/test/KeyHit.h
    ${CMAKE_SOURCE_DIR}/test/PlayerCmdl.cpp
    )

set_target_properties( playerCmdl PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options( playerCmdl PUBLIC -O2)
target_include_directories(
        playerCmdl
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(
        playerCmdl
        Threads::Threads
        ${Boost_LIBRARIES}
)

#
# loggerTest
#
add_executable( loggerTest
    ${CMAKE_SOURCE_DIR}/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/common/logger.h
    ${CMAKE_SOURCE_DIR}/test/loggerTest.cpp
    )

set_target_properties( loggerTest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options( loggerTest PUBLIC -O0 -g)
target_include_directories(
        loggerTest
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

#
# repeatTimerTeset
#
add_executable( repeatTimerTest
    ${CMAKE_SOURCE_DIR}/common/logger.cpp
    ${CMAKE_SOURCE_DIR}/common/logger.h
    ${CMAKE_SOURCE_DIR}/common/repeattimer.cpp
    ${CMAKE_SOURCE_DIR}/common/repeattimer.h
    ${CMAKE_SOURCE_DIR}/test/repeatTimerTest.cpp
    )
set_target_properties( repeatTimerTest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options( repeatTimerTest PUBLIC -O0 -g)
target_include_directories(
        repeatTimerTest
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(
        repeatTimerTest
        ${Boost_LIBRARIES}
        Threads::Threads
)

#
# mpvTest
#
add_executable( mpvTest
    ${COMMONCOMPONENTS}
    ${CMAKE_SOURCE_DIR}/test/mpvTest.cpp
    )
set_target_properties( mpvTest PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options( mpvTest PUBLIC -O0 -g)
target_include_directories(
        mpvTest
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(
        mpvTest
        ${Boost_LIBRARIES}
        Threads::Threads
)


install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

