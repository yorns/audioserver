cmake_minimum_required(VERSION 3.15)

project(audioServer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# in case this should be generated for an external system, all libs should be static
set(Boost_USE_STATIC_LIBS ON)

# we use boost information from cmake -DBOOST_DIR
# typically cmake -DBOOST_DIR=/opt/booooost/ ..
if (BOOST_DIR)
  set(Boost_NO_BOOST_CMAKE ON)
  set(BOOST_ROOT ${BOOST_DIR}) # This can be also set via cmake -DBOOST_ROOT=...
  message(STATUS "Boost root dir set to ${BOOST_ROOT}")
endif (BOOST_DIR)

# Required dependencies on ubuntu: atp install libboost-filesystem-dev libboost-regex-dev libtag1-dev libgstreamer1.0-dev nlohmann-json3-dev

find_package(snc QUIET CONFIG)
if(NOT snc_FOUND)
    include(FetchContent)
    message(INFO "fetching snc git")
    FetchContent_Declare(
      snc
      GIT_REPOSITORY https://github.com/yorns/snc.git
      GIT_TAG        master
    )
    FetchContent_MakeAvailable(snc)
endif()

find_package(Threads REQUIRED)
find_package(Boost 1.70.0 REQUIRED COMPONENTS filesystem regex)
find_package(nlohmann_json REQUIRED)

include(createdirs)
include(FindPkgConfig)

if (NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "no pkgConfig found in your system")
endif()

# taglib is mandatory
pkg_search_module(TagLib REQUIRED IMPORTED_TARGET taglib)

# gstreamer can be omitted
if (WITH_GST)
  message("User request, so find gstreamer libraries")
  pkg_check_modules(GST IMPORTED_TARGET gstreamer-1.0)
endif()

if (NOT GST_FOUND)
    message(STATUS "gstreamer not found, web only version is generated!")
else()
    message(STATUS "gstreamer found!")
    option(HAVE_GST "Enable GStreamer" ON)
endif()

configure_file(cmake/config.h.in config/system_config.h)

add_library(common)
add_subdirectory(common)
target_compile_features(common PUBLIC cxx_std_17)
target_compile_definitions(common PUBLIC BOOST_BEAST_USE_STD_STRING_VIEW)
target_include_directories(common
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)
if (GST_FOUND)
target_link_libraries(common
    PUBLIC
    Boost::boost
    snc::snc_client
    PkgConfig::GST
)
else (GST_FOUND)
target_link_libraries(common
    PUBLIC
    Boost::boost
    snc::snc_client
)
endif (GST_FOUND)

add_library(id3tagreader)
add_subdirectory(id3tagreader)
target_compile_features(id3tagreader PUBLIC cxx_std_17)
target_include_directories(id3tagreader
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(id3tagreader
    PUBLIC
    nlohmann_json::nlohmann_json
    Boost::boost
    Boost::filesystem
    PkgConfig::TagLib
)

add_library(database)
add_subdirectory(database)
target_compile_features(database PUBLIC cxx_std_17)
target_include_directories(database
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(database PUBLIC id3tagreader)
target_link_libraries(database PUBLIC
    Boost::boost
)


add_library(playerinterface)
add_subdirectory(playerinterface)
target_compile_features(playerinterface PUBLIC cxx_std_17)
target_include_directories(playerinterface
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>

)
if (GST_FOUND)
    target_link_libraries(playerinterface
        PUBLIC
        Boost::boost
        PkgConfig::GST
    )
else (GST_FOUND)
target_link_libraries(playerinterface
    PUBLIC
    Boost::boost
)
endif(GST_FOUND)


add_executable(${PROJECT_NAME})
target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)

add_subdirectory(audioserver)
add_subdirectory(webserver)
add_subdirectory(wifi)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        database
        playerinterface
        common
        snc::snc_client
        Boost::boost
        Boost::regex
        Boost::headers
        Threads::Threads
)

if (WITH_UNREADABLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC WITH_UNREADABLE)
endif ()

message("BOOST: include dir is set to ${Boost_INCLUDE_DIRS}")

add_executable(ituneJsonConverter)
add_subdirectory(itunesJsonConverter)
target_link_libraries(
    ituneJsonConverter
    PUBLIC
        common
        nlohmann_json::nlohmann_json
        Boost::boost
        Boost::headers
        Threads::Threads
)

add_executable(wpaGenerator)
add_subdirectory(wpa_generator)
target_compile_features(wpaGenerator PUBLIC cxx_std_17)
target_link_libraries(wpaGenerator
    PUBLIC
    nlohmann_json::nlohmann_json
    Boost::boost
    Boost::headers
    Threads::Threads
)


if (WITH_ADD)

    add_executable( mp3CoverExtractor)
    add_subdirectory(extractId3Cover)
    target_link_libraries(
            mp3CoverExtractor
            PUBLIC
            common
            PkgConfig::TagLib
            Boost::filesystem
            Boost::headers
            Threads::Threads
    )

    add_executable( uniqueNameGenerator)
    add_executable( databaseTest)
    add_executable( playerCmdl)
    add_executable( loggerTest)
    add_executable( repeatTimerTest)
    add_executable( mpvTest)
    add_executable( gstTest)
    add_executable( urlTest)

    add_subdirectory(test)

    target_link_libraries(
            uniqueNameGenerator
            PUBLIC
            common
            PkgConfig::TagLib
            Boost::headers
            Threads::Threads
    )

    target_link_libraries(
            databaseTest
            PUBLIC
            database
            common
            Boost::headers
            Threads::Threads
    )

    target_link_libraries(
            playerCmdl
            PRIVATE
            playerinterface
            database
            common
            Boost::headers
            Threads::Threads
    )

    target_link_libraries(loggerTest PUBLIC common)

    target_link_libraries(
            repeatTimerTest
            PUBLIC
            common
            Boost::headers
            Threads::Threads
    )

    target_link_libraries(
            mpvTest
            PUBLIC
            common
            Boost::headers
            Threads::Threads
    )

if (NOT WITHOUT_GST)
    target_link_libraries(
            gstTest
            PUBLIC
            common
            PkgConfig::GST
            Boost::headers
            Threads::Threads
    )
endif()

    target_link_libraries(
            urlTest
            PUBLIC
            common
            Boost::headers
            Threads::Threads
    )

endif(WITH_ADD)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

# cmake -DCMAKE_INSTALL_PREFIX=$OECORE_TARGET_SYSROOT/usr/ ..
install(TARGETS wpaGenerator
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

