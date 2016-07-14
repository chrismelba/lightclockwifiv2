const char alarm_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>

  <meta http-equiv=Content-Type content='text/html; charset=utf-8' />
  <meta name=viewport content='width=device-width, initial-scale=1.0'>
<style>
@import url(http://fonts.googleapis.com/css?family=Abel);@import url(https://maxcdn.bootstrapcdn.com/font-awesome/4.3.0/css/font-awesome.min.css);
</style>

$externallinks
<script>
jQuery(document).ready(function($){
  $("#alarmcolorspectrum").spectrum({
      flat: true,
      preferredFormat: "hex",
      showButtons: false,
      showInput: true,
      move: function(color) {c=color.toRgb(); alarmcolor = new RGBColour(c.r, c.g, c.b);
                             document.getElementById("alarmcolor").value=document.getElementById("alarmcolorspectrum").value;},
      change: function(color) {document.getElementById("alarmcolor").value=document.getElementById("alarmcolorspectrum").value;}
    });
  })
</script>
</head>
<body class=settings-page>
<form class=form-verticle action=/ method=GET>
<div class="color-box" style="width: 100% !important">
  <label>Alarm Colour</label>
  <input type='color' name = 'alarmcolorspectrum' id='alarmcolorspectrum' value='#ff0000'/>
  <input type='hidden' name = 'alarmcolor' id = 'alarmcolor' value = '#ff0000'>
</div>
<ul>
<li>
<label>Hours: </label>
<div class=form-field>
<input type=number name=alarmhour value=0 id="alarmhour"/><br>
</div>
</li>
<li>
<label>Minutes: </label>
<div class=form-field><input type=number name=alarmmin value=1 id="alarmmin"/></label><br>
</div>
</li>
<li>
<label>Seconds: </label>
<div class=form-field>
<input type=number name=alarmsec value=0 id="alarmsec"/></label><br>
</div>
</li>
<input type=submit value=Submit>
</form>
</body>
</html>
)=====";
