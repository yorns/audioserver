/*jslint devel: true */
var webSocket;
var songID = "";
var playlistID = "";
var name_typed;
var broadcastMsg = "";
var useExternalPlayer = true;
var count = 0;

var localDisplayData = {
    "songID": "",
    "playlistID": "",
    "title": "",
    "album": "",
    "performer": "",
    "position": 0,
    "volume": 20,
    "cover": "/img/unknown.png",
    "shuffle": false,
    "loop": false,
    "playing": false
}

$(document).ready(function () {

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
            location.reload();
        }
    });
    
    name_typed = "";
    
    function runWebsocket() {

        if ("WebSocket" in window) {

            webSocket = new WebSocket("ws://" + location.host + "/dynamic");

            webSocket.onerror = function (event) {
                alert("ws error" + event.data);
            };

            webSocket.onopen = function (event) {
                console.log('open websocket connection');
            };

            webSocket.onclose = function (event) {
                console.log('close websocket connection');
            };

            webSocket.onmessage = function (event) {
                try {
                    var msg = JSON.parse(event.data);
                    // TODO: only if song ID is new, request the name
                    if (msg.hasOwnProperty('SongBroadcastMessage')) {
                        if (useExternalPlayer) {
                            showAlbum(msg.SongBroadcastMessage);
                        }
                    }
                    if (msg.hasOwnProperty('SsidMessage')) {
                        //console.log('received: SsidMsg ' + event.data);

                        var trHTML ="";
                        for (var item in msg.SsidMessage) {
                        console.log('item ' + msg.SsidMessage[item] );
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

    runWebsocket();
    
    $("#customSwitch1").attr("checked", useExternalPlayer);
    
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
            //alert("local playback");
            syncPlaylist();
        }
    })
    
    $("#act_playlist").on("click", ".table-row", function() {
        var uid = $(this).attr("id");
        if (useExternalPlayer) {
            url = "/player?select=" + uid;
            $.post(url, "", function(data, textStatus) {}, "json");
        }
        else {
            select_audio(uid);
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

    document.getElementById("progress-box2").oninput = function () {
        setAudioPosition(this.value);
    };

    document.getElementById("volume-box").oninput = function () {
        setVolume(this.value);
    };

    $("#albumSearch").keyup(function() {
        //            if (console && console.log)
        name_typed = $("#albumSearch").val();
        console.log("request: " + name_typed);
        getAlbumList(name_typed);
    });

     setInterval(function() {
         if (!useExternalPlayer) {
             var currentTime = $(".my_audio").prop("currentTime");
             position = $(".my_audio").prop("currentTime") /  $(".my_audio").prop("duration")*100;
             console.log("current time: ", currentTime, " percent: ", position*100);
             localDisplayData.position = position*100;
            showAlbum(localDisplayData);
         }
     }, 500); 

    getActualPlaylist();
    getAlbumList(name_typed);
    $('#albumSearch').val("");
    syncPlaylist();
});

function setSongProgress(position)
{
    console.log("position: ", position/100.0);
    document.getElementById("progress-box2").value = 1.0*position/100.0;  
}

function showAlbum(displayData) {
    //alert("show album", displayData);
    if (displayData.playing) {
        allTitle = displayData.performer + "<br>" 
             + displayData.album + "<br>" 
             + displayData.title;
    }
    else {
        allTitle = displayData.performer + "<br>" 
             + displayData.album         
    }

    $("#actSong").html(allTitle);
    $("#actCover").attr("src", displayData.cover);

    setSongProgress(displayData.position);
    
    document.getElementById("volume-box").value = displayData.volume;

    setLoopButton(displayData.loop);
    setShuffleButton(displayData.shuffle);
    if (displayData.paused) {
        setPauseButton();
    }
    else {
        setPlayButton(displayData.playing);
    }

}

function setAudioPosition(position) {
    url = "/player?toPosition="+position;
    $.post(url, "", function (data, textStatus) {}, "json");    
}

function setVolume(volume) {
    url = "/player?volume="+tvolume;
    $.post(url, "", function (data, textStatus) {}, "json");
}

function setPlayButton(playing) {
   if (playing) {
        $("#playSymbol").removeClass("icon-pause");
        $("#playSymbol").addClass("icon-play");
        $("#btnPlay").removeClass("btn-black");
        $("#btnPlay").addClass("btn-gray");
   } else {
        $("#playSymbol").removeClass("icon-pause");
        $("#playSymbol").addClass("icon-play");
        $("#btnPlay").removeClass("btn-gray");
        $("#btnPlay").addClass("btn-black");
   }

}

function setShuffleButton(shuffle) {
    if (shuffle) {
        $("#btnShuffle").removeClass("btn-black");
        $("#btnShuffle").addClass("btn-gray");
    } 
    else {
        $("#btnShuffle").removeClass("btn-gray");
        $("#btnShuffle").addClass("btn-black");
    } 
}

function setLoopButton(loop) {
    if (loop) {
        $("#btnRepeat").removeClass("btn-black");
        $("#btnRepeat").addClass("btn-gray");
    }
    else {
        $("#btnRepeat").removeClass("btn-gray");
        $("#btnRepeat").addClass("btn-black");
    }
}

function setPauseButton() {
    $("#playSymbol").removeClass("icon-play");
    $("#playSymbol").addClass("icon-pause");
    $("#btnPlay").removeClass("btn-black");
    $("#btnPlay").addClass("btn-gray");   
}

function next() {
    count++;
    if (count >= Object.keys(playlist).length) {
        $("#sound_src").attr("src","");
        play_audio('stop');
        //alert("Playlist ended");
        localDisplayData.playing = false;
        count = 0;
    }
    else {
        $("#sound_src").attr("src", playlist[count])[0];
        $(".my_audio").trigger('load');
        play_audio('play'); 
        getSongInfo(playlist[count]);
    }
}

function getSongInfo(playlistItem) {
    if (playlistItem == "") {
       localDisplayData.performer = "";
       localDisplayData.album = "";
       localDisplayData.cover = "/img/unkown.png";
       localDisplayData.title = "";

    }
        
    console.log("uid: ", uid);
    var regex = /\/audio\/(.*)\.mp3/;
    var matches = regex.exec(playlistItem); // you honestly name that method exec?? 
    var uid = matches[1];
    console.log("uid is ", uid);

   var url = "/database?uid="+uid;
   $.getJSON(url).done(function(response) {
       console.log("uid info: ", response);
       localDisplayData.performer = response[0].performer;
       localDisplayData.album = response[0].album;
       localDisplayData.cover = response[0].cover;
       localDisplayData.title = response[0].title;
       showAlbum(localDisplayData);
       getActualPlaylist();
   });
}
    
function play_audio(task) {
      if(task == 'play'){
          //alert("play started");
          $(".my_audio").trigger('play');
          setPlayButton(true);
          getSongInfo(playlist[count]);
          localDisplayData.playing = true;
      }
      if(task == 'stop'){
          $(".my_audio").trigger('pause');
          $(".my_audio").prop("currentTime",0);
          localDisplayData.playing = false;
          setPlayButton(false);
      }
      if(task == 'next'){
          next();
      }
      if(task == 'pause'){
           $(".my_audio").trigger('pause');
      }
 }

function select_audio(uid) {
    var filename = "/audio/"+uid+".mp3";
    var found = false;
    for(i = 0; i < Object.keys(playlist).length; i++) {
        if (playlist[i] == filename) {
            found = true;
            count = i;
        }
    }
    if (found) {
        $("#sound_src").attr("src", playlist[count])[0];
        $(".my_audio").prop("currentTime",0);
        $(".my_audio").trigger('load');
        getSongInfo(playlist[count]);
        play_audio('play');                      
    } 
    else {
        alert("not found audio file ", uid);
    }
}

/* local playback */

function syncPlaylist() {
    var url = "/playlist?currentPlaylistUID";
  	$.getJSON(url).done(function(response) {
        if (response.current != "") {
            var url = "/pl/"+response.current+".json";
            //alert(url);
            $.getJSON(url).done(function(response) {
               //alert(response);
               playlist = response;
               //alert(playlist[0]);
               $('.my_audio').empty(); 
               $('.my_audio').append("<source id='sound_src' src=" + playlist[count] + " type='audio/mpeg'>");
               $('.my_audio').on('ended', function() { play_audio('next'); });
               $(".my_audio").trigger('load');
               getSongInfo(playlist[count]);
            });
        } 
        else {
            console.log("no playlist set");
        }
	});
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
        play_audio('play');
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
    if (console && console.log) {
        console.log("albumSelect: " + albumId);
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
          count = 0;
          $('#player').modal('show');
            if (!useExternalPlayer) {
              syncPlaylist();
            }
            else {
                console.log("set data for new playlist");
                songID = "";
                playlistID = albumId;
                getActualPlaylist();
            }
            
        }
    });
}

function getAlbumList(searchString) {
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
        trHTML += '<tr class="table-row" id="' + item.uid + '"><td>' + item.title + '</td><td>' + item.performer + '</td><td>' + item.album + '</td></tr>';
    });

    $('#act_playlist tbody').append(trHTML);
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
            getAlbumList(name_typed);
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


