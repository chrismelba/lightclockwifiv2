const char game_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<!DOCTYPE HTML>
<html>
<head>
$externallinks

<title>Settings</title>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<script>
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.onopen = function () {  connection.send('Connect ' + new Date()); };
connection.onerror = function (error) {    console.log('WebSocket Error ', error);};
connection.onmessage = function (e) {  console.log('Server: ', e.data);};

function gameplus(val){
  connection.send("gameplus|");
}
function newplayer(val){
  connection.send("newplayer|");
}
function gamestart(val){
  connection.send("gamestart|");
}
</script>
</head>
<body class=settings-page>
<button onclick="gameplus()">Click to win</button>
<button onclick="newplayer()">Click to join</button>
<button onclick="gamestart()">Click to start</button>
</body>
</html>
)=====";
