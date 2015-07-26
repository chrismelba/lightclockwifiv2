const char webconfig_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
$css
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<link rel=stylesheet href=style.css>
</head>
<body class=settings-page>
<h1>Choose Your Network</h1>
<form class=form-verticle action=/timezonesetup method=GET>
<ul>
$ssids
<label onClick=otherssid()>
<input type=radio name=ssid id=other_ssid value=other>or other ssid:</input>
<input type=text name=other id="other_text"/></label><br>
<label>Password: <input type=text name=pass id="pass"/></label><br>
<input type=submit value=Submit>
</form>
</body>
<script>function otherssid(){a=document.getElementById("other_ssid");a.checked=true}function regularssid(){a=document.getElementById("other_text");a.value=""};</script>
</html>
)=====";
