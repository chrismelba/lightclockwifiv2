// Every asset below is served from the clock itself, so the UI renders with no
// internet access and works identically in AP mode. See tools/build_assets.py.
const char externallinks[] PROGMEM = R"=====(
<link rel=stylesheet href="/clockmenustyle.css">
<script src="/Colour.js"></script>
<script src="/clock.js"></script>
)=====";
