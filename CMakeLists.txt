cmake_minimum_required(VERSION 3.3)

project(audioServer)

message("server build")

if (BOOST_DIR)
  set(BOOST_ROOT ${BOOST_DIR})
endif (BOOST_DIR)

find_package( Threads REQUIRED)
find_package( OpenSSL REQUIRED)
find_package( Boost 1.69.0 REQUIRED COMPONENTS filesystem system regex locale)

include(GNUInstallDirs)
include(FindPkgConfig)

pkg_search_module(TagLib REQUIRED taglib)

set( AUDIO_DATA_DIR ${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver )
set( AUDIO_MP3_DIR ${AUDIO_DATA_DIR}/mp3 )
set( AUDIO_TMP_DIR ${AUDIO_DATA_DIR}/tmp )
set( AUDIO_HTML_DIR ${AUDIO_DATA_DIR}/html )
set( AUDIO_HTMLIMG_DIR ${AUDIO_DATA_DIR}/html/img )
set( AUDIO_HTMLJS_DIR ${AUDIO_DATA_DIR}/html/js )
set( AUDIO_HTMLCSS_DIR ${AUDIO_DATA_DIR}/html/css )
set( AUDIO_PLAYLIST_DIR ${AUDIO_DATA_DIR}/playlist )
set( AUDIO_PLAYERLOG_DIR ${AUDIO_DATA_DIR}/player_log )

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
    ${CMAKE_SOURCE_DIR}/webserver/RequestHandler.cpp
    ${CMAKE_SOURCE_DIR}/webserver/RequestHandler.h
    ${CMAKE_SOURCE_DIR}/webserver/Session.cpp
    ${CMAKE_SOURCE_DIR}/webserver/Session.h
    ${CMAKE_SOURCE_DIR}/common/server_certificate.hpp
    ${CMAKE_SOURCE_DIR}/common/urlDecode.h
    ${CMAKE_SOURCE_DIR}/common/urlDecode.c
    )

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
    ${CMAKE_SOURCE_DIR}/common/Extractor.h
    ${CMAKE_SOURCE_DIR}/common/mime_type.h
    ${CMAKE_SOURCE_DIR}/common/NameGenerator.h
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
        OpenSSL::SSL
        OpenSSL::Crypto
        ${Boost_LIBRARIES}
        ${TagLib_LIBRARIES}
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
        ${CMAKE_SOURCE_DIR}
	${OPENSSL_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
        ${TagLib_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "audioServer")
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic )
#target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

install ( DIRECTORY DESTINATION ${AUDIO_MP3_DIR} DIRECTORY_PERMISSIONS 
          OWNER_WRITE OWNER_READ OWNER_EXECUTE
	  GROUP_WRITE GROUP_READ GROUP_EXECUTE 
	  WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_TMP_DIR} DIRECTORY_PERMISSIONS
         OWNER_WRITE OWNER_READ OWNER_EXECUTE
          GROUP_WRITE GROUP_READ GROUP_EXECUTE
          WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_HTML_DIR} DIRECTORY_PERMISSIONS
	OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_HTMLIMG_DIR} DIRECTORY_PERMISSIONS
         OWNER_WRITE OWNER_READ OWNER_EXECUTE
          GROUP_WRITE GROUP_READ GROUP_EXECUTE
          WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_HTMLJS_DIR} DIRECTORY_PERMISSIONS
       OWNER_WRITE OWNER_READ OWNER_EXECUTE
        GROUP_WRITE GROUP_READ GROUP_EXECUTE
        WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_HTMLCSS_DIR} DIRECTORY_PERMISSIONS
       OWNER_WRITE OWNER_READ OWNER_EXECUTE
        GROUP_WRITE GROUP_READ GROUP_EXECUTE
        WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_PLAYLIST_DIR} DIRECTORY_PERMISSIONS
         OWNER_WRITE OWNER_READ OWNER_EXECUTE
          GROUP_WRITE GROUP_READ GROUP_EXECUTE
          WORLD_WRITE WORLD_READ WORLD_EXECUTE )
install ( DIRECTORY DESTINATION ${AUDIO_PLAYERLOG_DIR} DIRECTORY_PERMISSIONS
         OWNER_WRITE OWNER_READ OWNER_EXECUTE
          GROUP_WRITE GROUP_READ GROUP_EXECUTE
          WORLD_WRITE WORLD_READ WORLD_EXECUTE )
  
install ( FILES ${CMAKE_SOURCE_DIR}/html/index.html 
	  DESTINATION ${AUDIO_HTML_DIR}
	  PERMISSIONS
	  OWNER_READ OWNER_WRITE
          GROUP_READ
          WORLD_READ )

install ( FILES
         ${CMAKE_SOURCE_DIR}/html/js/bootstrap.min.js
         ${CMAKE_SOURCE_DIR}/html/js/jquery-3.4.1.min.js
         ${CMAKE_SOURCE_DIR}/html/js/popper.min.js
        DESTINATION ${AUDIO_HTMLJS_DIR}
        PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ )

install ( FILES
         ${CMAKE_SOURCE_DIR}/html/css/bootstrap.min.css
        DESTINATION ${AUDIO_HTMLCSS_DIR}
        PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ )

