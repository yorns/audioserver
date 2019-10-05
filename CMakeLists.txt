cmake_minimum_required(VERSION 3.3)

project(server)

message("server build")

set(BOOST_ROOT /opt/boost_1_71/)

find_package( Threads REQUIRED)
find_package( OpenSSL REQUIRED)
find_package( Boost 1.65.0 REQUIRED COMPONENTS filesystem system regex locale)

include(FindPkgConfig)
pkg_search_module(TagLib REQUIRED taglib)

add_executable(${PROJECT_NAME}
		http_server_async_ssl.cpp
		common/Extractor.h
		Player.cpp
		MPlayer.cpp
		Id3Reader.cpp
		SimpleDatabase.cpp
		Listener.cpp
		Session.cpp
		RequestHandler.cpp
		)

target_link_libraries(${PROJECT_NAME}
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        ${Boost_LIBRARIES}
        id3
        ${TagLib_LIBRARIES}
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	${OPENSSL_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
    ${TagLib_INCLUDE_DIRS}
)

message(STATUS "Taglib include: 		${TagLib_INCLUDE_DIRS}")
message(STATUS "Taglib libraries: 		${TagLib_LIBRARIES}")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g)

add_executable(databaseTest test/databaseTest.cpp SimpleDatabase.cpp)

target_link_libraries(databaseTest
		Threads::Threads
		${Boost_LIBRARIES}
		id3
		${TagLib_LIBRARIES}
		)

target_include_directories(
		databaseTest
		PUBLIC
		${CMAKE_SOURCE_DIR}
		${Boost_INCLUDE_DIRS}
		${TagLib_INCLUDE_DIRS}
)

set_target_properties(databaseTest PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)


# 	$ENV{HOME}/raspberrypi/rootfs/usr/include/arm-linux-gnueabihf
# set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "--sysroot=$ENV{HOME}/raspberrypi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot -std=c++11")

# rsync -rl --delete-after --safe-links pi@192.168.2.115:/{lib,usr} $HOME/raspberrypi/rootfs

