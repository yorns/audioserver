# The AudioServer Introduction

The audioServer is a player for mp3 files and streams that is controlled and configured by a web page (and from your browser)

The underlying technology is a small, but efficient webserver, that serves as 
* resource delivery unit (for the webpage and all connected data) 
* REST API to administrate the player, the playlists and files
* websocket API to receive playing information
* upload unit to upload mp3 files to the system

All accesspoints are discribed below.

And below are information, how to setup an audioServer on a local host. There is also a yocto build for a raspberry pi (zero with wifi), to setup an audioServer unit on a standalone zero pi (what is really a nice thing).

So with this little thing, you can play your "old" mp3 files (e.g. created with itunes or downloaded from amazon). If your mp3 files are not filled with mp3 tags, you may need a moment to setup your mp3 files with "title" "album" and "performer" information and a cover image.

have fun!

# audioSever build and installation
creating is quite easy, as usally do (in the git directory)
```
mkdir build
cd build
cmake ..
make
<eventually>
sudo make install
```

for the build process you need (*actually the packets mpv, tagLib, boost (at least 1.70), gstreamer*) 

#### In case you use the installation
this will (hopefully in futur) 

 - add the binary audioServer to your binary path
 - creates several directories in your `share` location (usually `/usr/share/audioserver` and some below)
 - copies the html and javascript data to that places
 - modifies directory rights  

### calling the server
easy as this:
```
audioServer [config_file]
```

if no config file is given. The server tries to open the one at /etc/audioserver.json

**Example:**
```
audioServer ${HOME}/audioConfig/audioserver.json
```

### setup audio data

all mp3 files must be placed into the directory **${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioMp3** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

To identify the files, the must have a unique name. The name itself is not important for the later handling.

The mp3 files must conform the id3v2 version with the following ids set:

* title
* album
* performer
* cover

All these information are important, to setup the library and the (first) cover view.

### setup stream data

all stream information must be placed into the directory *${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioJson** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

A stream is defined in the following form:
```
{
    "Title": "Einslive",
    "Album": "WDR Stream",
    "Performer": "Einslive",
    "Extension": ".png",
    "AlbumCreation": false,
    "TrackNo": 0,
    "AllTrackNo": 24,
    "Url": "http://wdr-1live-live.icecast.wdr.de/wdr/1live/live/mp3/128/stream.mp3",
    "Image": "iVBORw0KGgoAAAANSUhEUgAAAK8AAACvCAYAAACLko51AAAABGdBTUEAALGPC/xhBQAAAA
    []
    AAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAABJRU5ErkJggg=="
}
```
Most information are understandable. The image is set from png or jpeg from the specified in the **Extension** information and must be given in base64 format. 

So if you want to add an image here, use an online image base 64 converter and just put the output here. It is that easy.

