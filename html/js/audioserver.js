/*jslint devel: true */
var webSocket;
var songID = "";
var playlistID = "";
var name_typed = "";
var broadcastMsg = "";
var useExternalPlayer = true;
var websocketConnected = false;
var websocketError = false;
//var count = 0;

var noSleep = {};
var tmpPlaylist = {};
var browserPlaylist = {};

function LocalStoreData(l_name_typed, l_useExternalPlayer, l_count, l_volume) {
    this.name_typed = l_name_typed;
    this.useExternalPlayer= l_useExternalPlayer;
    this.count = l_count;
    this.volume = l_volume;
}

var localDisplayData = {
    songID: "00000000-0000-0000-0000-000000000000",
    playlistID: "00000000-0000-0000-0000-000000000000",
    title: "",
    album: "",
    performer: "",
    position: 0,
    duration: 0,
    volume: 0,
    cover: "/img/unknown.png",
    shuffle: false,
    loop: false,
    playing: false,
    paused: false,
    count: 0,
    single: false,
    shuffle_list: [],
    
    hasPlaylistChanged : function(playlistID) {
        if (this.playlistID != playlistID) { 
            console.log("playlist changed: old ", this.playlistID, " != new -> ", playlistID);
        }
        return (this.playlistID != playlistID ); 
    },    
        
    loadTitle : function(uid, showTitleFunction) {
        console.log("uid is ", uid);
        var url = "/database?uid="+uid;
        $.getJSON(url).done(function(response) {
            try {
                titleInfoList = response[0];
                console.log("uid info: ", titleInfoList);
                this.performer = titleInfoList.performer;
                this.album = titleInfoList.album;
                this.cover = titleInfoList.cover;
                this.title = titleInfoList.title;
                this.songID = titleInfoList.uid;
                showTitleFunction(titleInfoList);
            } catch (e) {
                console.log("failed to parse title json: ", response);
            }
       });
    },
    
    loadPlaylist : function(uid, showPlaylistFunction) {
        let url = "/playlist?show="+uid;
        console.log("request for show actual playlist: ", url);
        $.getJSON(url).done(function(jsonResponse) {
            console.log("playlist reply: ", jsonResponse);
            if (jsonResponse != null) {
                var playlist = jsonResponse;
                setPlaylist(playlist);
                showPlaylistFunction(getPlaylist());
            }
            else {
                unshowPlaylist();
                unshowTitle();
                tmpDisplayData.cover = "/img/unknown.png";
            }
        });
    },
        
    hasPlayingChanged : function(jsonBroadcastMessage) {
        if (this.playing != jsonBroadcastMessage.playing) {
            console.log("playing changed");
        }
        return this.playing != jsonBroadcastMessage.playing;
    },

    hasAlbumChanged : function(jsonBroadcastMessage) {
        if (this.album != jsonBroadcastMessage.album) {
            console.log("album changed");
        }
        return this.album != jsonBroadcastMessage.album;
    },

    hasSongChanged : function(jsonBroadcastMessage) {
        if (this.songID != jsonBroadcastMessage.songID) {
            console.log("title changed from <", this.songID , "> to <", jsonBroadcastMessage.songID, ">" );
        }
        return this.songID != jsonBroadcastMessage.songID;
    },
    
    setLocalDisplayData : function(jsonBroadcastMessage, isBrowserCall) {
        // console.log("---------- store message information in localDisplayData pl: ", jsonBroadcastMessage.playlistID)
        this.title = jsonBroadcastMessage.title;
        this.album = jsonBroadcastMessage.album;
        this.performer = jsonBroadcastMessage.performer;
        this.songID = jsonBroadcastMessage.songID;
        this.playlistID = jsonBroadcastMessage.playlistID;
        this.position = jsonBroadcastMessage.position;
        this.volume = jsonBroadcastMessage.volume;
        this.cover = jsonBroadcastMessage.cover;
        this.shuffle = jsonBroadcastMessage.shuffle;
        this.loop = jsonBroadcastMessage.loop;
        this.playing = jsonBroadcastMessage.playing;
        this.paused = jsonBroadcastMessage.paused;
        this.single = jsonBroadcastMessage.single;
        this.shuffle_list = jsonBroadcastMessage.shuffle_list;
        
        if (jsonBroadcastMessage.duration) {
            this.duration = jsonBroadcastMessage.duration;
        }
        
        //if (jsonBroadcastMessage.count) {
            this.count = jsonBroadcastMessage.count;
        //}

        if (!useExternalPlayer && isBrowserCall) {
            tmpDisplayData = { ...this };
        }   
    }

}

var tmpDisplayData = { ...localDisplayData };

function unshowPlaylist() {
    $('#act_playlist tbody').empty();
    var trHTML ="";
    $('#act_playlist tbody').append(trHTML);
}

function showPlaylist(playlist) {
    console.log("reply for show playlist table");
    $('#act_playlist tbody').empty();
    var trHTML ="";
    $.each(playlist, function(i, item) {
        trHTML += '<tr class="table-row" id="' + item.uid + '"><td>' + item.title 
            + '</td><td>' + item.performer + '</td><td>' + item.album + '</td></tr>';
    });
    $('#act_playlist tbody').append(trHTML);
}

function unshowTitle(titleInfo) {
    let allTitle = "";
    
    if (titleInfo == null) {
        $("#actCover").removeAttr("src");
    }
    else { 
        allTitle = titleInfo.performer + "<br>" 
             + titleInfo.album;
        if ($("#actCover").attr("src") != titleInfo.cover) {
            console.log("set cover");
            $("#actCover").attr("src", titleInfo.cover);
        }
    }
    
    $("#actSong").html(allTitle);
    
}

function showTitle(titleInfo) {
    let allTitle = "";
    
    allTitle = titleInfo.performer + "<br>" 
             + titleInfo.album + "<br>" 
             + titleInfo.title;

    $("#actSong").html(allTitle);
    if ($("#actCover").attr("src") != titleInfo.cover) {
        console.log("set cover to "+titleInfo.cover);
        $("#actCover").attr("src", titleInfo.cover);
    }

}

function showVolume(volume) {
    document.getElementById("volume-box").value = volume;    
}

function showPlayerData(displayData) {
    
    showSongProgress(displayData.position);    
    showVolume(displayData.volume);

    showLoopButton(displayData.loop);
    showShuffelButton(displayData.shuffle);
    showSingleButton(displayData.single);

    showPlayButton(displayData.playing, displayData.paused);
    
}

function intervalHandler() {
    // request playlist ID
    
}

function handleShuffle() {
    var len = Object.keys(browserPlaylist).length;
    tmpDisplayData.shuffle_list = Array.from(Array(len).keys());
    if (tmpDisplayData.shuffle) {
        console.log(">>>> shuffling");
        tmpDisplayData.shuffle_list = tmpDisplayData.shuffle_list.sort(() => Math.random() - 0.5);
    }
    console.log("shuffle list is: ", tmpDisplayData.shuffle_list);
}

function setPlaylist(playlist) {
    if (useExternalPlayer) {
        tmpPlaylist = playlist;
    }
    else {
        browserPlaylist = playlist;
    }
}

function getPlaylist() {
    if (useExternalPlayer)
        return tmpPlaylist;
    return browserPlaylist;
}

function onPlayerMessage(msg, isBrowserCall) {

    var playlistID = msg.playlistID;
    var playing = msg.playing;
    var songID = msg.songID;
    
    var titleInfo = {
        title     : msg.title,
        performer : msg.performer,
        album     : msg.album,
        cover     : msg.cover
    };

    if (!playing && localDisplayData.playing) {
        titleInfo.title = "";
        unemphTableEntry(localDisplayData.songID);
        console.log("not playing - remove last entry ", localDisplayData.songID);
    }
    
    if (localDisplayData.hasPlaylistChanged(playlistID)) {
       console.log("playlist has changed - update (", playlistID, ")");
       if (playlistID != "00000000-0000-0000-0000-000000000000") {
           localDisplayData.loadPlaylist(playlistID, function(playlist) {
		 console.log("playlist loaded");

		 setPlaylist(playlist);
		 showPlaylist(playlist);
		 unshowTitle(titleInfo);
		 unemphTableEntry(localDisplayData.songID);
		 handleShuffle();
		 if (playing) {
		     showTitle(titleInfo);
		     emphTableEntry(msg.songID);
		 }
		else {
		    console.log("system is silent");
		}
		localDisplayData.setLocalDisplayData(msg, isBrowserCall);
		});
        }
        else {
            unshowPlaylist();
            unshowTitle();
            localDisplayData.setLocalDisplayData(msg, isBrowserCall);
            showTitle();
        }
    }
    else {
        if (localDisplayData.hasSongChanged(msg)) {
            console.log("song changed");
            unemphTableEntry(localDisplayData.songID);
            if (playing) {
                emphTableEntry(msg.songID);
console.log("cover set to "+msg.cover);
            }
         }
         else {
             if (!playing)
                console.log("system is silent");
             else
                 console.log("system is silent - playing");
         }
//            unshowTitle();
//            localDisplayData.cover = "/img/unknown.png";
console.log("again cover: "+msg.cover);
        localDisplayData.setLocalDisplayData(msg, isBrowserCall);        
        showTitle(titleInfo);
//        alert("image 2");
    }

    showPlayerData(localDisplayData);
}

function runWebsocket() {

    if ("WebSocket" in window) {

        webSocket = new WebSocket("ws://" + location.host + "/dynamic");

        webSocket.onerror = function (event) {
            // alert("ws error " + event.data);
            websocketError = true;
            webSocket.close();
        };

        webSocket.onopen = function (event) {
            websocketConnected = true;
            console.log('open websocket connection');
        };

        webSocket.onclose = function (event) {
            websocketConnected = false;
            // alert("ws close " + event.data);
        };

        webSocket.onmessage = function (event) {
            //console.log(event.data);
            try {
                var msg = JSON.parse(event.data);
                if (msg.hasOwnProperty('SongBroadcastMessage')) {
                    if (useExternalPlayer) {
                        onPlayerMessage(msg.SongBroadcastMessage, false);                                
                    }
                    // ignore message, when playing locally
                }
                if (msg.hasOwnProperty('SsidMessage')) {
                    //console.log('received: SsidMsg ' + event.data);

                    var trHTML ="";
                    for (var item in msg.SsidMessage) {
                    // console.log('item ' + msg.SsidMessage[item] );
                        trHTML += '<tr class="table-row" id="'  + msg.SsidMessage[item] + '"><td></td><td>' + msg.SsidMessage[item] + '</td><td></td></tr>';
                    }
                    $('#wifiList tbody').empty();
                    $('#wifiList tbody').append(trHTML);
                }

                //console.log('received: loop: ' + msg.SongBroadcastMessage.loop + ' shuffle: ' + msg.SongBroadcastMessage.shuffle );
            } catch (e) {
                console.log("json parse failed");
            }

        };
    } else {

        // The browser doesn't support WebSocket
        alert("WebSocket NOT supported by your Browser!");
    }

}

$(document).ready(function () {
    
    noSleep = new NoSleep();
    noSleep.enable();
    
    $('#player').on('show.bs.modal', function(e) {
        playlistID="";
        window.location.hash = "modal";
    });
    $('#upload').on('show.bs.modal', function(e) {
        window.location.hash = "modal";
    });
    $('#wifi').on('show.bs.modal', function(e) {
        window.location.hash = "modal";
    });

    $(window).on('hashchange', function (event) {
        if(window.location.hash != "#modal") {
            $('#player').modal('hide');
            $('#upload').modal('hide');
            $('#wifi').modal('hide');
            location.reload(true);
        }
    });

    runWebsocket();
    
    $("#wifiCredentials").click(function() {
        url = "wifi";
        url += "?ssid=" + encodeURIComponent($("#credential_ssid").val());
        url += "&psk=" + encodeURIComponent($("#credential_psk").val());
        //alert("wifi url: " + url);
        $.post(url, "", function(data, textStatus) {}, "json");
    })
    
    $("#wifiList").on("click", ".table-row", function() {
        $("#credential_ssid").val($(this).attr("id"));
        //alert("wifi ssid: " + $(this).attr("id"));
    });

    
    $("#customSwitch1").on('change', function() {
        if ($(this).is(':checked')) {
            useExternalPlayer = $(this).is(':checked');
        }
        else {
           useExternalPlayer = $(this).is(':checked');
//            console.log("playlist ID is ", localDisplayData.playlistID);
            onPlayerMessage(tmpDisplayData,true);
        }
        console.log("changed externalPlayer to ", useExternalPlayer);
        init();
    })
    
    $("#act_playlist").on("click", ".table-row", function() {
        var uid = $(this).attr("id");
        if (useExternalPlayer) {
            url = "/player?select=" + uid;
            $.post(url, "", function(data, textStatus) {}, "json");
        }
        else {
            console.log("playing webplayer uid: ", uid);
            songSelectBrowser(uid);
        }
    });

    $("#createPlaylist").click(function() {
        var newPlaylistName = $("#newPlaylistName").val();
        if (newPlaylistName === "") {
            alert("no playlist set");
        }
        $.get("/playlist?create=" + newPlaylistName, function() {})
            .always(function() {
                alert("Playlist <" + newPlaylistName + "> created");
            });
    });

//    $('.my_audio').on('ended', function() { play_audio('next'); });
    
    $("#btnPlay").click(function() {
        startPlay();
    });

    $("#btnNext").click(function() {
        nextTitle();
    });

    $("#btnPrevious").click(function() {
        previousTitle();
    });

    $("#btnStop").click(function() {
        stopPlayer();
    });

    $("#btnPause").click(function() {
        pausePlayer();
    });

    $("#btnRepeat").click(function() {
        toogleLoopPlayer();
    });

    $("#btnShuffle").click(function() {
        toogleRandomPlayer();
    });

    $("#btnFastForward").click(function() {
        fastForwardPlayer();
    });

    $("#btnFastBackward").click(function() {
        fastBackwardPlayer();
    });
    
    $("#btnSingle").click(function() {
        toggleSingle();
    })

    document.getElementById("progress-box").oninput = function () {
        setPosition(this.value);
    };

    document.getElementById("volume-box").oninput = function () {
        setVolume(this.value);
    };

    $("#albumSearch").keyup(function() {
        var newSearchString = $("#albumSearch").val();
        if (newSearchString != name_typed) {
            name_typed = newSearchString;
            console.log("request: " + name_typed);
            showAlbumList(name_typed);
        }
    });

    
     setInterval(function() {

         if (!useExternalPlayer) {
                        
             if (tmpDisplayData.playing) {
                 var currentTime = $(".my_audio").prop("currentTime");
                 var duration = $(".my_audio").prop("duration");
                 tmpDisplayData.position = (currentTime * 100*100) / duration;
                 tmpDisplayData.duration = duration;
             }
             else {
                 tmpDisplayData.position = 0;
                 tmpDisplayData.duration = 0;
             }
             onPlayerMessage(tmpDisplayData, true);
             
         }
         storePersistentData();
     }, 500); 

    restorePersistentData();
//    setVolume(15); // init default value
    init();
});

function resetLocalAudioTag()
{
    $('.my_audio').empty(); 
    $('.my_audio').append("<source id='sound_src' src='' title='' poster='img/unknown.png' type='audio/mpeg'>");
    $('.my_audio').off('ended');
    $('.my_audio').on('ended', function() { 
        console.log("ended trigger received"); 
        play_audio('next'); 
    });            
}

function init()
{
    $("#customSwitch1").attr("checked", useExternalPlayer);
    $('#albumSearch').val(name_typed);
    showAlbumList(name_typed);
    
    if (!useExternalPlayer) {
        // reset all displayed information and display them from local playback
        resetLocalAudioTag();
        localDisplayData.setLocalDisplayData(tmpDisplayData);
    }
    
    
}

function storePersistentData() {
    let volume = tmpDisplayData.volume;
    var localStoreData = new LocalStoreData(name_typed, useExternalPlayer, 0, );
    sessionStorage.setItem('localStoreData', JSON.stringify(localStoreData));
}

function restorePersistentData() {
    var localStoreData = JSON.parse(sessionStorage.getItem('localStoreData'));
    if (localStoreData) {
        useExternalPlayer = localStoreData.useExternalPlayer;
        name_typed = localStoreData.name_typed;
        localDisplayData.volume = localStoreData.volume;
    }
    else {
        console.log("cannot restore local data");
    }
}

function showSongProgress(position)
{
    document.getElementById("progress-box").value = 1.0*position/100.0;  
}

function setVolume(_volume) {
    console.log("set volume: ", _volume);    
    if (!useExternalPlayer) {
        if (tmpDisplayData.volume != _volume) {
            tmpDisplayData.volume =_volume;
            $(".my_audio").prop("volume",_volume/100);
            console.log("browser");
        }
    }
    else {
        console.log("external player");
        url = "/player?volume="+_volume;
        $.post(url, "", function (data, textStatus) {}, "json");
    }
}

function setPosition(_position) {
    if (!useExternalPlayer) {
        console.log("new position set to: ", _position, "% of ", tmpDisplayData.duration);
        if (tmpDisplayData.duration) {            
            $(".my_audio").prop("currentTime",_position*tmpDisplayData.duration/100);
            // console.log("position: ", _position*tmpDisplayData.duration/100);
        }
    }
    else {
    url = "/player?toPosition="+_position;
    $.post(url, "", function (data, textStatus) {}, "json");    
    }
}


function showPlayButton(playing, paused) {
   if (playing) {
       if (paused) {
            $("#playSymbol").removeClass("icon-play");
            $("#playSymbol").addClass("icon-pause");
            $("#btnPlay").removeClass("btn-black");
            $("#btnPlay").addClass("btn-gray");   
       }
       else {
            $("#playSymbol").removeClass("icon-pause");
            $("#playSymbol").addClass("icon-play");
            $("#btnPlay").removeClass("btn-black");
            $("#btnPlay").addClass("btn-gray");
       }
   } else {
        $("#playSymbol").removeClass("icon-pause");
        $("#playSymbol").addClass("icon-play");
        $("#btnPlay").removeClass("btn-gray");
        $("#btnPlay").addClass("btn-black");
   }

}

function showShuffelButton(shuffle) {
    if (shuffle) {
        $("#btnShuffle").removeClass("btn-black");
        $("#btnShuffle").addClass("btn-gray");
    } 
    else {
        $("#btnShuffle").removeClass("btn-gray");
        $("#btnShuffle").addClass("btn-black");
    } 
}

function showLoopButton(loop) {
    if (loop) {
        $("#btnRepeat").removeClass("btn-black");
        $("#btnRepeat").addClass("btn-gray");
    }
    else {
        $("#btnRepeat").removeClass("btn-gray");
        $("#btnRepeat").addClass("btn-black");
    }
}

function showSingleButton(single) {
    if (single) {
        $("#btnSingle").removeClass("btn-black");
        $("#btnSingle").addClass("btn-gray");
    }
    else {
        $("#btnSingle").removeClass("btn-gray");
        $("#btnSingle").addClass("btn-black");
    }
}

function unemphTableEntry(songID) {
    if (!songID || songID == "00000000-0000-0000-0000-000000000000") {
        $("#act_playlist").attr("style", "");          
    }
    else  {
        if (songID) {
            console.log("emphisis remove <", songID, ">");
            $("#act_playlist #"+songID).attr("style", "");  
        }
    }
}

function emphTableEntry(songID) {   //(oldUid, newUid) {
    if (songID && songID != "00000000-0000-0000-0000-000000000000") {
            console.log("emphisis add to <", songID, ">");
            $("#act_playlist #"+songID).attr("style", "background-color:gray");
    }
}

function getPlaylistCover(albumId) {
    console.log("get playlist cover <", albumId, ">" );
    if (tmpDisplayData.playing)
        return;
    
    getPlaylistUid(albumId);
}

function next() {
    if (!tmpDisplayData.playing)
        return;
    
    if (tmpDisplayData.single) {
        console.log("playing only single file - stopping");
        $("#sound_src").attr("src","");
        play_audio('stop');
        tmpDisplayData.count = 0;
    }
    else {
        tmpDisplayData.count++;
        var length = Object.keys(browserPlaylist).length;
        let id = tmpDisplayData.shuffle_list[tmpDisplayData.count];
        console.log("play next: ", tmpDisplayData.count, " - ", id, " / ", length);
        if (tmpDisplayData.count >= length) {
            console.log("end found");
            play_audio('stop');
        }
        else {
            play_audio('play'); 
        }
    }
}

function play_audio(task) {
      if(task == 'play'){         
          var length = Object.keys(browserPlaylist).length;
          
          if (tmpDisplayData.count >= length) {
            alert("chosen id is out of range");
          }
          //handleShuffle();        
          let id = tmpDisplayData.shuffle_list[tmpDisplayData.count];
          
          tmpDisplayData.songID = browserPlaylist[id];
          // set new title/album etc
          let audioUrl = "/audio/" + browserPlaylist[id].uid; 
          console.log("go on playing: ", browserPlaylist[id].title, " ", 
                            browserPlaylist[id].album, " " + 
                            browserPlaylist[id].performer);
          $("#sound_src").attr("src", audioUrl);
          $("#sound_src").attr("title", browserPlaylist[id].title);              
          $(".my_audio").prop("currentTime",0);
          $(".my_audio").trigger('load');
          $("#sound_src").attr("autoplay", true);       
          $(".my_audio").trigger('play');
          tmpDisplayData.playing = true;
          tmpDisplayData.paused = false;

          if (browserPlaylist) {
              tmpDisplayData.songID = browserPlaylist[id].uid;
              tmpDisplayData.title = browserPlaylist[id].title;
              tmpDisplayData.album = browserPlaylist[id].album;
              tmpDisplayData.performer = browserPlaylist[id].performer;
              tmpDisplayData.cover = browserPlaylist[id].cover;
          }
      }
      if(task == 'stop'){
          $(".my_audio").trigger('pause');
        tmpDisplayData.position = 0;
        tmpDisplayData.title = "";
        tmpDisplayData.songID = "00000000-0000-0000-0000-000000000000";
        tmpDisplayData.playing = false;
        tmpDisplayData.paused = false;
        tmpDisplayData.count = 0;
        getPlaylistCover(tmpDisplayData.playlistID);
      }
      if(task == 'next'){
          next();
      }
      if(task == 'pause'){
          if (tmpDisplayData.playing && !tmpDisplayData.paused) {
              $(".my_audio").trigger('pause');
              tmpDisplayData.paused = true;
          }
      }
      if(task == 'unpause'){
          if (tmpDisplayData.playing && tmpDisplayData.paused) {
              $(".my_audio").trigger('play');
              tmpDisplayData.paused = false;
          }
      }
      if(task == 'toggleSingle') {
          console.log("toggle playing single song from ", tmpDisplayData.single);          
          tmpDisplayData.single = !tmpDisplayData.single;
          console.log("toggle playing single song to ", tmpDisplayData.single);          
      }
      if(task == 'random') {
          if (tmpDisplayData.playing && tmpDisplayData.shuffle) {
              tmpDisplayData.count = tmpDisplayData.shuffle_list[tmpDisplayData.count] 
          }
          if (tmpDisplayData.playing && !tmpDisplayData.shuffle) {
              tmpDisplayData.count = 0;
          }
          tmpDisplayData.shuffle = !tmpDisplayData.shuffle;
          handleShuffle();
          
      }
 }

function songSelectBrowser(uid) {
    var found = false;
    for(i = 0; i < Object.keys(browserPlaylist).length; i++) {
        let id = localDisplayData.shuffle_list[i];
        if (browserPlaylist[id].uid == uid) {
            found = true;
            tmpDisplayData.count = i;     
        }
    }
    if (found) {
        let audioUrl = "/audio/"+uid;
        console.log("play selected audio playlist item <",audioUrl,">");
        tmpDisplayData.songID = uid;
        resetLocalAudioTag();
        play_audio('play');
    } 
    else {
        alert("not found audio file ", uid);
    }
}

function previousTitle() {
    if (useExternalPlayer) {
        url = "/player?prev=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('previous');
    }
}

function nextTitle() {
    if (useExternalPlayer) {
        url = "/player?next=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    } 
    else {
        play_audio('next');
    }
}


function stopPlayer() {
    if (useExternalPlayer) {
        url = "/player?stop=true";
        $.post(url, "", function (data, textStatus) {}, "json");
    }
    else {
        play_audio('stop'); 
    }
}

function startPlay() {
    if (useExternalPlayer) {
        url = "/player?play=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        console.log("playing: ", tmpDisplayData.playing, " paused: ", tmpDisplayData.paused)
        if (!tmpDisplayData.playing) {
            tmpDisplayData.count = 0;
            resetLocalAudioTag();
            play_audio('play');
        }
        else {
            if (!tmpDisplayData.paused) {
                play_audio('pause');
            }
            else {
                play_audio('unpause');
            }            
        }
    }
}

function pausePlayer() {
    if (useExternalPlayer) {
        url = "/player?pause=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    } 
    else {
        play_audio('pause');
    }
}

function toogleLoopPlayer() {
    if (useExternalPlayer) {
        url = "/player?toggleLoop=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('loop');
    }
}

function toggleSingle() {
    if (useExternalPlayer) {
        url = "/player?toggleSingle=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('toggleSingle');
    }    
}

function toogleRandomPlayer() {
    if (useExternalPlayer) {
        url = "/player?toggleShuffle=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('random');
    }
}

function fastForwardPlayer() {
    if (useExternalPlayer) {
        url = "/player?fastForward=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('fastForwamrd');
    }
}

function fastBackwardPlayer() {
    if (useExternalPlayer) {
        url = "/player?fastBackward=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    }
    else {
        play_audio('fastBackward');
    }
}

function albumSelect(albumId) {
    
    if (useExternalPlayer) {
        if (console && console.log) {
            console.log("external play album select: " + albumId);
        }

        // open overlay / new page
        // show playlist with all album titles
        var url = "/playlist?change=" + encodeURIComponent(albumId);
        if (console && console.log)
            console.log("request: " + url);
        $.getJSON(url).done(function(response) {
            if (response.result != "ok") {
                alert(response.result);
            } else {
              localDisplayData.count = 0; //   tmpDisplayData.count = 0;
              $('#player').modal('show');
              //localDisplayData.playlistID = albumId; //tmpDisplayData.playlistID = albumId;
            }
        });
    }
    else {
        if (console && console.log) {
            console.log("webbrowser play album select: " + albumId);
            // ensure, shuffle list is set
            handleShuffle();
        }
        
        $('#player').modal('show');
        getPlaylistUid(albumId);

    }
}

function showAlbumList(searchString) {
    if (searchString == "") {
        searchString = "Playlist:";
    }
    if (searchString == "*") {
        searchString = "";
    }
        
    var url = "/database?albumList=" + encodeURIComponent(searchString);
    $.getJSON(url).done(function(response) {
        $('#cover').empty();
        var trHTML = '<div class="container-fluid"> <div class="row mt-5 justify-content-center" id="myimg">';
        if (response) {
            response.sort(function(a,b){
                if (a.performer == b.performer) {
                  if (a.album == b.album)    
                    return 0;
                if (a.album < b.album) 
                    return -1;
                if (a.album > b.album)
                    return 1;                

                }
                if (a.performer < b.performer) 
                    return -1;
                if (a.performer > b.performer)
                    return 1;
            })

            $.each(response, function(i, item) {
                trHTML += `
                            <div class='card bg-black-1 border-3 mx-sm-2 mb-sm-3' onclick='albumSelect("${item.uid}")' > 
                            <div class="text-center">
                            <img class="card-img" style="max-width: 96%; padding-top: 2%" src="${item.cover}" alt="${item.album}" >
                            </div>
                            <div class="card-body" >
                              <h5 class="card-title">${item.album}</h5>
                              <p class="card-text">${item.performer}</p>
                            </div>
                            </div>`
            });
        }
        trHTML += '</div>';
        $('#cover').append(trHTML);
    })
}

function getActualPlaylist() {
    url = "/playlist?show";
    console.log("request for show actual playlist: ", url);
    $.getJSON(url).done(function(response) {
        console.log("reply for show actual playlist: ", response);
        $('#act_playlist tbody').empty();
        var trHTML ="";
        $.each(response, function(i, item) {
            trHTML += '<tr class="table-row" id="' + item.uid + '"><td>' + item.title 
                + '</td><td>' + item.performer + '</td><td>' + item.album + '</td></tr>';
        });
        $('#act_playlist tbody').append(trHTML);
    });

}

function getPlaylistUid(uid) {
    url = "/playlist?albumUid="+uid;
    $.getJSON(url).done(function(response) {
        // test if there are any entries at all ... if (resonse)
        console.log("reply for get playlist (", uid, "): ", response);
        tmpDisplayData.album = response[0].album;
        tmpDisplayData.performer = response[0].performer;
        tmpDisplayData.cover = response[0].cover;
        tmpDisplayData.playlistID = uid;
    });
}

function fileChange() {
    //FileList Objekt aus dem Input Element mit der ID "fileA"
    var fileList = document.getElementById("fileA").files;

    $('#filelist').empty();

    var fileListHTML = '<ul class="list-group">';

    $.each(fileList, function(i, item) {
        fileListHTML += `<li class="list-group-item">${item.name}</li>`;
    });
    fileListHTML += '</ul>';

    $('#filelist').append(fileListHTML);

    $('.progress-bar').css("width", "0%");
}

var client = null;

function uploadFile() {
    //Wieder unser File Objekt
    var fileList = document.getElementById("fileA").files;

    $.each(fileList, function(i, file) {

        //create XMLHttpRequest
        client = new XMLHttpRequest();

        if (!file)
            return;

        client.onerror = function(e) {
            alert("upload error");
        };

        client.onload = function(e) {
            $('.progress-bar').css("width", "100%");
            showAlbumList(name_typed);
            // mark transfered file as copied
        };

        client.upload.onprogress = function(e) {
            var p = Math.round(100 / e.total * e.loaded);
            $('.progress-bar').css("width", p + "%");
        };

        client.onabort = function(e) {
            alert("Upload aborted");
        };

        client.open("POST", "/upload");
        client.send(file);
    });
}

function uploadAbort() {
    if (client instanceof XMLHttpRequest)
        client.abort();
}


