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



