# The AudioServer Introduction

The audioServer is a player for mp3 files and streams that is controlled and configured by a web page (and from your browser)

The underlying technology is a small, but efficient webserver, that serves as 
* resource delivery unit (for the webpage and all connected data) 
* REST API to manage the player, the playlists and files
* websocket API to receive playing information
* upload unit to upload mp3 files to the system

# Image Impressions

<p float="left">
  <img src="https://github.com/yorns/images/blob/master/playlists.jpg" alt="Playlist View" width="200"/>
  <img src="https://github.com/yorns/images/blob/master/player.jpg" alt="Player" width="200"/>
</p>
Image Credits: Pexels 
https://www.pexels.com/de-de/

All accesspoints are discribed here: https://github.com/yorns/audioserver/wiki/API

And below are information, how to setup an audioServer on a local host. There is also a yocto build for a raspberry pi (zero with wifi), to setup an audioServer unit on a standalone zero pi (what is really a nice thing).

So with this little thing, you can play your "old" mp3 files (e.g. created with itunes or downloaded from amazon). If your mp3 files are not filled with mp3 tags, you may need a moment to setup your mp3 files with "title" "album" and "performer" information and a cover image.

have fun!

# find tutorials here:

https://github.com/yorns/audioserver/wiki/Tutorials

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
	"AudioInterface": "gst",
	"EnableCache": true
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
* **EnableCache** enables Cache creation and read on startup
  This is important on slow filesystems (e.g. sd card on raspberry pi). Increases startup time. Id3 Information are not read from MP3 files, but from cache. 

### setup audio data

The mp3 audio files, you want to manage with the audioServer must be placed into the directory **${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioMp3** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

Here you can add arbitrary files and directories. Please take care that all metadata is set.

The mp3 files must conform the id3 version with the following ids set:

* title
* album
* performer
* cover

All these information are important, to setup the library and the cover view.

## setup tags

Tags are a concept not involved in the mp3 metatags. However, huge audio libraries need tagging, to find specific songs or albums. For that reason, it is possible to tag titles, albums and performer in the following way in the file **audioserver/tag/tag.data**.

```<identifier list> | <tag>```
	
The **identifier list** is an AND connected list of names (with the same ability as implemented in the search) to match a specific audio file(s). In that way, you specify a tag to a group of songs (e.g. an album).


## setup and handle caching

When caching is enabled, only unknown files are read to grep id3 meta information. Changes on the files are not recognized. If there is a change in any file ( due to some adjustments e.g. on the meta information), it is recommended to remove all cache files (all files in audioserver/cache/). In that case, the all data is reread and the cache is regenerated with the new information. It is not recommeded, but possible, to change the cache files manually. 

## automatic album creation

Whenever the audioserver starts up, all files are checked for the album name and a temporal playlist is generated by an album. If you want to stop that mechanism, it is only with audio files given in json information. Here you can set the "AlbumCreation" field to "false".

## search mechanism

### album search

The album search method is done the the REST accesspoint /database?albumList=<searchString>

The search string can be multiples of signs, words or halfwords. Seperation is through space. A valid request could be:

```/database?albumList=f%20z```

what is a search for albums with f AND z in its name.

The search is done against performer and album names.

## Special searches

there are some little tweaks for the search, to have more convinient results 

### Empty request

in case the **album list** search field is left blank. The REST call will return the album, that is tagged with "playlist:"

### All request

in case the **album list** search field is set to asterisk, the REST call will return all album title.


### setup stream information (e.g. internet/streaming radio)

All stream information must be placed into the directory *${CMAKE_INSTALL_LOCALSTATEDIR}/audioserver/audioJson** , what is CMAKE_INSTALL_LOCALSTATEDIR is set to /usr/local/var on an host system. raspberry pi yocto environment sets this directory /var/.

A stream is defined in the following form:
```
{
    "Id": "2ffef5e6-3c99-4c14-a32b-4dac9f14d08a",
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
	
## REST API 

can now be found here https://github.com/yorns/audioserver/wiki/API
