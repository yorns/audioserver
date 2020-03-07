/*jslint devel: true */
var webSocket;
var songID;
$(document).ready(function () {

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
                    $("#songProgress").css("width", msg.SongBroadcastMessage.position + "%");
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
        alert($(this).attr("id"));
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

    $("#albumSearch").keyup(function() {
        //            if (console && console.log)
        var name_typed = $("#albumSearch").val();
        console.log("request: " + name_typed);
        // alert("keypressed:"+$("#albumSearch").val());
        getAlbumList(name_typed);
    });

    readPlaylist();
    getActualPlaylist();
    getAlbumList("");
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


function albumSelect(album) {
    if (console && console.log)
        console.log("albumSelect:" + album);
    // open overlay / new page
    // create playlist with all album titles
    var url = "/playlist?createAlbumList=" + encodeURIComponent(album);
    if (console && console.log)
        console.log("request: " + url);
    $.getJSON(url).done(function(response) {
        if (response.result != "ok") {
            alert(response.result);
        } else {
            stopPlayer();
            readPlaylist();
            getActualPlaylist();
            getAlbumList("");
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
        var trHTML = '<div class="container"> <div class="row mt-5 justify-content-center" id="myimg">';
        $.each(response, function(i, item) {
            trHTML += `
                        <div class="card card-custom mx-2 mb-3"> 
                        <img class="card-img-top" style="width: 240px" src="${item.cover}" alt="${item.album}" id="${item.uid}">
                        <div class="card-body">
                          <h5 class="card-title" style="width: 200px">${item.album}</h5>
                          <p class="card-text">${item.performer}</p>
                        </div>
                        </div>`
        });
        trHTML += '</div>';
        $('#cover').append(trHTML);
        $("#myimg div img").on("click", function() {
            albumSelect($(this).attr("alt"));
        }); //??
    })
}

function readPlaylist() {
    var url = "/playlist?showLists=true";
    $.getJSON(url).done(function(response) {
        var items = '';
        $.each(response.playlists, function(i, item) {
            var text = item.playlist;
            items += '<a class="dropdown-item" href="#">' + decodeURIComponent(text) + '</a>';
        });

        $("#demolist").html(items);
        $('#actPlaylistName').text(decodeURIComponent(response.actualPlaylist));

        //alert(response.actualPlaylist);

        $('#demolist a').on('click', function() {
            var txt = encodeURIComponent($(this).text());
            var url = "/playlist?change=" + txt;
            $.getJSON(url).done(function(response) {
                getActualPlaylist();
                //alert(txt);
                $('#actPlaylistName').html(txt);
            });
        });
    });
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
            getAlbumList("");
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
