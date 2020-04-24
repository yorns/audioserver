# The AudioServer

The audioServer is a player for mp3 files and streams that is controlled and configured by a web page (and from your browser)

The underlying technology is a small, but efficient webserver, that serves as 
* resource delivery unit (for the webpage and all connected data) 
* REST API to control the player
* websocket API to receive playing information
* upload unit to send mp3 files to the system

below are information, how to setup an audioServer on a local host. There is also a yocto build for a raspberry pi (zero with wifi), to setup an audioServer unit on a standalone zero pi (what is really a nice thing).

So with this little thing, you can play your "old" mp3 files (e.g. created with itunes or downloaded from amazon). If your mp3 files are not filled with mp3 tags, you may need a moment to setup your mp3 files with "title" "album" and "performer" information and a cover image.

have fun!

# audioSever
creating is quite easy, as usally do (in the git directory)
```
mkdir build
cd build
cmake ..
make
<eventually>
sudo make install
```

#### In case you use the installation
this will (hopefully in futur) 

 - add the binary audioServer to your binary path
 - creates several directories in your `share` location (usually `/usr/share/audioserver` and some below)
 - copies the html and javascript data to that places
 - modifies directory rights  

### calling the server
easy as this:
```
audioServer <ip address> <port> <path to the share root>
```
**Example:**
```
audioServer 0.0.0.0 80 /usr/share/audioserver
```



