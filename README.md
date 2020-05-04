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

# audioServer setup

### configuration file

```
{
        "IpAddress": "0.0.0.0",
        "Port": "8080",
        "BasePath": "/usr/local/var/audioserver",
        "LogLevel": "debug"

}
```

Within this file, you can configure the following information:

* **IpaAddress** (or better, interface) the server should listen to.
* **Port** to listen. The connection is not secure and therefor http, so using 443 does not provide any security (the audioserver should only be used in secure home nets, not on the internet)
* **Base** path to find all relevant information for the webpage or the audio information. This must fit your installation path, defined by the system (**${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/**)
* **LogLevel** what information should be logged to e.g. systemd logging
  * debug
  * info
  * warning
  * error

### setup audio data

The mp3 audio files, you want to manage with the audioServer must be placed into the directory **${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioMp3** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

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

If you do not what the auto Album Creator be used on this file, set **AlbumCreation** to false. In that case, you need to create your own playlist.

# How does it work

This is more internal information, you can skip this paragraph if you do not want to read deep details.

### Reading in all information

There are four sources of information to be read by the audioserver during startup:

* The **mp3** files: All files are read and the id3Tags (title, album, performer and genre) are bound together and are placed into a database. The unique ID is defined by the file name (As there could not be an identical file names within one directory). The cover is read and is not directly placed into a database. A hash value is created and is used to identify if this image is unique or can be represented by another identical cover (Mostly within one album.)
* The **json** files. This files are in Json format and own more or less the same information as given by the id3 tags of the mp3 files. The Url must of course be reachable (but can be available over the internet. The Image Format must be specified by the file extension and must then be given just in a binary format (but converted in base64)
* The m3u playlist files. This files can be read even directly by a player. It does only support a list of files with relative path and a beginning of **# <playlist name>**
 
