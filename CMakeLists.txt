cmake_minimum_required(VERSION 3.3)

project(server)

message("server build")

if (BOOST_DIR)
  set(BOOST_ROOT ${BOOST_DIR})
endif (BOOST_DIR)

find_package( Threads REQUIRED)
find_package( OpenSSL REQUIRED)
find_package( Boost 1.65.0 REQUIRED COMPONENTS filesystem system regex locale)

include(GNUInstallDirs)
include(FindPkgConfig)

pkg_search_module(TagLib REQUIRED taglib)

add_executable(${PROJECT_NAME}
		http_server.cpp
		common/Extractor.h
		common/urlDecode.c
		Player.cpp
		MPlayer.cpp
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
        ${TagLib_LIBRARIES}
)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	${OPENSSL_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
    ${TagLib_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "audioServer")
#target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

#add_executable(databaseTest test/databaseTest.cpp SimpleDatabase.cpp common/urlDecode.c)

#target_link_libraries(databaseTest
#		Threads::Threads
#		${Boost_LIBRARIES}
#		id3
#		${TagLib_LIBRARIES}
#		)

#target_include_directories(
#		databaseTest
#		PUBLIC
#		${CMAKE_SOURCE_DIR}
#		${Boost_INCLUDE_DIRS}
#		${TagLib_INCLUDE_DIRS}
#)

#set_target_properties(databaseTest PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)


# 	$ENV{HOME}/raspberrypi/rootfs/usr/include/arm-linux-gnueabihf
# set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "--sysroot=$ENV{HOME}/raspberrypi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot -std=c++11")

# rsync -rl --delete-after --safe-links pi@192.168.2.115:/{lib,usr} $HOME/raspberrypi/rootfs

