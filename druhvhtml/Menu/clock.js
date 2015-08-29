var width; 
var height;
var needRedraw = false;
var key_left = false;
var key_right = false;
var key_up = false;
var drawRadius;
var innerRadius;
var curTime;
var lastSeconds = -1;
var TwoPI = 2 * Math.PI;
var pixelCount = 300;
var INTERVAL = TwoPI / pixelCount;
var ctx;// = $("#canvas")[0].getContext('2d');

var hourcolor;
var minutecolor;
var blendpoint = 0.5;
var clock = [];


function show(ctx, x, y) {
  ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (-Math.PI/2);

  for (var i = 0; i < pixelCount; i++) {
    var a = i * INTERVAL;
    var end = a + INTERVAL + (Math.PI / 100);
    
    ctx.beginPath ();
     ctx.fillStyle = clock[i].getCSSIntegerRGB();
     ctx.arc (0, 0, drawRadius, a, end);
     ctx.arc (0, 0, innerRadius, end, a, true);
     ctx.fill();
  };

  //make the black face
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius, 0, TwoPI);
  ctx.fillStyle = "black";
  ctx.fill();
  //make the centre bolt
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius*0.03, 0, TwoPI);
  ctx.fillStyle = "silver";
  ctx.fill();
  //ctx.restore();

  //make the white inner rim
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius, 0, TwoPI);
  ctx.strokeStyle = "white";
  ctx.stroke();
  //ctx.restore();
  //make the white outter rim
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius*1.2, 0, TwoPI);
  //ctx.strokeStyle = "white";
  ctx.stroke();
  ctx.restore();




    ctx.beginPath();
    ctx.strokeStyle = "transparent";
    
    var grdRadial = ctx.createRadialGradient(x, y,innerRadius *1.2 , x, y, drawRadius);
    
    grdRadial.addColorStop(0, "rgba(255, 255, 255, 0)");//outside(1 is opauque)
    grdRadial.addColorStop(0.01, "rgba(255, 255, 255, 0.4)");//outside(1 is opauque)
    //grdRadial.addColorStop(0.501, "rgba(255, 255, 255, 0.2)");//outside(1 is opauque)
    //grdRadial.addColorStop(0.5, "rgba(255, 255, 255, 0)");
    grdRadial.addColorStop(1, "rgba(255, 255, 255, 1)"); //1 is outter (0 is transparent)
    
    
    //ctx.fillRect(10, 10, 150, 100);
    ctx.arc(x, y, drawRadius*1.01,0,2*Math.PI);
    ctx.fillStyle = grdRadial;
    ctx.fill();
}

function standardclock(hour_pos, min_pos, ctx, x, y) {
  ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (Math.PI);

  ctx.rotate (TwoPI*hour_pos/pixelCount);
  ctx.beginPath();
  ctx.moveTo(0,0);
  ctx.lineTo(0,innerRadius*0.6);
  ctx.strokeStyle = "white";
  ctx.lineWidth=4;
  ctx.stroke();
  ctx.restore();
    ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (Math.PI);

  ctx.rotate (TwoPI*min_pos/pixelCount);
  ctx.beginPath();
  ctx.moveTo(0,0);
  ctx.lineTo(0,innerRadius*0.9);
  ctx.strokeStyle = "white";
  ctx.lineWidth=4;
  ctx.stroke();
  ctx.restore();

}


function parsergb(input) {
  m = input.match(/^rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);
    if( m) {
        return [m[1],m[2],m[3]];
    }

  }

function tick()
{


  /* Check the time */
  curTime = new Date();
  var newTime = curTime.getSeconds();
  newTime = newTime + 0.1 * parseInt (curTime.getMilliseconds / 100);
  
  blendpoint = document.getElementById("blendpoint").value/100;

  
  


  if (newTime != lastSeconds) {
    lastSeconds = curTime.getSeconds();
    needRedraw = true;
  }
  if (needRedraw) {
    needRedraw = false;

      var mins = curTime.getSeconds() % 10 * 6 + curTime.getMilliseconds()/1000*6;
      var hours = curTime.getSeconds()/10 + (6*(curTime.getMinutes()%2)); 

    

    ctx.clearRect(0,0,width,height);
    //draw(ctx, width/6, height/2,hoursColour,white, hours, mins);

    var hour_pos = Math.floor((hours % 12) * pixelCount / 12);
    var min_pos = Math.floor(mins * pixelCount / 60);
    
   
    epiphanyface(hour_pos, min_pos);
    show(ctx, width/2, height/2);
    // if (document.getElementById("showhands").checked) {

    // standardclock(hour_pos, min_pos, ctx, width/2, height/2)
    // };


    




  }
}


function epiphanyface(hour_pos,  min_pos)
{
//this face colours the clock in 2 sections, the c1->c2 divide represents the minute hand and the c2->c1 divide represents the hour hand.
      var c1;
      var c1blend;
      var c2;
      var c2blend;
      var gap;
      var firsthand = Math.min(hour_pos, min_pos);
      var secondhand = Math.max(hour_pos, min_pos);
    //check which hand is first, so we know what colour the 0 pixel is

    if(hour_pos>min_pos){       
        c2 = hourcolor;
        c1 = minutecolor;         
    }
    else
    {
        c1 = hourcolor;
        c2 = minutecolor;
    }

    c1blend = LinearBlend(c1, c2, blendpoint);

    
    c2blend = LinearBlend(c2, c1, blendpoint);


    gap = secondhand - firsthand;

    //create the blend between first and 2nd hand
    for(i=firsthand; i<secondhand; i++){
      clock[i] = LinearBlend(c2blend, c2, (i-firsthand)/gap);    
    }
    gap = pixelCount - gap;
    //and the last hand
    for(i=secondhand; i<pixelCount+firsthand; i++){
      clock[i%pixelCount] = LinearBlend(c1blend, c1, (i-secondhand)/gap);

    }
    //clock.SetPixelColor(hour_pos,hourcolor);
    //clock.SetPixelColor(min_pos,minutecolor);
}



function LinearBlend(left, right, progress)
{   
    var hue;
    var righth = right.getHSV().h;
    var lefth = left.getHSV().h;
    var d = righth-lefth;
    var temp;
    var output;
    var sat;
    var lum;

    //sat = left.getHSV().S + progress*(right.getHSV().S - left.getHSV().S);
    //lum = left.getHSV().L + progress*(right.getHSV().L - left.getHSV().L);

    sat = left.getHSV().s + ((right.getHSV().s - left.getHSV().s) * progress)
    lum = left.getHSV().v + ((right.getHSV().v - left.getHSV().v) * progress)

    if (left.getHSV().s==0||right.getHSV().s==0) // special case, one of the colours is white
    {
        if (left.getHSV().s==0)
        {
            hue = right.getHSV().h;
        } else {
            hue = left.getHSV().h;
        }
    } else {
    if (lefth > righth)
    {
        temp = righth;
        righth = lefth;
        lefth = temp;
        d = -d;
        progress = 1-progress;
    }

    if (d>180)
    {
        lefth += 360;

        hue = (lefth+progress*(righth-lefth));
        if (hue > 360)
        {
            hue -= 360;
        }
    }
    else {
        hue = lefth+progress*d;
    }
  }
    output = new HSVColour(hue, sat, lum);

    
    return output;
}

function manualmode() {
  document.getElementById("manualradio").checked = true;
}

function pad(n) {
  return (n < 10) ? ("0" + n) : n;
}

function middayIsTwelve(n) {
   var x;
    x = n;
    if (n == 0) {x = 12};
    if (n == 12) {x = 0};
    return x;
}

function parseCSScolour(color) {
    var digits = /(.*?)rgb\((\d+), (\d+), (\d+)\)/.exec(color);

    var red = parseInt(digits[2]);
    var green = parseInt(digits[3]);
    var blue = parseInt(digits[4]);

    var output = [red, green, blue];
    return output;
}

function parseHEXcolour(color) {
    var digits = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(color);

    var red = parseInt(digits[1], 16);
    var green = parseInt(digits[2], 16);
    var blue = parseInt(digits[3], 16);

    var output = [red, green, blue];
    return output;
}

//$(function(){
jQuery(document).ready(function($){

  //alert(document.body.style.backgroundColor););
 if (document.getElementById("canvas")) {



  ctx = $("#canvas")[0].getContext('2d');
  $("#hourcolorspectrum").spectrum({
    flat: true,
    preferredFormat: "hex",
    showButtons: false,
    showInput: true,
    move: function(color) {c=color.toRgb(); hourcolor = new RGBColour(c.r, c.g, c.b); 
                           document.getElementById("hourcolor").value=document.getElementById("hourcolorspectrum").value;},
    change: function(color) {document.getElementById("hourcolor").value=document.getElementById("hourcolorspectrum").value;}
  });
    
  $("#minutecolorspectrum").spectrum({
    flat: true,
    preferredFormat: "hex",
    showInput: true,
    showButtons: false,
    //color: minutecolor.getCSSIntegerRGB(),
    move: function(color) {c=color.toRgb(); minutecolor = new RGBColour(c.r, c.g, c.b);
                            document.getElementById("minutecolor").value=document.getElementById("minutecolorspectrum").value;},
    change: function(color) {document.getElementById("minutecolor").value=document.getElementById("minutecolorspectrum").value;}
    
  });

  c = document.getElementById("hourcolor").value;  
  c = parseHEXcolour(c);
  hourcolor = new RGBColour(c[0], c[1], c[2]);

  c = document.getElementById("minutecolor").value;  
  c = parseHEXcolour(c);
  minutecolor = new RGBColour(c[0], c[1], c[2]);


  // Make it visually fill the positioned parent
  canvas.style.width ='100%';
  canvas.style.height='100%';
  // ...then set the internal size to match
  canvas.width  = canvas.offsetWidth;
  canvas.height = canvas.width;

  width = document.getElementById("canvas").width;
  height = (width)+10;
  needRedraw = true;


  /* setup */
  drawRadius = height/2 * 0.95;
  innerRadius = drawRadius * 0.5;
  tick();
  setInterval(tick, 100);
 }
})



