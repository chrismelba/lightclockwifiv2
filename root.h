const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
$css
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<link rel=stylesheet href=style.css>
</head>
<body class=settings-page>
<form class=form-verticle action=/ method=GET>
<ul>

<li class=checkbox>
Hour Colour: <input type='color' name='hourcolor' value='$hourcolor'/><br>
Minute Colour: <input type='color' name='minutecolor' value='$minutecolor'/><br>
Blend Point<br><input type='range' name='blendpoint' value='$blendpoint'><br>
</li>
<input class="btn btn-default" type=submit name=submit value='Update The Light Clock'/><br>
<input type='submit' name='submit' value='Save Scheme 1' /><input type='submit' name='submit' value='Save Scheme 2' /><input type='submit' name='submit' value='Save Scheme 3' /><br>
<input type='submit' name='submit' value='Load Scheme 1' /><input type='submit' name='submit' value='Load Scheme 2' /><input type='submit' name='submit' value='Load Scheme 3' /><br>
</form>
<div class=btn-box>
<a href=/settings>Settings</a><br>
</div>
</body>
</html>
)=====";
