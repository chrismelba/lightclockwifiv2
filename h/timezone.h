const char timezone_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head><title>Time Zone</title>
<meta name=viewport content="width=device-width, initial-scale=1">
<link rel=stylesheet href="/clockmenustyle.css">
</head>
<body class="settings-page">
<div class=container>
<h2>Where does this clock live?</h2>
<p class=zone-note>Pick the place and the clock handles daylight saving on its own &mdash; it knows when your region changes over, and it will keep knowing every year without being told.</p>

<form action=/ method=GET>
<div class=zone-picker>
<button class="btn btn-sm" type=button id=detect hidden>Use this device's location</button>
<input type=search id=zonefilter placeholder="Search for a city or region" autocomplete=off hidden>
<select name=zone id=zone size=12>$zoneoptions</select>
<p class=zone-status id=zonestatus>Now set to $zonesummary</p>
</div>
<div class="btn-box"><input class=btn type=submit name=submit value='Save time zone'></div>
</form>
</div>
<a href=/>Return without saving</a><br>
<script>
(function(){
var sel=document.getElementById('zone'),filter=document.getElementById('zonefilter'),
    status=document.getElementById('zonestatus'),detect=document.getElementById('detect'),
    chosen=sel.value,all=[];
for(var i=0;i<sel.options.length;i++)all.push([sel.options[i].value,sel.options[i].text]);

// The search box and detect button only appear once this runs, so a browser
// that will not run it is left with a plain, complete, working list.
filter.hidden=false;detect.hidden=false;

function render(){
  var q=filter.value.trim().toLowerCase(),html='',shown=0;
  for(var i=0;i<all.length;i++){
    var hit=!q||all[i][1].toLowerCase().indexOf(q)>=0;
    // Always keep the current choice on the list. Filtering it out of sight
    // would leave the browser submitting whatever option floated to the top.
    if(!hit&&all[i][0]!==chosen)continue;
    html+='<option value="'+all[i][0]+'"'+(all[i][0]===chosen?' selected':'')+'>'+all[i][1]+'</option>';
    if(hit)shown++;
  }
  sel.innerHTML=html;
  if(!shown)status.textContent='Nothing matches "'+filter.value.trim()+'".';
}

sel.addEventListener('change',function(){chosen=sel.value;});
filter.addEventListener('input',render);

detect.addEventListener('click',function(){
  var guess=null;
  try{guess=Intl.DateTimeFormat().resolvedOptions().timeZone;}catch(e){}
  if(!guess){status.textContent="This browser won't say which zone it is in — pick from the list instead.";return;}
  var want=guess.replace(/_/g,' '),at=-1;
  for(var i=0;i<all.length;i++)if(all[i][1]===want){at=i;break;}
  if(at<0){status.textContent='This device says '+guess+', which this clock does not know. Pick the closest city.';return;}
  chosen=all[at][0];filter.value='';render();sel.focus();
  status.textContent='This device is in '+want+'. Save to use it.';
});
})();
</script>
</body>
</html>
)=====";
