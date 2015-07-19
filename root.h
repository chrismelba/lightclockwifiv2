const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head><style></style>
</head>
<body>
<form action='/' method='GET'>"
"Hour Colour: <input type='color' name='hourcolor' value='$hourcolor'/><br>
Minute Colour: <input type='color' name='minutecolor' value='$minutecolor'/><br>
Blend Point<br><input type='range' name='blendpoint' value='$blendpoint'>""<br>
<input type='submit' name='submit' value='Update The Light Clock'/></form>
</body>
</html>
)=====";
