const char clocktype_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<link rel=stylesheet href='clockmenustyle.css'>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
<script type="text/javascript">
function CheckClockType(val){
 var pixcount=document.getElementById('pixelCount');
 var power=document.getElementById('powerType');


 switch(val) {
   case '1':
    document.getElementById('pixelCountli').style.display='none';
    document.getElementById('powerTypeli').style.display='none';
    pixcount.value=120;
    power.value=1;
    break;
  case '2':
    document.getElementById('pixelCountli').style.display='none';
    document.getElementById('powerTypeli').style.display='none';
    pixcount.value=60;
    power.value=2;
    break;
  case '3':
    document.getElementById('pixelCountli').style.display='block';
    document.getElementById('powerTypeli').style.display='block';
    break;


 }

}

</script> 
<title>Settings</title>
</head>
<body class=settings-page>
<form class=form-verticle action=/ method=GET>
<ul>

<li>
<label>Time Zone</label>
<div class=form-field>
<select name=timezone id=timezone>
<option value="1" $timezonevalue1 > (GMT-12:00) International Date Line West</option>     
<option value="2" $timezonevalue2 > (GMT-11:00) Midway Island, Samoa</option>      
<option value="3" $timezonevalue3 > (GMT-10:00) Hawaii</option>        
<option value="4" $timezonevalue4 > (GMT-09:00) Alaska</option>        
<option value="5" $timezonevalue5 > (GMT-08:00) Pacific Time (US & Canada)</option>    
<option value="6" $timezonevalue6 > (GMT-08:00) Tijuana, Baja California</option>      
<option value="7" $timezonevalue7 > (GMT-07:00) Arizona</option>        
<option value="8" $timezonevalue8 > (GMT-07:00) Chihuahua, La Paz, Mazatlan</option>     
<option value="9" $timezonevalue9 > (GMT-07:00) Mountain Time (US & Canada)</option>    
<option value="10" $timezonevalue10 > (GMT-06:00) Central America</option>       
<option value="11" $timezonevalue11 > (GMT-06:00) Central Time (US & Canada)</option>    
<option value="12" $timezonevalue12 > (GMT-06:00) Guadalajara, Mexico City, Monterrey</option>     
<option value="13" $timezonevalue13 > (GMT-06:00) Saskatchewan</option>        
<option value="14" $timezonevalue14 > (GMT-05:00) Bogota, Lima, Quito, Rio Branco</option>    
<option value="15" $timezonevalue15 > (GMT-05:00) Eastern Time (US & Canada)</option>    
<option value="16" $timezonevalue16 > (GMT-05:00) Indiana (East)</option>       
<option value="17" $timezonevalue17 > (GMT-04:00) Atlantic Time (Canada)</option>      
<option value="18" $timezonevalue18 > (GMT-04:00) Caracas, La Paz</option>      
<option value="19" $timezonevalue19 > (GMT-04:00) Manaus</option>        
<option value="20" $timezonevalue20 > (GMT-04:00) Santiago</option>        
<option value="21" $timezonevalue21 > (GMT-03:30) Newfoundland</option>        
<option value="22" $timezonevalue22 > (GMT-03:00) Brasilia</option>        
<option value="23" $timezonevalue23 > (GMT-03:00) Buenos Aires, Georgetown</option>      
<option value="24" $timezonevalue24 > (GMT-03:00) Greenland</option>        
<option value="25" $timezonevalue25 > (GMT-03:00) Montevideo</option>        
<option value="26" $timezonevalue26 > (GMT-02:00) Mid-Atlantic</option>        
<option value="27" $timezonevalue27 > (GMT-01:00) Cape Verde Is.</option>      
<option value="28" $timezonevalue28 > (GMT-01:00) Azores</option>        
<option value="29" $timezonevalue29 > (GMT+00:00) Casablanca, Monrovia, Reykjavik</option>      
<option value="30" $timezonevalue30 > (GMT+00:00) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London</option> 
<option value="31" $timezonevalue31 > (GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna</option>   
<option value="32" $timezonevalue32 > (GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague</option>    
<option value="33" $timezonevalue33 > (GMT+01:00) Brussels, Copenhagen, Madrid, Paris</option>     
<option value="34" $timezonevalue34 > (GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb</option>     
<option value="35" $timezonevalue35 > (GMT+01:00) West Central Africa</option>      
<option value="36" $timezonevalue36 > (GMT+02:00) Amman</option>        
<option value="37" $timezonevalue37 > (GMT+02:00) Athens, Bucharest, Istanbul</option>      
<option value="38" $timezonevalue38 > (GMT+02:00) Beirut</option>        
<option value="39" $timezonevalue39 > (GMT+02:00) Cairo</option>        
<option value="40" $timezonevalue40 > (GMT+02:00) Harare, Pretoria</option>       
<option value="41" $timezonevalue41 > (GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius</option>   
<option value="42" $timezonevalue42 > (GMT+02:00) Jerusalem</option>        
<option value="43" $timezonevalue43 > (GMT+02:00) Minsk</option>        
<option value="44" $timezonevalue44 > (GMT+02:00) Windhoek</option>        
<option value="45" $timezonevalue45 > (GMT+03:00) Kuwait, Riyadh, Baghdad</option>      
<option value="46" $timezonevalue46 > (GMT+03:00) Moscow, St. Petersburg, Volgograd</option>     
<option value="47" $timezonevalue47 > (GMT+03:00) Nairobi</option>        
<option value="48" $timezonevalue48 > (GMT+03:00) Tbilisi</option>        
<option value="49" $timezonevalue49 > (GMT+03:30) Tehran</option>        
<option value="50" $timezonevalue50 > (GMT+04:00) Abu Dhabi, Muscat</option>      
<option value="51" $timezonevalue51 > (GMT+04:00) Baku</option>        
<option value="52" $timezonevalue52 > (GMT+04:00) Yerevan</option>        
<option value="53" $timezonevalue53 > (GMT+04:30) Kabul</option>        
<option value="54" $timezonevalue54 > (GMT+05:00) Yekaterinburg</option>        
<option value="55" $timezonevalue55 > (GMT+05:00) Islamabad, Karachi, Tashkent</option>      
<option value="56" $timezonevalue56 > (GMT+05:30) Sri Jayawardenapura</option>       
<option value="57" $timezonevalue57 > (GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi</option>    
<option value="58" $timezonevalue58 > (GMT+05:45) Kathmandu</option>        
<option value="59" $timezonevalue59 > (GMT+06:00) Almaty, Novosibirsk</option>       
<option value="60" $timezonevalue60 > (GMT+06:00) Astana, Dhaka</option>       
<option value="61" $timezonevalue61 > (GMT+06:30) Yangon (Rangoon)</option>       
<option value="62" $timezonevalue62 > (GMT+07:00) Bangkok, Hanoi, Jakarta</option>      
<option value="63" $timezonevalue63 > (GMT+07:00) Krasnoyarsk</option>        
<option value="64" $timezonevalue64 > (GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi</option>    
<option value="65" $timezonevalue65 > (GMT+08:00) Kuala Lumpur, Singapore</option>      
<option value="66" $timezonevalue66 > (GMT+08:00) Irkutsk, Ulaan Bataar</option>      
<option value="67" $timezonevalue67 > (GMT+08:00) Perth</option>        
<option value="68" $timezonevalue68 > (GMT+08:00) Taipei</option>        
<option value="69" $timezonevalue69 > (GMT+09:00) Osaka, Sapporo, Tokyo</option>      
<option value="70" $timezonevalue70 > (GMT+09:00) Seoul</option>        
<option value="71" $timezonevalue71 > (GMT+09:00) Yakutsk</option>        
<option value="72" $timezonevalue72 > (GMT+09:30) Adelaide</option>        
<option value="73" $timezonevalue73 > (GMT+09:30) Darwin</option>        
<option value="74" $timezonevalue74 > (GMT+10:00) Brisbane</option>        
<option value="75" $timezonevalue75 > (GMT+10:00) Canberra, Melbourne, Sydney</option>      
<option value="76" $timezonevalue76 > (GMT+10:00) Hobart</option>        
<option value="77" $timezonevalue77 > (GMT+10:00) Guam, Port Moresby</option>      
<option value="78" $timezonevalue78 > (GMT+10:00) Vladivostok</option>        
<option value="79" $timezonevalue79 > (GMT+11:00) Magadan, Solomon Is., New Caledonia</option>    
<option value="80" $timezonevalue80 > (GMT+12:00) Auckland, Wellington</option>       
<option value="81" $timezonevalue81 > (GMT+12:00) Fiji, Kamchatka, Marshall Is.</option>     
<option value="82" $timezonevalue82 > (GMT+13:00) Nuku'alofa</option>        

</select>
<br>
</li>
<li class=checkbox>
<input id=DSThidden type=hidden name=DSThidden value=0>
<input id=DST type=checkbox name=DST $DSTtime>
<label for=DST>Daylight Savings</label>
</li>
<li>
<label>Clock Type</label>
<div class=form-field>
<select name=clocktype id=clocktype onchange='CheckClockType(this.value);'>
      
<option value="1"  $original> Original</option>      
<option value="2"  $mini> Mini</option>        
<option value="3" $customtype > Custom</option>      

</select>
<br>
</li>
<li id=pixelCountli style='display:none;'>
<input name=pixelCount id=pixelCount type="number" value=120 name="quantity" min="1" max="255">
<label for=pixelCount>Number of LEDs</label>
</li>
<li id=powerTypeli style='display:none;'>
<label>Power Type</label>
<div class=form-field>
<select name=powerType id=powerType>
      
<option value="1" selected > Mains Power</option>      
<option value="2" > USB Power</option>        
    

</select>

</li>



<div class=btn-box>
<input class="btn btn-default" type=submit name=submit value='Save Settings'/>
</div>
</ul>
</form>

</body>
</html>
)=====";
