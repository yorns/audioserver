/*jslint devel: true */
var webSocket;
var songID;
var name_typed;

$(document).ready(function () {

    name_typed = "";
    
    function runWebsocket() {

        if ("WebSocket" in window) {

            webSocket = new WebSocket("ws://" + location.host + "/dynamic");

            webSocket.onerror = function (event) {
                //onError(event)
                alert("ws error" + event.data);
            };

            webSocket.onopen = function (event) {
                //onOpen(event)
            };

            webSocket.onmessage = function (event) {
                try {
                    var msg = JSON.parse(event.data);
                    // TODO: only if song ID is new, request the name
                    if (songID != msg.SongBroadcastMessage.songID) {
                        songID = msg.SongBroadcastMessage.songID;
                        var url = "/database?uid=" + msg.SongBroadcastMessage.songID;
                        $.getJSON(url).done(function(response) {
                            //                    console.log('received: ' + response[0].titel);
                            if (response) {
                                $("#actSong").html(response[0].performer + "<br>" + response[0].album + "<br>" + response[0].titel);
                                $("#actCover").attr("src", response[0].cover);
                            }
                        });
                        getActualPlaylist();
                    }
                    //console.log('received: loop: ' + msg.SongBroadcastMessage.loop + ' shuffle: ' + msg.SongBroadcastMessage.shuffle );
                    $("#songProgress").css("width", msg.SongBroadcastMessage.position + "%");
                    if (msg.SongBroadcastMessage.loop) {
                        $("#btnRepeat").removeClass("btn-gray");
                        $("#btnRepeat").addClass("btn-black");
                    }
                    else {
                        $("#btnRepeat").removeClass("btn-black");
                        $("#btnRepeat").addClass("btn-gray");
                    }
                    if (msg.SongBroadcastMessage.shuffle) {
                        $("#btnShuffle").removeClass("btn-gray");
                        $("#btnShuffle").addClass("btn-black");
                    } 
                    else {
                        $("#btnShuffle").removeClass("btn-black");
                        $("#btnShuffle").addClass("btn-gray");
                    }                    
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
    
    $("#act_playlist").on("click", ".table-row", function() {
        //alert($(this).attr("id"));
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

    $("#progress-box").click(function(e) {
        var posX = $(this).offset().left;
        var width = $("#progress-box").width();
        //alert("progress "+ (e.pageX - posX)*100/width);
        var toPosition = parseInt((e.pageX - posX)*100/width);
            url = "/player?toPosition="+toPosition;
            $.post(url, "", function (data, textStatus) {}, "json");

    });

    $("#albumSearch").keyup(function() {
        //            if (console && console.log)
        name_typed = $("#albumSearch").val();
        console.log("request: " + name_typed);
        getAlbumList(name_typed);
    });

    getActualPlaylist();
    getAlbumList(name_typed);
    $('#albumSearch').val("");
    
    $('#ex1').slider({
	formatter: function(value) {
		return 'Current value: ' + value;
	}
});
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
    if (console && console.log)
        console.log("albumSelect:" + albumId);
    // open overlay / new page
    // create playlist with all album titles
    var url = "/playlist?change=" + encodeURIComponent(albumId);
    if (console && console.log)
        console.log("request: " + url);
    $.getJSON(url).done(function(response) {
        if (response.result != "ok") {
            alert(response.result);
        } else {
            stopPlayer();
//            readPlaylist();
//            getActualPlaylist();
            getAlbumList(name_typed);
            startPlay();
        }

    });

    // show playlist with image
    // show player

}

function getAlbumList(searchString) {
    var url = "/database?albumList=" + searchString;
    $.getJSON(url).done(function(response) {
        $('#cover').empty();
        var trHTML = '<div class="container-fluid"> <div class="row mt-5 justify-content-center" id="myimg">';
        $.each(response, function(i, item) {
            trHTML += `
                        <div class="card card-custom mx-2 mb-3"> 
                        <img class="card-img-top" style="width: 28em" src="${item.cover}" alt="${item.album}" id="${item.uid}">
                        <div class="card-body">
                          <h5 class="card-title" style="width: 11em">${item.album}</h5>
                          <p class="card-text">${item.performer}</p>
                        </div>
                        </div>`
        });
        trHTML += '</div>';
        $('#cover').append(trHTML);
        $("#myimg div img").on("click", function() {
            albumSelect($(this).attr("id"));
        }); //??
    })
}

function getActualPlaylist() {
    url = "/playlist?show=";
    $.getJSON(url).done(function(response) {
        $('#act_playlist tbody').empty();
        var trHTML;
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
