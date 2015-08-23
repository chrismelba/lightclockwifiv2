const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
$fonts
$css
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js"></script>
<script src="http://thelightclock.com/clockjshosting/Colour.js"></script>
<script src="http://thelightclock.com/clockjshosting/spectrum.js"></script>
<script src="http://thelightclock.com/clockjshosting/clock.js"></script>
<link rel=stylesheet href="http://thelightclock.com/clockjshosting/spectrum.css">
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<link rel=stylesheet href=style.css>
</head>
<body class=settings-page>
      <div id="canvasholder">
      <canvas id="canvas"></canvas>
    </div>
<form class=form-verticle action=/ method=GET>
<ul>

<li class=checkbox>
  Hour Colour  <input type='color' name = 'hourcolor' id="hourcolor" value='$hourcolor'/><input type='color' name='minutecolor' id="minutecolor" value='$minutecolor'/>  Minute Colour<br>

Blend Point<br><input type='range' id= 'blendpoint' name='blendpoint' value=$blendpoint><br>
</li>
<input class="btn btn-default" type=submit name=submit value='Update The Light Clock'/><br>
<input class="scheme1" type='submit' name='submit' value='Save Scheme 1' /><input type='submit' class="scheme2" name='submit' value='Save Scheme 2' /><input type='submit' class="scheme3" name='submit' value='Save Scheme 3' /><br>
<input class="scheme1" type='submit' name='submit' value='Load Scheme 1' /><input type='submit' class="scheme2" name='submit' value='Load Scheme 2' /><input type='submit' class="scheme3" name='submit' value='Load Scheme 3' /><br>
</form>
<div class=btn-box>
<a href=/settings>Settings</a><br>
</div>
</body>
</html>
)=====";
