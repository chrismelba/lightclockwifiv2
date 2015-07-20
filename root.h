const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head><style></style>
</head>
<body>
<form action='/' method='GET'>
Hour Colour: <input type='color' name='hourcolor' value='$hourcolor'/><br>
Minute Colour: <input type='color' name='minutecolor' value='$minutecolor'/><br>
Blend Point<br><input type='range' name='blendpoint' value='$blendpoint'><br>
<input type='submit' name='submit' value='Update The Light Clock'/><br>
<input type='submit' name='submit' value='Save Scheme 1' /><input type='submit' name='submit' value='Save Scheme 2' /><input type='submit' name='submit' value='Save Scheme 3' /><br>
<input type='submit' name='submit' value='Load Scheme 1' /><input type='submit' name='submit' value='Load Scheme 2' /><input type='submit' name='submit' value='Load Scheme 3' /><br>
</form>
<a href=/settings>Settings</a><br>
</body>
</html>
)=====";
