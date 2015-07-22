const char timezone_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>$css<title>Time Zone</title></head>
<form action=/ method=GET>
Set timezone manually: <input type=text name=timezone id=timezone value=$timezone><br>
<button type=button onclick=getLocation()>Use Latitude/Longitude for Timezone:</button><br>
<p id=demo></p>
<input type=text name=latitude id=latitude>
<input type=text name=longitude id=longitude><br>
<input type=submit name=submit value='Update Timezone'/></form>
<a href=/>Return without saving</a><br>
<script>var x=document.getElementById("latitude");var y=document.getElementById("longitude");function getLocation(){if(navigator.geolocation){navigator.geolocation.getCurrentPosition(showPosition)}else{x.innerHTML="Geolocation is not supported by this browser."}}function showPosition(a){console.log("in showPosition");x.value=Math.round(a.coords.latitude*100)/100;y.value=Math.round(a.coords.longitude*100)/100};</script>
</body>
</html>
)=====";
