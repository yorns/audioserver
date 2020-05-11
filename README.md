# The AudioServer Introduction

The audioServer is a player for mp3 files and streams that is controlled and configured by a web page (and from your browser)

The underlying technology is a small, but efficient webserver, that serves as 
* resource delivery unit (for the webpage and all connected data) 
* REST API to manage the player, the playlists and files
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
        "LogLevel": "debug",
	"AudioInterface": "gst"
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
* **AudioInterface** what audio output interface should be taken 
  * "gst": gstreamer interface (actually better supported and default)
  * "mpv": mpv interface (uses the pipe interface to mpv what needs to run as a second process with pipe interface) 

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

All stream information must be placed into the directory *${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioJson** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

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

### setup m3u playlists

All m3u playlist information must be placed into the directory *${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/playlistM3u** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

The playlist file is very simple. It can be used with any other player as well. It looks like this:
```
../mp3/d90eaf0f-2b7f-49b1-b2aa-7acb2f526aa2.mp3
../mp3/537e54d9-1c15-4819-8b4d-76e0b7ad39d7.mp3
../mp3/8ff9c77a-c704-4866-bcba-a111285f608c.mp3
../mp3/d01d53f1-2389-4c31-b3aa-8204c6429f2f.mp3
../mp3/325c319a-79f6-43fb-84a8-f59fecb5fb73.mp3
../mp3/2bff96d6-1172-4274-b75d-aa40b7acc0ac.mp3
../mp3/5025b416-92f9-4121-9f75-88a1c47da7f7.mp3
```

However, it is not strictly necessary to use the relative path as the audio files will be identified by their unique name.

### setup json playlists

as the m3u playlists are not flexible in some points, there is another format to define playlists in json:

They look something similar to the json file specification for a singel url:
```
{
    "Name": "WDR",
    "Performer" : "Wdr Stream",
    "Extension" : ".png",
    "Items" : [
    { "Id" : "Einslive" },
    { "Id" : "WDR2_Ruhrgebiet" },
    { "Id" : "WDR2_Münsterland" },
    { "Id" : "WDR2_RheinRuhr" },
    { "Id" : "WDR2_Aachen" },
    { "Id" : "WDR2_Ostwestfalen" },
    { "Id" : "WDR2_Bergisches" },
    { "Id" : "WDR2_Rheinland" },
	   { "Id" : "WDR2_Südwestfalen" }
    ] ,
    "Image": "iVBORw0KGgoAAAANSUhEUgAAAyAAAAEECAYAAADQ0lBhAAAABmJL
    []
    4p8AAAAASUVORK5CYII="
 }
```
The **Items** list is just a list of unique identifiers (i.e. names) of mp3s or streams. The cover of this entry is identical as described above. (Image type must be specified by the extension and the image binary data must be place as base64)

# How does it work

This is more internal information, you can skip this paragraph if you do not want to read deep details.

These are the main building blocks:

![Audio Server Construction](https://github.com/yorns/images/blob/master/AudioServer.png?raw=true)


### Reading in all information

There are four sources of information to be read by the audioserver during startup:

* The **mp3** files: All files are read and the id3Tags (title, album, performer and genre) are bound together and are placed into a database. The unique ID is defined by the file name (As there could not be an identical file names within one directory). The cover is read and is not directly placed into a database. A hash value is created and is used to identify if this image is unique or can be represented by another identical cover (Mostly within one album.)
* The **json** files. This files are in Json format and own more or less the same information as given by the id3 tags of the mp3 files. The Url must of course be reachable (but can be available over the internet. The Image Format must be specified by the file extension and must then be given just in a binary format (but converted in base64)
* The m3u playlist files. This files can be read even directly by a player. It does only support a list of files with relative path and a beginning of **# <playlist name>**
[tbd]
	
## REST API (actually not autogenerated)

An audioserver consists of an accesspoint a parameter and a value in this form:

**/\<accesspoint\>?\<parameter\>=\<value\>**

Accesspoints are described below

### database:

### playlist:

### player

The player accesspoint is implemented as REST POST: 

* Start playing a playlist **/player?play=true** 
The playlist must be defined before.
* select a playlist be its unique ID **/player/select=\<uniqueID\>**
* Play next item in playlist **/player?next=true**
* Play previous item in playlist **/player?previous=true**
* Stop playing **/player/stop=true**
* Pause playing **/player/pause=true**
* jumpt to a (percentage) position within the item that is actually played (not working for live streams) **/player?toPosition=\<percentagePosition\>**
* fast forward (20 Sec) **/player/fastForward=true**
* fast backward (20 Sec) **/player/fastBackward=true**
* toogle Shuffle **/player/toggleShuffle=true**
* toggle Looping **/player/toggleLoop=true**
* set volume **/player/volume=<0-100>**

Feedback for the action (e.g. if a toogle was successsfull) is given through the websocket interface. This will keep all states within the Player and the UI does not care for this.


## Web Socket API

The websocket accesspoint is **/dynamic**. Here are messages send for regular updates:
```
{
  "SongBroadcastMessage": {
  "songID" = string   -> unique ID of the actual file (or 0, if no file is actually played)
  "playlistID" = string   -> unique ID of the playlist, that should be presented 
  "curPlaylistID" = string   -> unique ID of the current playlist
  "position" = int   ->  song position in % (0-100)
  "loop" = boolean   ->  true if looping is enabled 
  "shuffle" = boolean   -> true if shuffle is enabled
  "playing" = boolean   -> true if player is playing
  "volume" = int   -> volume value in % (0-100)
  }
}
```

for more information about the song or playlist, the database REST API is the way to go.
