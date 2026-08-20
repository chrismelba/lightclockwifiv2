const char timezonesetup_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head><title>Time Zone</title>
<link rel=stylesheet href="/clockmenustyle.css">
<meta name=viewport content="width=device-width, initial-scale=1">
</head>
<body class="settings-page">
<div class=container>

<a href='/'>Network</a>-> <a href='/passwordinput'>Password</a> -> <strong>Timezone</strong><BR>
<h2>Where does this clock live?</h2>
<p class=zone-note>Pick the place and the clock handles daylight saving on its own &mdash; it knows when your region changes over, and it will keep knowing every year without being told.</p>

<form class=form-verticle action=/a method=GET>
<div class=zone-picker>
<button class="btn btn-sm" type=button id=detect>Use this device's location</button>
<input type=search id=zonefilter placeholder="Search for a city or region" autocomplete=off>
<input type=hidden name=zone id=zonevalue value=$zoneindex>
<select id=zone size=10><option value=$zoneindex>Loading zones&hellip;</option></select>
<p class=zone-status id=zonestatus></p>
</div>

<ul>
<li>
<label>Clock Type</label>
<div class=form-field>
<select name=clocktype id=clocktype onchange='CheckClockType(this.value);'>
<option value="1" selected> Original</option>
<option value="2"> Mini</option>
<option value="3"> Custom</option>
</select>
</div>
</li>
<li id=pixelCountli style='display:none;'>
<input name=pixelCount id=pixelCount type="number" value=120 min="1" max="255">
<label for=pixelCount>Number of LEDs</label>
</li>
<li id=powerTypeli style='display:none;'>
<label>Power Type</label>
<div class=form-field>
<select name=powerType id=powerType>
<option value="1" selected> Mains Power</option>
<option value="2"> USB Power</option>
</select>
</div>
</li>
</ul>

<div class="btn-box">
<input class=btn type=submit name=submit value='Save and Restart'>
</div>
</form>
<noscript><p class=zone-status>This page needs JavaScript to load the zone list.</p></noscript>
</div>
<script>
function CheckClockType(val){
 var pixcount=document.getElementById('pixelCount');
 var power=document.getElementById('powerType');
 switch(val){
  case '1':
   document.getElementById('pixelCountli').style.display='none';
   document.getElementById('powerTypeli').style.display='none';
   pixcount.value=120;power.value=1;break;
  case '2':
   document.getElementById('pixelCountli').style.display='none';
   document.getElementById('powerTypeli').style.display='none';
   pixcount.value=60;power.value=2;break;
  case '3':
   document.getElementById('pixelCountli').style.display='block';
   document.getElementById('powerTypeli').style.display='block';
   break;
 }
}
(function(){
var sel=document.getElementById('zone'),filter=document.getElementById('zonefilter'),
    field=document.getElementById('zonevalue'),
    status=document.getElementById('zonestatus'),detect=document.getElementById('detect'),
    zones=[],chosen=$zoneindex;

function render(){
  var q=filter.value.trim().toLowerCase(),html='',shown=0;
  for(var i=0;i<zones.length;i++){
    if(q&&zones[i].toLowerCase().indexOf(q)<0)continue;
    html+='<option value="'+i+'"'+(i===chosen?' selected':'')+'>'+zones[i].replace(/_/g,' ')+'</option>';
    shown++;
  }
  if(!shown)html='<option disabled>No zone matches "'+q+'"</option>';
  sel.innerHTML=html;
}

// The hidden field is what actually gets submitted; the list only nominates.
function choose(i){chosen=i;field.value=i;}
sel.addEventListener('change',function(){choose(parseInt(sel.value,10));});
filter.addEventListener('input',render);

function autodetect(announce){
  var guess=null;
  try{guess=Intl.DateTimeFormat().resolvedOptions().timeZone;}catch(e){}
  if(!guess){if(announce)status.textContent="This browser won't say which zone it is in — pick from the list instead.";return;}
  var at=zones.indexOf(guess);
  if(at<0){if(announce)status.textContent='This device says '+guess+', which this clock does not know. Pick the closest city.';return;}
  choose(at);filter.value='';render();
  status.textContent='Set to '+guess.replace(/_/g,' ')+', from this device.';
}
detect.addEventListener('click',function(){autodetect(true);});

var req=new XMLHttpRequest();
req.open('GET','/tzlist',true);
req.onload=function(){
  if(req.status!==200){sel.innerHTML='<option>Could not load the zone list</option>';return;}
  zones=req.responseText.split('\n');
  while(zones.length&&!zones[zones.length-1])zones.pop();
  render();
  // First run: start from wherever the phone setting this up thinks it is.
  autodetect(false);
};
req.onerror=function(){sel.innerHTML='<option>Could not load the zone list</option>';};
req.send();
})();
</script>
</body>
</html>
)=====";
