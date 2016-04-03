const char alarm_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
  <meta http-equiv=Content-Type content='text/html; charset=utf-8' />
  <meta name=viewport content='width=device-width, initial-scale=1.0'>
<style>
@import url(http://fonts.googleapis.com/css?family=Abel);@import url(https://maxcdn.bootstrapcdn.com/font-awesome/4.3.0/css/font-awesome.min.css);
</style>

$externallinks

</head>
<body class=settings-page>
<form class=form-verticle action=/ method=GET>
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
