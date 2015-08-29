const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
  <meta http-equiv=Content-Type content='text/html; charset=utf-8' />
  <meta name=viewport content='width=device-width, initial-scale=1.0'>
$externallinks
</head>
<body class=settings-page>

  <div id='canvasholder'>
    <canvas id='canvas'></canvas>
  </div>
  <form action=/ method=GET>
    <div class="color-section">
      <div class="color-box">
        <label>Hour Colour</label>
        <input type='text' name = 'hourcolorspectrum' id='hourcolorspectrum' value='$hourcolor'/>
        <input type='hidden' name = 'hourcolor' id = 'hourcolor' value = '$hourcolor'>
      </div>
      <div class="color-box">
        <label>Minute Colour</label>
        <input type='text' name='minutecolorspectrum' id='minutecolorspectrum' value='$minutecolor'/>
        <input type='hidden' name = 'minutecolor' id = 'minutecolor' value = '$minutecolor'/>
      </div>  
    </div>
    <div class="slide-section">
      <div class="point-slide">
        <label>Blend Point</label>
        <input type='range' id= 'blendpoint' name='blendpoint' >
      </div>
      <div class="point-slide">
        <label>Dimming</label>
        <input type='range' id= 'dimming' name='dimming' >
      </div>
    </div>
    <div class="btn-box">
      <input class='btn btn-sm' type='submit' name='submit' value='Save Scheme 1' />
      <input class='btn btn-sm' type='submit' name='submit' value='Save Scheme 2' />
      <input class='btn btn-sm' type='submit' name='submit' value='Save Scheme 3' />
    </div>
    <div class="btn-box">
      <input class='btn btn-sm' type='submit' name='submit' value='Load Scheme 1' />
      <input class='btn btn-sm' type='submit' name='submit' value='Load Scheme 2' />
      <input class='btn btn-sm' type='submit' name='submit' value='Load Scheme 3' />
    </div>

    <div class="btn-footer">
      <a class="btn btn-default" href=/settings>Settings</a>
      <input class='btn btn-green' type=submit name=submit value='Update The Light Clock'/>
    </div>
  </form>

</body>
</html>
)=====";
