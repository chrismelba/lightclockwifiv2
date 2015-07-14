#include <Time.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>

#define pixelCount 120            //number of pixels in RGB strip

NeoPixelBus strip = NeoPixelBus(pixelCount, 2);  //GPIO 2

RgbColor minutecolor = RgbColor(255, 0, 0);
RgbColor hourcolor = RgbColor(255, 255, 0);
float blendpoint = 0.4;
int prevsecond;
const char* ssid     = "ASIO Secret Base";
const char* password = "pq2macquarie";

IPAddress apIP(192, 168, 1, 1);        //FOR AP mode
IPAddress netMsk(255,255,255,0);         //FOR AP mode

const char* html = "<html><head><style></style></head>"
                   "<body><form action='/' method='GET'><input type='color' name='hourcolor' value='$hourcolor'/><input type='color' name='minutecolor' value='$minutecolor'/>"
                   "<input type='submit' name='submit' value='Update RGB Strip'/><input type='text' name='blendpoint' value='$blendpoint'></form></body></html>";

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  strip.Begin();   
  //strip.setBrightness(50);
  strip.Show();

//***************WIFI client************************//
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//***************WIFI ACCESS POINT******************//
/*  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP,apIP,netMsk);
  WiFi.softAP(ssid);//,password);  //leave password away for open AP
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());   */
//**************************************************//

 
  server.on("/", handle_root);             //root page
  server.onNotFound(handleNotFound);       //page if not found
  server.begin();
  Serial.println("HTTP server started");
  prevsecond =second();
}


void loop() {
  int hour_pos;
  int min_pos;
  server.handleClient();
  hour_pos = (minute() % 12) * pixelCount / 12 + second()/6;
  min_pos = second() * pixelCount / 60;
  delay(50);

  if(second()!=prevsecond) {
    epiphanyface(hour_pos, min_pos);
    strip.Show();
    prevsecond = second();
  }
}

void handleNotFound() {
  Serial.print("\t\t\t\t URI Not Found: ");
  Serial.println(server.uri());
  server.send ( 200,"text/plain","URI Not Found" );
}

void handle_root() {
  String toSend = html;
  if (server.hasArg("hourcolor")) {
    String hourrgbStr = server.arg("hourcolor");  //get value from html5 color element
    hourrgbStr.replace("%23","#"); //%23 = # in URI
    getRGB(hourrgbStr, hourcolor);  

  
  }       
  
  if (server.hasArg("minutecolor")) {
    String minutergbStr = server.arg("minutecolor");  //get value from html5 color element
    minutergbStr.replace("%23","#"); //%23 = # in URI
    getRGB(minutergbStr,minutecolor);                //convert RGB string to rgb ints

  
  }
  if (server.hasArg("blendpoint")) {
    char c[3];
    String blendpointstring = server.arg("blendpoint");  //get value from html5 color element
    Serial.println(blendpointstring);
    blendpointstring.toCharArray(c,4);
    Serial.println(c);
    int blendpointint = atoi(c);  //get value from html5 color element
    Serial.println(blendpointint);
    blendpoint = (float)blendpointint/100;
    Serial.println(String(blendpoint));
  }
  toSend.replace("$minutecolor",rgbToText(minutecolor));
  toSend.replace("$hourcolor",rgbToText(hourcolor));
  toSend.replace("$blendpoint",String(int(blendpoint*100)));
  server.send(200, "text/html", toSend);
  delay(100);
}

void getRGB(String hexRGB, RgbColor &rgb) {
  hexRGB.toUpperCase();
  char c[7]; 
  hexRGB.toCharArray(c,8);
  rgb.R = hexcolorToInt(c[1],c[2]);//red
  rgb.G = hexcolorToInt(c[3],c[4]); //green
  rgb.B = hexcolorToInt(c[5],c[6]); //blue  
}

int hexcolorToInt(char upper,char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal >64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal >64 ? lVal - 55 : lVal - 48;
  //Serial.println(uVal+lVal);
  return uVal + lVal;
}

String rgbToText(RgbColor input) {
  //convert RGB values to #FFFFFF notation. Add in 0s where hexcode would be only a single digit.
  String out;
  out += "#";
  (String(input.R,HEX)).length()==1 ? out+=String(0, HEX) : out+="";
  out += String(input.R, HEX);
  (String(input.G,HEX)).length()==1 ? out+=String(0, HEX) : out+="";
  out += String(input.G, HEX);
  (String(input.B,HEX)).length()==1 ? out+=String(0, HEX) : out+="";
  out += String(input.B, HEX);

  return out;
  
}


void epiphanyface(uint16_t hour_pos, uint16_t min_pos) {
//this face colours the clock in 2 sections, the c1->c2 divide represents the minute hand and the c2->c1 divide represents the hour hand.
      HslColor c1;
      HslColor c1blend;
      HslColor c2;
      HslColor c2blend;
      int gap;
      int firsthand = std::min(hour_pos, min_pos);
      int secondhand = std::max(hour_pos, min_pos);
    //check which hand is first, so we know what colour the 0 pixel is

    if(hour_pos>min_pos){       
        c2 = RgbColor(hourcolor);
        c1 = RgbColor(minutecolor);         
    }
    else
    {
        c1 = RgbColor(hourcolor);
        c2 = RgbColor(minutecolor);         
    }

    c1blend = HslColor::LinearBlend(c1, c2, blendpoint);
    c2blend = HslColor::LinearBlend(c2, c1, blendpoint);

    gap = secondhand - firsthand;

    //create the blend between first and 2nd hand
    for(uint16_t i=firsthand; i<secondhand; i++){
      strip.SetPixelColor(i,HslColor::LinearBlend(c2blend, c2, ((float)i-(float)firsthand)/(float)gap));      
    }
    gap = 120 - gap;
    //and the last hand
    for(uint16_t i=secondhand; i<pixelCount+firsthand; i++){
      strip.SetPixelColor(i%120,HslColor::LinearBlend(c1blend, c1, ((float)i-(float)secondhand)/(float)gap));// [i%120]=HslColor::LinearBlend(c1blend, c1, ((float)i-(float)secondhand)/(float)gap);
    }
    strip.SetPixelColor(hour_pos,hourcolor);
    strip.SetPixelColor(min_pos,minutecolor);
}


