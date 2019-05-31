cmake_minimum_required(VERSION 3.3)

project(server)

message("server build")

set(BOOST_INCLUDEDIR /opt/boost/include)

find_package( Threads REQUIRED)
find_package( OpenSSL REQUIRED)
find_package( Boost 1.65.0 REQUIRED COMPONENTS filesystem system regex locale)

#add_definitions("-std=c++17")

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
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	${OPENSSL_INCLUDE_DIR}
	${BOOST_INCLUDEDIR} )

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

add_executable(databaseTest test/databaseTest.cpp SimpleDatabase.cpp)

target_link_libraries(databaseTest
		Threads::Threads
		${Boost_LIBRARIES}
		id3
		)

target_include_directories(
		databaseTest
		PUBLIC
		${CMAKE_SOURCE_DIR}
		${BOOST_INCLUDEDIR}
)

set_target_properties(databaseTest PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)


# 	$ENV{HOME}/raspberrypi/rootfs/usr/include/arm-linux-gnueabihf
# set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "--sysroot=$ENV{HOME}/raspberrypi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot -std=c++11")

# rsync -rl --delete-after --safe-links pi@192.168.2.115:/{lib,usr} $HOME/raspberrypi/rootfs

