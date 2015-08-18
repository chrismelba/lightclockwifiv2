

/*This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Time.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>
#include <ntp.h>
#include <Ticker.h>
#include "settings.h"
#include "root.h"
#include "timezone.h"
#include "timezonesetup.h"
#include "css.h"
#include "webconfig.h"
#include "importfonts.h"
#include "clearromsure.h"
#include "password.h"
#include "buttongradient.h"
#include "jquery.h"


#define clockPin 4                //GPIO pin that the LED strip is on
#define pixelCount 120            //number of pixels in RGB clock



IPAddress dns(8, 8, 8, 8);  //Google dns
String clientName = "TheLightClock"; //The MQTT ID -> MAC adress will be added to make it kind of unique
String ssid = "The Light Clock"; //The ssid when in AP mode
MDNSResponder mdns;
ESP8266WebServer server(80);
//WiFiUDP UDP;
unsigned int localPort = 2390;      // local port to listen on for magic locator packets
char packetBuffer[255]; //buffer to hold incoming packet
char  ReplyBuffer[] = "I'm a light clock!";       // a string to send back

NeoPixelBus clock = NeoPixelBus(pixelCount, clockPin);  //Clock Led on Pin 4
time_t getNTPtime(void);
NTP NTPclient;
Ticker NTPsyncclock;
WiFiClient DSTclient;

const char* DSTTimeServer = "api.timezonedb.com";

bool DSTchecked = 0;



const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec
int webMode; //decides if we are in setup, normal or local only mode
const int debug = 0; //Set to one to get more log to serial
bool updateTime = true;
unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
int testrun;

//to be read from EEPROM Config
String esid = "";
String epass = "";


float latitude;
float longitude;

RgbColor hourcolor; // starting colour of hour
RgbColor minutecolor; //starting colour of minute

uint8_t blendpoint = 40; //level of default blending
int randommode; //face changes colour every hour
int hourmarks = 1; //where marks should be made (midday/quadrants/12/brianmode)
int sleep = 23; //when the clock should go to night mode
int wake = 7; //when clock should wake again
float timezone = 10; //Australian Eastern Standard Time
bool showseconds = 1; //should the seconds hand tick around
bool DSTauto = 1; //should the clock automatically update for DST



int prevsecond;


IPAddress apIP(192, 168, 1, 1);        //FOR AP mode
IPAddress netMsk(255, 255, 255, 0);      //FOR AP mode



//-----------------------------------standard arduino setup and loop-----------------------------------------------------------------------
void setup() {
  EEPROM.begin(512);
  delay(10);
  Serial.begin(115200);

  clock.Begin();
  logo();
  clock.Show();
  //write a magic byte to eeprom 196 to determine if we've ever booted on this device before
  if (EEPROM.read(500) != 196) {
    //if not load default config files to EEPROM
    writeInitalConfig();
  }

  loadConfig();
  //webMode = 2;
  initWiFi();

  adjustTime(36600);
  delay(1000);
  if (DSTauto == 1) {
    readDSTtime();
  }
  //initialise the NTP clock sync function
  if (webMode == 1) {
    NTPclient.begin("2.au.pool.ntp.org", timezone);
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);
  }
  //UDP.begin(localPort);
  prevsecond = second();

}

void loop() {

  server.handleClient();
  delay(50);
  if (second() != prevsecond) {
    updateface();
    prevsecond = second();
  }
  if (webMode == 1) {
    if (hour() == 5 && DSTchecked == 0 && DSTauto == 1) {
      DSTchecked = 1;
      readDSTtime();
    } else {
      DSTchecked = 0;
    }
  }
}



//--------------------UDP responder functions----------------------------------------------------

//void checkUDP(){
//  //Serial.println("checking UDP");
//  // if there's data available, read a packet
//  int packetSize = UDP.parsePacket();
//  if (packetSize) {
//    Serial.print("Received packet of size ");
//    Serial.println(packetSize);
//    Serial.print("From ");
//    IPAddress remoteIp = UDP.remoteIP();
//    Serial.print(remoteIp);
//    Serial.print(", port ");
//    Serial.println(UDP.remotePort());
//
//    // read the packet into packetBufffer
//    int len = UDP.read(packetBuffer, 255);
//    if (len > 0) {
//      packetBuffer[len] = 0;
//    }
//    Serial.println("Contents:");
//    Serial.println(packetBuffer);
//
//    // send a reply, to the IP address and port that sent us the packet we received
//    UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
//    UDP.write(ReplyBuffer);
//    UDP.endPacket();
//  }
//}


//--------------------EEPROM initialisations-----------------------------------------------------
void loadConfig() {
  Serial.println("reading settings from EEPROM");
  //Tries to read ssid and password from EEPROM
  EEPROM.begin(512);
  delay(10);

  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);


  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  loadFace(1);
  latitude = readLatLong(175);
  Serial.print("latitude: ");
  Serial.println(latitude);
  longitude = readLatLong(177);
  Serial.print("longitude: ");
  Serial.println(longitude);
  timezone = EEPROM.read(179);
  Serial.print("timezone: ");
  Serial.println(timezone);
  randommode = EEPROM.read(180);
  Serial.print("randommode: ");
  Serial.println(randommode);
  hourmarks = EEPROM.read(181);
  Serial.print("hourmarks: ");
  Serial.println(hourmarks);
  sleep = EEPROM.read(182);
  Serial.print("sleep: ");
  Serial.println(sleep);
  wake = EEPROM.read(183);
  Serial.print("wake: ");
  Serial.println(wake);
  showseconds = EEPROM.read(184);
  Serial.print("showseconds: ");
  Serial.println(showseconds);
  DSTauto = EEPROM.read(185);
  Serial.print("DSTauto: ");
  Serial.println(DSTauto);
  webMode = EEPROM.read(186);
  Serial.print("webMode: ");
  Serial.println(webMode);

}

void writeInitalConfig() {
  Serial.println("can't find settings so writing defaults");
  EEPROM.begin(512);
  delay(10);
  writeLatLong(-36.1214, 175); //default to wodonga
  writeLatLong(146.881, 177);//default to wodonga
  EEPROM.write(179, 10);//timezone default AEST
  EEPROM.write(180, 0);//default randommode off
  EEPROM.write(181, 0); //default hourmarks to off
  EEPROM.write(182, 23); //default to sleep at 23:00
  EEPROM.write(183, 7); //default to wake at 7:00
  EEPROM.write(184, 1); //default to showseconds to yes
  EEPROM.write(185, 0); //default DSTauto off until user sets lat/long
  EEPROM.write(186, 0); //default webMode to setup mode off until user sets local wifi
  EEPROM.write(500, 196);//write magic byte to 500 so that system knows its set up.

  EEPROM.commit();
  delay(500);

  //face 1 defaults
  hourcolor = RgbColor(255, 255, 0);
  minutecolor = RgbColor(0, 57, 255);
  blendpoint = 40;
  saveFace(1);
  //face 2 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(0, 0, 255);
  blendpoint = 30;
  saveFace(2);
  //face 3 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(255, 255, 0);
  blendpoint = 50;
  saveFace(3);

}




void initWiFi() {
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  esid.trim();
  if (webMode ==2){
    WiFi.mode(WIFI_AP);
    WiFi.softAP((char*) ssid.c_str());
    WiFi.begin((char*) ssid.c_str()); // not sure if need but works
    Serial.print("Access point started with name ");
    Serial.println(ssid);
    launchWeb(2);
    return;
    
  }
  if (webMode == 1) {
    // test esid
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_STA);
    Serial.print("Connecting to WiFi ");
    Serial.println(esid);
    WiFi.begin(esid.c_str(), epass.c_str());
    if ( testWifi() == 20 ) {
      launchWeb(1);
      return;
    }
  }
  setupAP();
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return (20);
    }
    delay(500);
    Serial.print(".");
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return (10);
}

void setupAP(void) {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    st = "<label><input type='radio' name='ssid' value='No networks found' onClick='regularssid()'>No networks found</input></label><br>";
  } else {
    Serial.print(n);
    Serial.println("Networks found");
    //st = "<ul>";
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " (OPEN)" : "*");

      // Print to web SSID and RSSI for each network found
      st += "<label><input type='radio' name='ssid' value='";
      st += WiFi.SSID(i);
      st += "' onClick='regularssid()'>";
      st += i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " (OPEN)" : "*";
      st += "</input></label><br>";
      delay(10);
    }
    //st += "</ul>";
  }
  Serial.println("");
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  //Build SSID
  //uint8_t mac[6];
  //WiFi.macAddress(mac);
  //ssid += "-";
  //ssid += macToStr(mac);

  WiFi.softAP((char*) ssid.c_str());
  //WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  Serial.print("Access point started with name ");
  Serial.println(ssid);
  launchWeb(0);
}

//------------------------------------------------------Web handle sections-------------------------------------------------------------------
void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  switch(webtype) {
    case 0:
      webMode == 0;
      Serial.println(WiFi.softAPIP());
      server.on("/", webHandleConfig);
      server.on("/a", webHandleConfigSave);
      server.on("/timezonesetup", webHandleTimeZoneSetup);
      server.on("/passwordinput", webHandlePassword);
    break;
    
    case 1:
       //setup DNS since we are a client in WiFi net
      if (!mdns.begin("thelightclock")) {
        Serial.println("Error setting up MDNS responder!");
        while (1) {
          delay(1000);
        }
      } else {
        Serial.println("mDNS responder started");
      }
      Serial.println(WiFi.localIP());
      server.on("/", handleRoot);
      server.on("/cleareeprom", webHandleClearRom);
      server.on("/cleareepromsure", webHandleClearRomSure);
      server.on("/settings", handleSettings);
      server.on("/timezone", handleTimezone);
    break;

    case 2: 
      server.on("/", handleRoot);
      server.on("/cleareeprom", webHandleClearRom);
      server.on("/cleareepromsure", webHandleClearRomSure);
      server.on("/settings", handleSettings);
      server.on("/timezone", handleTimezone);
    break;
      
  }
  if (webtype == 0) {
    

  } else {

  }
  
  //server.onNotFound(webHandleRoot);
  server.begin();
  Serial.println("Web server started");
  webMode = webtype; //Store global to use in loop()
}

void webHandleConfig() {
  Serial.println("Sending webHandleConfig");
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String s;

  String toSend = webconfig_html;
  toSend.replace("$css", css_file);
  toSend.replace("$ssids", st);

  server.send(200, "text/html", toSend);
}

void webHandlePassword() {
  Serial.println("Sending webHandlePassword");

  
  String toSend = password_html;
  toSend.replace("$css", css_file);
  
  server.send(200, "text/html", toSend);

   String qsid;
  if (server.arg("ssid") == "other") {
    qsid = server.arg("other");
  } else {
    qsid = server.arg("ssid");
  }
  qsid.replace("%2F", "/");
  qsid.replace("+", " ");
  Serial.println(qsid);
  Serial.println("");
  Serial.println("clearing old ssid.");
  clearssid();
  EEPROM.begin(512);
  delay(10);
  Serial.println("writing eeprom ssid.");
  //addr += EEPROM.put(addr, qsid);
  for (int i = 0; i < qsid.length(); ++i)
  {
    EEPROM.write(i, qsid[i]);
    Serial.print(qsid[i]);
  }
  Serial.println("");
  EEPROM.commit();
  delay(1000);
  EEPROM.end();

}

void webHandleTimeZoneSetup() {
  Serial.println("Sending webHandleTimeZoneSetup");
  String toSend = timezonesetup_html;
  toSend.replace("$css", css_file);
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$latitude", String(latitude));
  toSend.replace("$longitude", String(longitude));

  server.send(200, "text/html", toSend);

  Serial.println("clearing old pass.");
  clearpass();
 

  String qpass;
  qpass = server.arg("pass");
  qpass.replace("%2F", "/");
  Serial.println(qpass);
  Serial.println("");

  //int addr=0;
  EEPROM.begin(512);
  delay(10);


  Serial.println("writing eeprom pass.");
  //addr += EEPROM.put(addr, qpass);
  for (int i = 0; i < qpass.length(); ++i)
  {
    EEPROM.write(32 + i, qpass[i]);
    Serial.print(qpass[i]);
  }
  Serial.println("");
  EEPROM.write(186, 1);

  EEPROM.commit();
  delay(1000);
  EEPROM.end();

}

void webHandleConfigSave() {
  Serial.println("Sending webHandleConfigSave");
  // /a?ssid=blahhhh&pass=poooo
  String s;
  s = "<p>Settings saved to memeory now resetting to boot into new settings</p>\r\n\r\n";
  server.send(200, "text/html", s);
  EEPROM.begin(512);
  if (server.hasArg("timezone")) {
    String timezonestring = server.arg("timezone");
    timezone = timezonestring.toInt();//atoi(c);

    EEPROM.write(179, timezone);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }


  if (server.hasArg("latitude")) {
    String latitudestring = server.arg("latitude");  //get value from blend slider
    latitude = latitudestring.toInt();//atoi(c);  //get value from html5 color element
    writeLatLong(175, latitude);
  }
  if (server.hasArg("longitude")) {
    String longitudestring = server.arg("longitude");  //get value from blend slider
    longitude = longitudestring.toInt();//atoi(c);  //get value from html5 color element
    writeLatLong(177, longitude);
    DSTauto = 1;
    EEPROM.write(185, 1);
    EEPROM.write(179, timezone);


  }
  EEPROM.commit();
  delay(1000);
  EEPROM.end();
  Serial.println("Settings written, restarting!");
  ESP.reset();
}

void handleNotFound() {
  Serial.println("Sending handleNotFound");
  Serial.print("\t\t\t\t URI Not Found: ");
  Serial.println(server.uri());
  server.send ( 200, "text/plain", "URI Not Found" );
}

void handleRoot() {
  EEPROM.begin(512);
  String fontreplace;
  String tempgradient = "";
  String csswgradient = css_file;
  const String scheme = "scheme";
  RgbColor tempcolor; 
  HslColor tempcolorHsl; 
  
  for(int i = 1; i < 4; i++){
    //loop makes each of the save/load buttons coloured based on the scheme
    tempgradient = buttongradient_css;
    //load hour color
    tempcolor.R = EEPROM.read(75 + i * 25);
    tempcolor.G = EEPROM.read(76 + i * 25);
    tempcolor.B = EEPROM.read(77 + i * 25);
    //fix darkened colour schemes by manually lightening them. 
    tempcolorHsl = tempcolor;
    tempcolorHsl.L = 0.5;
    tempcolor=tempcolorHsl;
    tempgradient.replace("$hourcolor", rgbToText(tempcolor));
    //load minute color
    tempcolor.R = EEPROM.read(78 + i * 25);
    tempcolor.G = EEPROM.read(79 + i * 25);
    tempcolor.B = EEPROM.read(80 + i * 25);
    tempcolorHsl = tempcolor;
    tempcolorHsl.L = 0.5;
    tempcolor=tempcolorHsl;
    tempgradient.replace("$minutecolor", rgbToText(tempcolor));

    tempgradient.replace("$scheme", scheme+i);

    csswgradient += tempgradient;
    
    
  }
    
  if(webMode == 1){fontreplace=importfonts;} else {fontreplace="";}
  Serial.println("Sending handleRoot");
  String toSend = root_html;
  toSend.replace("$jquery", "<script src='//ajax.googleapis.com/ajax/libs/jquery/1.8.3/jquery.min.js'></script>");
  toSend.replace("$css", csswgradient);
  toSend.replace("$fonts", fontreplace);
  
  //Check for all the potential incoming arguments
  if (server.hasArg("hourcolor")) {
    String hourrgbStr = server.arg("hourcolor");  //get value from html5 color element
    hourrgbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(hourrgbStr, hourcolor);
  }

  if (server.hasArg("minutecolor")) {
    String minutergbStr = server.arg("minutecolor");  //get value from html5 color element
    minutergbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(minutergbStr, minutecolor);               //convert RGB string to rgb ints
  }
  if (server.hasArg("blendpoint")) {
    String blendpointstring = server.arg("blendpoint");  //get value from blend slider
    blendpoint = blendpointstring.toInt();//atoi(c);  //get value from html5 color element

  }

  if (server.hasArg("hourmarks")) {
    String hourmarksstring = server.arg("hourmarks");  //get value from blend slider
    hourmarks = hourmarksstring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(181, hourmarks);
  }
  if (server.hasArg("sleep")) {
    String sleepstring = server.arg("sleep");  //get value from blend slider
    sleep = sleepstring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(182, sleep);
  }
  if (server.hasArg("wake")) {
    String wakestring = server.arg("wake");  //get value from blend slider
    wake = wakestring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(183, wake);
  }
  if (server.hasArg("timezone")) {
    int oldtimezone = timezone;
    String timezonestring = server.arg("timezone");
    timezone = timezonestring.toInt();//atoi(c);
    NTPclient.updateTimeZone(timezone);
    //setTime(NTPclient.getNtpTime());
    adjustTime((timezone-oldtimezone)*3600);
    EEPROM.write(179, timezone);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }


  if (server.hasArg("latitude")) {
    String latitudestring = server.arg("latitude");  //get value from blend slider
    latitude = latitudestring.toInt();//atoi(c);  //get value from html5 color element
    writeLatLong(175, latitude);
  }
  if (server.hasArg("longitude")) {
    String longitudestring = server.arg("longitude");  //get value from blend slider
    longitude = longitudestring.toInt();//atoi(c);  //get value from html5 color element
    writeLatLong(177, longitude);
    DSTauto = 1;
    EEPROM.write(185, 1);
    readDSTtime();
    EEPROM.write(179, timezone);


  }


  if (server.hasArg("showsecondshidden")) {
    showseconds = server.hasArg("showseconds");
    EEPROM.write(184, showseconds);
  }

  if (server.hasArg("submit")) {
    String memoryarg = server.arg("submit");
    Serial.println(memoryarg);
    Serial.println(server.arg("submit"));
    String saveloadmode = memoryarg.substring(5, 11);
    Serial.println(saveloadmode);
    if (saveloadmode == "Scheme") {

      String saveload = memoryarg.substring(0, 4);
      String location = memoryarg.substring(12);
      if (saveload == "Save") {
        saveFace(location.toInt());
      } else {
        loadFace(location.toInt());
      }
    }
  }

  toSend.replace("$minutecolor", rgbToText(minutecolor));
  toSend.replace("$hourcolor", rgbToText(hourcolor));
  toSend.replace("$blendpoint", String(int(blendpoint)));
  server.send(200, "text/html", toSend);
  EEPROM.commit();
  delay(300);
}


void handleSettings() {
  String fontreplace;
  if(webMode == 1){fontreplace=importfonts;} else {fontreplace="";}
  Serial.println("Sending handleSettings");
  String toSend = settings_html;
  for (int i = 0; i < 5; i++) {
    if (i == hourmarks) {
      toSend.replace("$hourmarks" + String(i), "selected");
    } else {
      toSend.replace("$hourmarks" + String(i), "");
    }
  }
  toSend.replace("$css", css_file);
  toSend.replace("$fonts", fontreplace);
  String ischecked;
  showseconds ? ischecked = "checked" : ischecked = "";
  toSend.replace("$showseconds", ischecked);
  toSend.replace("$sleep", String(sleep));
  toSend.replace("$wake", String(wake));


  server.send(200, "text/html", toSend);

}

void handleTimezone() {
    String fontreplace;
  if(webMode == 1){fontreplace=importfonts;} else {fontreplace="";}
  String toSend = timezone_html;
  toSend.replace("$css", css_file);
  toSend.replace("$fonts", fontreplace);
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$latitude", String(latitude));
  toSend.replace("$longitude", String(longitude));


  server.send(200, "text/html", toSend);
  
  Serial.println("Sending handleTimezone");
}


void webHandleClearRom() {
  String s;
  s = "<p>Clearing the EEPROM and reset to configure new wifi<p>";
  s += "</html>\r\n\r\n";
  Serial.println("Sending webHandleClearRom");
  server.send(200, "text/html", s);
  Serial.println("clearing eeprom");
  clearEEPROM();
  delay(10);
  Serial.println("Done, restarting!");
  ESP.reset();
}


void webHandleClearRomSure() {
  String toSend = clearromsure_html;
  toSend.replace("$css", css_file);
  Serial.println("Sending webHandleClearRomSure");
  server.send(200, "text/html", toSend);
}

//-------------------------text input conversion functions---------------------------------------------

void getRGB(String hexRGB, RgbColor &rgb) {
  hexRGB.toUpperCase();
  char c[7];
  hexRGB.toCharArray(c, 8);
  rgb.R = hexcolorToInt(c[1], c[2]); //red
  rgb.G = hexcolorToInt(c[3], c[4]); //green
  rgb.B = hexcolorToInt(c[5], c[6]); //blue
}

int hexcolorToInt(char upper, char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal > 64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal > 64 ? lVal - 55 : lVal - 48;
  //Serial.println(uVal+lVal);
  return uVal + lVal;
}

String rgbToText(RgbColor input) {
  //convert RGB values to #FFFFFF notation. Add in 0s where hexcode would be only a single digit.
  String out;
  out += "#";
  (String(input.R, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.R, HEX);
  (String(input.G, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.G, HEX);
  (String(input.B, HEX)).length() == 1 ? out += String(0, HEX) : out += "";
  out += String(input.B, HEX);

  return out;

}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
//------------------------------------------------animating functions-----------------------------------------------------------

void updateface() {

  int hour_pos;
  int min_pos;
  switch (testrun) {
    case 0:
      // no testing
      hour_pos = (hour() % 12) * pixelCount / 12 + minute() / 6;
      min_pos = minute() * pixelCount / 60;

      break;
    case 1:
      //set the face to tick ever second rather than every minute
      hour_pos = (minute() % 12) * pixelCount / 12 + second() / 6;
      min_pos = second() * pixelCount / 60;

      break;
    case 2:
      //set the face to the classic 10 past 10 for photos
      hour_pos = 10 * pixelCount / 12;
      min_pos = 10 * pixelCount / 60;
  }

  if (hour() >= sleep || hour() < wake) {
    nightface(hour_pos, min_pos);
  } else {
    face(hour_pos, min_pos);
    switch (hourmarks) {
      case 0:
        break;
      case 1:
        showMidday();
        break;
      case 2:
        showQuadrants();
        break;
      case 3:
        showHourMarks();
        break;
      case 4:
        darkenToMidday(hour_pos, min_pos);
    }
    //only show seconds in "day mode"
    if (showseconds) {
      invertLED(second()*pixelCount / 60);
    }
  }



  clock.Show();

}

void face(uint16_t hour_pos, uint16_t min_pos) {
  //this face colours the clock in 2 sections, the c1->c2 divide represents the minute hand and the c2->c1 divide represents the hour hand.
  HslColor c1;
  HslColor c1blend;
  HslColor c2;
  HslColor c2blend;
  int gap;
  int firsthand = std::min(hour_pos, min_pos);
  int secondhand = std::max(hour_pos, min_pos);
  //check which hand is first, so we know what colour the 0 pixel is

  if (hour_pos > min_pos) {
    c2 = RgbColor(hourcolor);
    c1 = RgbColor(minutecolor);
  }
  else
  {
    c1 = RgbColor(hourcolor);
    c2 = RgbColor(minutecolor);
  }
  // the blending is the colour that the hour/minute colour will meet. The greater the blend, the closer to the actual hour/minute colour it gets.
  c1blend = HslColor::LinearBlend(c1, c2, (float)blendpoint / 100);
  c2blend = HslColor::LinearBlend(c2, c1, (float)blendpoint / 100);

  gap = secondhand - firsthand;

  //create the blend between first and 2nd hand
  for (uint16_t i = firsthand; i < secondhand; i++) {
    clock.SetPixelColor(i, HslColor::LinearBlend(c2blend, c2, ((float)i - (float)firsthand) / (float)gap));
  }
  gap = 120 - gap;
  //and the last hand
  for (uint16_t i = secondhand; i < pixelCount + firsthand; i++) {
    clock.SetPixelColor(i % 120, HslColor::LinearBlend(c1blend, c1, ((float)i - (float)secondhand) / (float)gap)); // [i%120]=HslColor::LinearBlend(c1blend, c1, ((float)i-(float)secondhand)/(float)gap);
  }
  clock.SetPixelColor(hour_pos, hourcolor);
  clock.SetPixelColor(min_pos, minutecolor);
}

void nightface(uint16_t hour_pos, uint16_t min_pos) {
  for (int i = 0; i < pixelCount; i++) {
    clock.SetPixelColor(i, 0, 0, 0);
  }
  clock.SetPixelColor(hour_pos, hourcolor);
  clock.SetPixelColor(min_pos, minutecolor);

}

void invertLED(int i) {
  //This function will set the LED to in inverse of the two LEDs next to it showing as white on the main face
  RgbColor averagecolor;
  averagecolor = RgbColor::LinearBlend(clock.GetPixelColor((i - 1) % pixelCount), clock.GetPixelColor((i + 1) % pixelCount), 0.5);
  averagecolor = RgbColor(255 - averagecolor.R, 255 - averagecolor.G, 255 - averagecolor.B);
  clock.SetPixelColor(i, averagecolor);
}

void showHourMarks() {
  //shows white at the four quadrants and darkens each hour mark to help the user tell the time
  RgbColor c;
  for (int i = 0; i < 12; i++) {
    c = clock.GetPixelColor(i);
    c.Darken(255);
    clock.SetPixelColor(i * pixelCount / 12, c);
  }

  for (int i = 0; i < 4; i++) {
    invertLED(i * pixelCount / 4);
  }
}

void showQuadrants() {
  //shows white at each of the four quadrants to orient the user
  for (int i = 0; i < 4; i++) {
    invertLED(i * pixelCount / 4);
  }
}

void showMidday() {
  //shows a bright light at midday to orient the user
  invertLED(0);
}

void darkenToMidday(uint16_t hour_pos, uint16_t min_pos) {
  //darkens the pixels between the second hand and midday because Brian suggested it.
  int secondhand = std::max(hour_pos, min_pos);
  RgbColor c;
  for (uint16_t i = secondhand; i < pixelCount; i++) {
    c = clock.GetPixelColor(i);
    c.Darken(240);
    clock.SetPixelColor(i, c);
  }
}

void logo() {
  //this lights up the clock as the C logo
  //yellow section
  for (int i = 14 / (360 / pixelCount); i < 48 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 255, 255, 0);
  }

  //blank section
  for (int i = 48 / (360 / pixelCount); i < 140 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 0, 0, 0);
  }

  //blue section
  for (int i = 140 / (360 / pixelCount); i < 296 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 0, 120, 255);
  }

  //green section
  for (int i = 296 / (360 / pixelCount); i < (360 + 14) / (360 / pixelCount); i++) {
    clock.SetPixelColor(i % pixelCount, 60, 255, 0);
  }

  clock.Show();
}

//------------------------------EEPROM save/read functions-----------------------

void writeLatLong(int partition, float latlong) {
  int val = (int16_t)(latlong * 182);

  EEPROM.write(partition, (val & 0xff));
  EEPROM.write(partition + 1, ((val >> 8) & 0xff));

}

float readLatLong(int partition) {
  EEPROM.begin(512);
  delay(10);
  int16_t val = EEPROM.read(partition) | (EEPROM.read(partition + 1) << 8);

  return (float)val / 182;
}

void saveFace(uint8_t partition)
{
  if (partition > 0 && partition < 4) { // only 3 locations for saved faces. Don't accidentally overwrite other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color

    EEPROM.write(75 + partition * 25, hourcolor.R);
    EEPROM.write(76 + partition * 25, hourcolor.G);
    EEPROM.write(77 + partition * 25, hourcolor.B);


    //write the minute color
    EEPROM.write(78 + partition * 25, minutecolor.R);
    EEPROM.write(79 + partition * 25, minutecolor.G);
    EEPROM.write(80 + partition * 25, minutecolor.B);


    //write the blend point
    EEPROM.write(81 + partition * 25, blendpoint);

    EEPROM.commit();
    delay(500);
  }
}


void clearEEPROM() {
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();
}


void clearssid() {
  EEPROM.begin(512);
  // write a 0 to ssid and pass bytes of the EEPROM
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();

}
void clearpass() {
  EEPROM.begin(512);
  // write a 0 to ssid and pass bytes of the EEPROM
  for (int i = 32; i < 96; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  EEPROM.commit();
  EEPROM.end();

}
void loadFace(uint8_t partition)
{
  if (partition > 0 && partition < 4) { // only 3 locations for saved faces. Don't accidentally read/write other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color
    hourcolor.R = EEPROM.read(75 + partition * 25);
    hourcolor.G = EEPROM.read(76 + partition * 25);
    hourcolor.B = EEPROM.read(77 + partition * 25);

    //write the minute color
    minutecolor.R = EEPROM.read(78 + partition * 25);
    minutecolor.G = EEPROM.read(79 + partition * 25);
    minutecolor.B = EEPROM.read(80 + partition * 25);

    //write the blend point
    blendpoint = EEPROM.read(81 + partition * 25);
  }
}



//------------------------------NTP Functions---------------------------------


time_t getNTPtime(void)
{
  time_t newtime;
  newtime = NTPclient.getNtpTime();
  for (int i = 0; i < 5; i++) {
    if (newtime == 0) {
      Serial.println("Failed NTP Attempt" + i);
      delay(2000);
      newtime = NTPclient.getNtpTime();
    }
  }

  return newtime;
}


//----------------------------------------DST adjusting functions------------------------------------------------------------------
void connectToDSTServer() {
  String GETString;
  // attempt to connect, and wait a millisecond:

  Serial.println("Connecting to DST server");
  DSTclient.connect("api.timezonedb.com", 80);

  if (DSTclient.connect("api.timezonedb.com", 80)) {
    // make HTTP GET request to timezonedb.com:
    GETString += "GET /?lat=";
    GETString += latitude;
    GETString += "&lng=";
    GETString += longitude;
    GETString += "&key=N9XTPTVFZJFN HTTP/1.1";

    DSTclient.println(GETString);
    Serial.println(GETString);
    DSTclient.println("Host: api.timezonedb.com");
    Serial.println("Host: api.timezonedb.com");
    DSTclient.println("Connection: close\r\n");
    Serial.println("Connection: close\r\n");
    //DSTclient.print("Accept-Encoding: identity\r\n");
    //DSTclient.print("Host: api.geonames.org\r\n");
    //DSTclient.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");
    //DSTclient.print("Connection: close\r\n\r\n");

    int i = 0;
    while ((!DSTclient.available()) && (i < 1000)) {
      delay(10);
      i++;
    }
  }
}

void readDSTtime() {
  float oldtimezone = timezone;
  String currentLine = "";
  bool readingUTCOffset = false;
  String UTCOffset;
  connectToDSTServer();
  Serial.print("DST.connected: ");
  Serial.println(DSTclient.connected());
  Serial.print("DST.available: ");
  Serial.println(DSTclient.available());

  while (DSTclient.connected()) {
    if (DSTclient.available()) {

      // read incoming bytes:
      char inChar = DSTclient.read();
      // add incoming byte to end of line:
      currentLine += inChar;

      // if you're currently reading the bytes of a UTC offset,
      // add them to the UTC offset String:
      if (readingUTCOffset) {//the section below has flagged that we're getting the UTC offset from server here
        if (inChar != '<') {
          UTCOffset += inChar;
        }
        else {
          // if you got a "<" character,
          // you've reached the end of the UTC offset:
          readingUTCOffset = false;
          Serial.print("UTC Offset in seconds: ");
          Serial.println(UTCOffset);
          //update the internal time-zone
          timezone = UTCOffset.toInt() / 3600;
          adjustTime((timezone-oldtimezone)*3600);
          NTPclient.updateTimeZone(timezone);
          //setTime(NTPclient.getNtpTime());

          // close the connection to the server:
          DSTclient.stop();
        }
      }

      // if you get a newline, clear the line:
      if (inChar == '\n') {

        Serial.println(currentLine);
        currentLine = "";
      }
      // if the current line ends with <text>, it will
      // be followed by the tweet:
      if ( currentLine.endsWith("<gmtOffset>")) {
        // UTC offset is beginning. Clear the tweet string:

        Serial.println(currentLine);
        readingUTCOffset = true;
        UTCOffset = "";
      }


    }
  }
}
