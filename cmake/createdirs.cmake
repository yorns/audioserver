include(GNUInstallDirs)

set( AUDIO_DATA_DIR ${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver )
set( AUDIO_MP3_DIR ${AUDIO_DATA_DIR}/mp3 )
set( AUDIO_TMP_DIR ${AUDIO_DATA_DIR}/tmp )
set( AUDIO_HTML_DIR ${AUDIO_DATA_DIR}/html )
set( AUDIO_HTMLIMG_DIR ${AUDIO_DATA_DIR}/html/img )
set( AUDIO_HTMLFONT_DIR ${AUDIO_DATA_DIR}/html/font )
set( AUDIO_HTMLJS_DIR ${AUDIO_DATA_DIR}/html/js )
set( AUDIO_HTMLCSS_DIR ${AUDIO_DATA_DIR}/html/css )
set( AUDIO_PLAYLIST_DIR ${AUDIO_DATA_DIR}/playlist )
set( AUDIO_PLAYERLOG_DIR ${AUDIO_DATA_DIR}/player_log )

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
install ( DIRECTORY DESTINATION ${AUDIO_HTMLFONT_DIR} DIRECTORY_PERMISSIONS
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
       ${CMAKE_SOURCE_DIR}/html/js/audioserver.js
      DESTINATION ${AUDIO_HTMLJS_DIR}
      PERMISSIONS
      OWNER_READ OWNER_WRITE
      GROUP_READ
      WORLD_READ )

  install ( FILES
         ${CMAKE_SOURCE_DIR}/html/font/fontello.svg
         ${CMAKE_SOURCE_DIR}/html/font/fontello.ttf
         ${CMAKE_SOURCE_DIR}/html/font/fontello.woff
         ${CMAKE_SOURCE_DIR}/html/font/fontello.woff2
        DESTINATION ${AUDIO_HTMLFONT_DIR}
        PERMISSIONS
        OWNER_READ OWNER_WRITE
        GROUP_READ
        WORLD_READ )


install ( FILES
       ${CMAKE_SOURCE_DIR}/html/css/bootstrap.min.css
       ${CMAKE_SOURCE_DIR}/html/css/fontello.css
      DESTINATION ${AUDIO_HTMLCSS_DIR}
      PERMISSIONS
      OWNER_READ OWNER_WRITE
      GROUP_READ
      WORLD_READ )

install (FILES
      ${CMAKE_SOURCE_DIR}/html/img/unknown.png
      DESTINATION ${AUDIO_HTMLIMG_DIR}
      PERMISSIONS
      OWNER_READ OWNER_WRITE
      GROUP_READ
      WORLD_READ )
