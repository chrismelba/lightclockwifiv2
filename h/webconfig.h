const char webconfig_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
<link rel=stylesheet href='clockmenustyle.css'>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
</head>
<body class=settings-page>
<strong>Network </strong>-> Password -> Timezone<BR>
<h1>Choose Your Network</h1>
<form class=form-verticle action=/passwordinput method=GET>
<ul>
$ssids
<label onClick=otherssid()>
<input type=radio name=ssid id=other_ssid value=other>or other ssid:</input>
<input type=text name=other id="other_text"/></label><br>
<input type=submit value=Submit>
</form>
</body>
<script>function otherssid(){a=document.getElementById("other_ssid");a.checked=true}function regularssid(){a=document.getElementById("other_text");a.value=""};</script>
</html>
)=====";
