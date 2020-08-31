/*jslint devel: true */
var webSocket;
var songID = "";
var playlistID = "";
var name_typed;
var interval = 3000; // Interval for calling the function in milliseconds (seconds * 1000)
var safety = 500; // This is the allowed divergence to the interval
var broadcastMsg = "";

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

                        if (broadcastMsg.playlistID != msg.SongBroadcastMessage.playlistID) {
                            getActualPlaylist();
                        } 
                        
                        broadcastMsg = msg.SongBroadcastMessage;

                        allTitle = msg.SongBroadcastMessage.performer + "<br>" 
                                 + msg.SongBroadcastMessage.album + "<br>" 
                                 + msg.SongBroadcastMessage.title;

                        $("#actSong").html(allTitle);
                        $("#actCover").attr("src", msg.SongBroadcastMessage.cover);

                        document.getElementById("progress-box2").value = msg.SongBroadcastMessage.position/100;

                        document.getElementById("volume-box").value = msg.SongBroadcastMessage.volume;

                        if (msg.SongBroadcastMessage.loop) {
                            $("#btnRepeat").removeClass("btn-black");
                            $("#btnRepeat").addClass("btn-gray");
                        }
                        else {
                            $("#btnRepeat").removeClass("btn-gray");
                            $("#btnRepeat").addClass("btn-black");
                        }
                        if (msg.SongBroadcastMessage.shuffle) {
                            $("#btnShuffle").removeClass("btn-black");
                            $("#btnShuffle").addClass("btn-gray");
                        } 
                        else {
                            $("#btnShuffle").removeClass("btn-gray");
                            $("#btnShuffle").addClass("btn-black");
                        } 
                        if (msg.SongBroadcastMessage.paused) {
                            $("#playSymbol").removeClass("icon-play");
                            $("#playSymbol").addClass("icon-pause");
                            $("#btnPlay").removeClass("btn-black");
                            $("#btnPlay").addClass("btn-gray");
                        }
                        else {
                            if (msg.SongBroadcastMessage.playing) {
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

    
    $("#act_playlist").on("click", ".table-row", function() {
        url = "/player?select=" + $(this).attr("id");
        $.post(url, "", function(data, textStatus) {}, "json");
        
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
        url = "/player?next=true";
        $.post(url, "", function(data, textStatus) {}, "json");
    });

    $("#btnPrevious").click(function() {
        url = "/player?prev=true";
        $.post(url, "", function(data, textStatus) {}, "json");
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
        url = "/player?toPosition="+this.value;
        $.post(url, "", function (data, textStatus) {}, "json");
    };

    document.getElementById("volume-box").oninput = function () {
            url = "/player?volume="+this.value;
            $.post(url, "", function (data, textStatus) {}, "json");
    };

    $("#albumSearch").keyup(function() {
        //            if (console && console.log)
        name_typed = $("#albumSearch").val();
        console.log("request: " + name_typed);
        getAlbumList(name_typed);
    });

    getActualPlaylist();
    getAlbumList(name_typed);
    $('#albumSearch').val("");
    
});

function stopPlayer() {
    url = "/player?stop=true";
    $.post(url, "", function (data, textStatus) {}, "json");
}

function startPlay() {
    url = "/player?play=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}

function pausePlayer() {
    url = "/player?pause=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}

function toogleLoopPlayer() {
    url = "/player?toggleLoop=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}

function toogleRandomPlayer() {
    url = "/player?toggleShuffle=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}

function fastForwardPlayer() {
    url = "/player?fastForward=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}

function fastBackwardPlayer() {
    url = "/player?fastBackward=true";
    $.post(url, "", function(data, textStatus) {}, "json");
}


function albumSelect(albumId) {
    if (console && console.log) {
        console.log("albumSelect:" + albumId);
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
                $('#player').modal('show'); 
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
            if (a.uid == b.uid) 
                return 0;
            if (a.uid > b.uid) 
                return -1;
            if (a.uid < b.uid)
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
    url = "/playlist?show=";
    console.log("request for show actual playlist: ", url);
    $.getJSON(url).done(function(response) {
    console.log("reply for show actual playlist: ", response);
    $('#act_playlist tbody').empty();
    var trHTML ="";
    $.each(response, function(i, item) {
        trHTML += '<tr class="table-row" id="' + item.uid + '"><td>' + item.titel + '</td><td>' + item.performer + '</td><td>' + item.album + '</td></tr>';
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
