const char game_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<!DOCTYPE HTML>
<html>
<head>
$externallinks
<style>
*{-webkit-box-sizing:border-box; -moz-box-sizing:border-box; box-sizing:border-box;}
html{ /* Permalink - use to edit and share this gradient: http://colorzilla.com/gradient-editor/$playercolor+0,ffffff+50,e0e0e0+100 */
background: $playercolor; /* Old browsers */
background: -moz-linear-gradient(left,  $playercolor 0%, #ffffff 50%, $playercolor 100%); /* FF3.6+ */
background: -webkit-gradient(linear, left top, right top, color-stop(0%,$playercolor), color-stop(50%,#ffffff), color-stop(100%,$playercolor)); /* Chrome,Safari4+ */
background: -webkit-linear-gradient(left,  $playercolor 0%,#ffffff 50%,$playercolor 100%); /* Chrome10+,Safari5.1+ */
background: -o-linear-gradient(left,  $playercolor 0%,#ffffff 50%,$playercolor 100%); /* Opera 11.10+ */
background: -ms-linear-gradient(left,  $playercolor 0%,#ffffff 50%,$playercolor 100%); /* IE10+ */
background: linear-gradient(to right,  $playercolor 0%,#ffffff 50%,$playercolor 100%); /* W3C */
filter: progid:DXImageTransform.Microsoft.gradient( startColorstr='$playercolor', endColorstr='$playercolor',GradientType=1 ); /* IE6-9 */
}

body {
    text-align: center;
}

.bigbutton {
    display: inline-block;
    margin: 10px;
    -webkit-border-radius: 8px;
    -moz-border-radius: 8px;
    border-radius: 8px;
    -webkit-box-shadow:    0 8px 0 #c5376d, 0 15px 20px rgba(0, 0, 0, .35);
    -moz-box-shadow: 0 8px 0 #c5376d, 0 15px 20px rgba(0, 0, 0, .35);
    box-shadow: 0 8px 0 #c5376d, 0 15px 20px rgba(0, 0, 0, .35);
    -webkit-transition: -webkit-box-shadow .1s ease-in-out;
    -moz-transition: -moz-box-shadow .1s ease-in-out;
    -o-transition: -o-box-shadow .1s ease-in-out;
    transition: box-shadow .1s ease-in-out;
    font-size: 50px;
    color: #fff;
}

.bigbutton span {
    display: inline-block;
    padding: 200px 30px;
    background-color: $playercolor;
    background-image: -webkit-gradient(linear, 0% 0%, 0% 100%, from(hsla(338, 90%, 80%, .8)), to(hsla(338, 90%, 70%, .2)));
    background-image: -webkit-linear-gradient(hsla(338, 90%, 80%, .8), hsla(338, 90%, 70%, .2));
    background-image: -moz-linear-gradient(hsla(338, 90%, 80%, .8), hsla(338, 90%, 70%, .2));
    background-image: -o-linear-gradient(hsla(338, 90%, 80%, .8), hsla(338, 90%, 70%, .2));
    -webkit-border-radius: 8px;
    -moz-border-radius: 8px;
    border-radius: 8px;
    -webkit-box-shadow: inset 0 -1px 1px rgba(255, 255, 255, .15);
    -moz-box-shadow: inset 0 -1px 1px rgba(255, 255, 255, .15);
    box-shadow: inset 0 -1px 1px rgba(255, 255, 255, .15);
    font-family: 'Pacifico', Arial, sans-serif;
    line-height: 1;
    text-shadow: 0 -1px 1px rgba(175, 49, 95, .7);
    -webkit-transition: background-color .2s ease-in-out, -webkit-transform .1s ease-in-out;
    -moz-transition: background-color .2s ease-in-out, -moz-transform .1s ease-in-out;
    -o-transition: background-color .2s ease-in-out, -o-transform .1s ease-in-out;
    transition: background-color .2s ease-in-out, transform .1s ease-in-out;
}

.bigbutton:hover span {
    background-color: $playercolor;
    text-shadow: 0 -1px 1px rgba(175, 49, 95, .9), 0 0 5px rgba(255, 255, 255, .8);
}

.bigbutton:active, .bigbutton:focus {
    -webkit-box-shadow:    0 8px 0 #c5376d, 0 12px 10px rgba(0, 0, 0, .3);
    -moz-box-shadow: 0 8px 0 #c5376d, 0 12px 10px rgba(0, 0, 0, .3);
    box-shadow:    0 8px 0 #c5376d, 0 12px 10px rgba(0, 0, 0, .3);
}

.bigbutton:active span {
    -webkit-transform: translate(0, 4px);
    -moz-transform: translate(0, 4px);
    -o-transform: translate(0, 4px);
    transform: translate(0, 4px);
}

</style>
<title>Game On!</title>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<script>
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.onopen = function () {  connection.send('Connect ' + new Date());
                                   connection.send("newplayer|");};
connection.onerror = function (error) {    console.log('WebSocket Error ', error);};
connection.onmessage = function (e) {  console.log('Server: ', e.data);};

function gameplus(val){
  connection.send("gameplus|");
}

function gamestart(val){
  connection.send("gamestart|");
}
</script>
</head>
<body class=settings-page>
<a href="#" class="bigbutton" onclick="gameplus()">
    <span>Click Fast To Win!</span>
</a>

<button onclick="gamestart()">Click to start</button>
</body>
</html>
)=====";
