



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

//#include <ESP8266WebServer.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
//#include <Adafruit_NeoPixel.h>
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
#include "externallinks.h"
#include "spectrumcss.h"
#include "send_progmem.h"
#include "colourjs.h"
#include "clockjs.h"
#include "spectrumjs.h"
#include "alarm.h"
#include <ESP8266HTTPUpdateServer.h>

#define clockPin 4                //GPIO pin that the LED strip is on
#define pixelCount 120            //number of pixels in RGB clock

byte mac[6]; // MAC address
String macString;
String ipString;
String netmaskString;
String gatewayString;
String clockname = "thelightclock";

IPAddress dns(8, 8, 8, 8);  //Google dns
const char* ssid = "The Light Clock"; //The ssid when in AP mode
MDNSResponder mdns;
ESP8266WebServer server(80);
//ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
const byte DNS_PORT = 53;
//WiFiUDP UDP;
unsigned int localPort = 2390;      // local port to listen on for magic locator packets
char packetBuffer[255]; //buffer to hold incoming packet
char  ReplyBuffer[] = "I'm a light clock!";       // a string to send back

NeoPixelBus clock = NeoPixelBus(pixelCount, clockPin);  //Clock Led on Pin 4
time_t getNTPtime(void);
NTP NTPclient;
Ticker NTPsyncclock;
WiFiClient DSTclient;
Ticker alarmtick;
int alarmprogress = 0;

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
int brightness = 50; // a variable to dim the over-all brightness of the clock

uint8_t blendpoint = 40; //level of default blending
int randommode; //face changes colour every hour
int hourmarks = 1; //where marks should be made (midday/quadrants/12/brianmode)
int sleep = 22; //when the clock should go to night mode
int sleepmin = 0; //when the clock should go to night mode
int wake = 7; //when clock should wake again
int wakemin = 0; //when clock should wake again
int nightmode = 0;
unsigned long lastInteraction;

float timezone = 10; //Australian Eastern Standard Time
int timezonevalue;
int DSTtime; //add one if we're in DST
bool showseconds; //should the seconds hand tick around
bool DSTauto; //should the clock automatically update for DST
int alarmmode = 0;



int prevsecond;
int hourofdeath; //saves the time incase of an unplanned reset
int minuteofdeath; //saves the time incase of an unplanned reset



//-----------------------------------standard arduino setup and loop-----------------------------------------------------------------------
void setup() {
  
  httpUpdater.setup(&server);
  EEPROM.begin(512);
  delay(10);
  Serial.begin(115200);

  clock.Begin();
 
  //write a magic byte to eeprom 196 to determine if we've ever booted on this device before
  if (EEPROM.read(500) != 196) {
    //if not load default config files to EEPROM
    writeInitalConfig();
  }

  loadConfig();
  nightCheck();
  updateface();
  
  initWiFi();
  lastInteraction = millis();
  //adjustTime(36600);
  delay(1000);
  if (DSTauto == 1) {
    readDSTtime();
  }
  //initialise the NTP clock sync function
  if (webMode == 1) {
    NTPclient.begin("2.au.pool.ntp.org", timezone+DSTtime);
    setSyncInterval(SECS_PER_HOUR);
    setSyncProvider(getNTPtime);

    macString = String(WiFi.macAddress());
    ipString = StringIPaddress(WiFi.localIP());
    netmaskString = StringIPaddress(WiFi.subnetMask());
    gatewayString = StringIPaddress(WiFi.gatewayIP());
  }
  //UDP.begin(localPort);
  prevsecond = second();// initalize prev second for main loop

  //update sleep/wake to current
  nightCheck();


}

void loop() {
  if(webMode == 0) {
    //initiate web-capture mode
    dnsServer.processNextRequest();
  }
  if(webMode == 0 && millis()-lastInteraction > 300000) {
    lastInteraction = millis(); 

      ESP.reset();

    
  }
  server.handleClient();
  delay(50);
  if (second() != prevsecond) {
    EEPROM.begin(512);
    delay(10);
    EEPROM.write(193, hour()); 
    EEPROM.write(194, minute()); 
    EEPROM.commit();
    delay(200); // this section of code will save the "time of death" to the clock so if it unexpectedly resets it should be seemless to the user.
    
    if (second() == 0) {
      
      if(hour() == sleep && minute() == sleepmin){
        nightmode = 1;
      }
      if(hour() == wake && minute() == wakemin){
        nightmode = 0;
      }
    }
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

  clockname = "";
  for (int i = 195; i < 228; ++i)
  {
    clockname += char(EEPROM.read(i));
  }
  clockname = clockname.c_str();
  Serial.print("clockname: ");
  Serial.println(clockname);

  
  loadFace(0);
  latitude = readLatLong(175);
  Serial.print("latitude: ");
  Serial.println(latitude);
  longitude = readLatLong(177);
  Serial.print("longitude: ");
  Serial.println(longitude);
  timezonevalue = EEPROM.read(179);
  Serial.print("timezonevalue: ");
  Serial.println(timezonevalue);
  interpretTimeZone(timezonevalue);
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
  sleepmin = EEPROM.read(183);
  Serial.print("sleepmin: ");
  Serial.println(sleepmin);
  showseconds = EEPROM.read(184);
  Serial.print("showseconds: ");
  Serial.println(showseconds);
  DSTauto = EEPROM.read(185);
  Serial.print("DSTauto: ");
  Serial.println(DSTauto);
  webMode = EEPROM.read(186);
  Serial.print("webMode: ");
  Serial.println(webMode);
  wake = EEPROM.read(189);
  Serial.print("wake: ");
  Serial.println(wake);
  wakemin = EEPROM.read(190);
  Serial.print("wakemin: ");
  Serial.println(wakemin);
  brightness = EEPROM.read(191);
  Serial.print("brightness: ");
  Serial.println(brightness);
  DSTtime = EEPROM.read(192);
  Serial.print("DST (true/false): ");
  Serial.println(DSTtime);
  hourofdeath = EEPROM.read(193);
  Serial.print("Hour of Death: ");
  Serial.println(hourofdeath);
  minuteofdeath = EEPROM.read(194);
  Serial.print("minuteofdeath: ");
  Serial.println(minuteofdeath);
  setTime(hourofdeath, minuteofdeath, 0, 0,0, 0);
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
  EEPROM.write(182, 22); //default to sleep at 22:00
  EEPROM.write(183, 0);
  EEPROM.write(184, 1); //default to showseconds to yes
  EEPROM.write(185, 0); //default DSTauto off until user sets lat/long
  EEPROM.write(186, 0); //default webMode to setup mode off until user sets local wifi
  EEPROM.write(500, 196);//write magic byte to 500 so that system knows its set up.
  EEPROM.write(189, 7); 
  EEPROM.write(190, 0); //default to wake at 7:00
  EEPROM.write(191, 100); //default to full brightness
  EEPROM.write(192, 0); //default no daylight savings
  EEPROM.write(193, 10); //default "hour of death" is 10am
  for (int i = 195; i < 228; i++) {//zero (instead of null) the values where clockname will be written.
     EEPROM.write(i, 0);
  }
  EEPROM.write(194, 10); //default "minute of death" is 10am
   for (int i = 0; i < clockname.length(); ++i){
    EEPROM.write(195+i, clockname[i]);
    Serial.print(clockname[i]);
  }

  
  
  
  
  

  EEPROM.commit();
  delay(500);

  //face 1 defaults
  hourcolor = RgbColor(255, 255, 0);
  minutecolor = RgbColor(0, 57, 255);
  blendpoint = 40;
  saveFace(0);
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
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid);
//    WiFi.begin((char*) ssid.c_str()); // not sure if need but works
    //dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    //dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("USP Server started");
    Serial.print("Access point started with name ");
    Serial.println(ssid);
    //server.on("/generate_204", handleRoot);  //Android captive 
    server.onNotFound(handleRoot);
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
  logo();
  clock.Show();
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
    st = "";
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
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid);
    //WiFi.begin((char*) ssid.c_str()); // not sure if need but works
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("USP Server started");
    Serial.print("Access point started with name ");
    Serial.println(ssid);
  //WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  Serial.print("Access point started with name ");
  Serial.println(ssid);
  launchWeb(0);
}

//------------------------------------------------------Web handle sections-------------------------------------------------------------------
void launchWeb(int webtype) {
  webMode = webtype;
  int clockname_len = clockname.length() + 1; 
  char clocknamechar[clockname_len];
  Serial.println("");
  Serial.println("WiFi connected");
  switch(webtype) {
    case 0:
    //set up wifi network to connect to since we are in setup mode.
      webMode == 0;
      Serial.println(WiFi.softAPIP());
      server.on("/", webHandleConfig);
      server.on("/a", webHandleConfigSave);
      server.on("/timezonesetup", webHandleTimeZoneSetup);
      server.on("/passwordinput", webHandlePassword);
      server.on("/clockmenustyle.css", handleCSS);
      server.on("/switchwebmode", webHandleSwitchWebMode);
      server.on("/generate_204", webHandleConfig);  //Android captive 
      server.onNotFound(webHandleConfig);

    break;
    
    case 1:
       //setup DNS since we are a client in WiFi net

      clockname.toCharArray(clocknamechar, clockname_len);
      if (!mdns.begin(clocknamechar)) {
        Serial.println("Error setting up MDNS responder!");
        while (1) {
          delay(1000);
        }
      } else {
        Serial.println("mDNS responder started");
      }

      Serial.printf("Starting SSDP...\n");
      SSDP.setSchemaURL("description.xml");
      SSDP.setHTTPPort(80);
      SSDP.setName("The Light Clock");
      SSDP.setSerialNumber("4");
      SSDP.setURL("index");
      SSDP.setModelName("The Light Clock v1");
      SSDP.setModelNumber("2");
      SSDP.setModelURL("http://www.thelightclock.com");
      SSDP.setManufacturer("CAJ Heavy Industries");
      SSDP.setManufacturerURL("http://www.thelightclock.com");
      SSDP.begin();
      
      Serial.println(WiFi.localIP());
      setUpServerHandle();

    break;

    case 2: 
    //direct control over clock through it's own wifi network
      setUpServerHandle();

    break;
      
  }
  if (webtype == 0) {
    

  } else {

  }
  
  //server.onNotFound(webHandleRoot);
  server.begin();
  Serial.println("Web server started");
   //Store global to use in loop()
}

void setUpServerHandle() {
      server.on("/", handleRoot);
      server.on("/index.html", handleRoot);
      server.on("/description.xml", ssdpResponder);
      server.on("/cleareeprom", webHandleClearRom);
      server.on("/cleareepromsure", webHandleClearRomSure);
      server.on("/settings", handleSettings);
      server.on("/timezone", handleTimezone);
      server.on("/clockmenustyle.css", handleCSS);
      server.on("/spectrum.css", handlespectrumCSS);
      server.on("/spectrum.js", handlespectrumjs);
      server.on("/Colour.js", handlecolourjs);
      server.on("/clock.js", handleclockjs);
      server.on("/switchwebmode", webHandleSwitchWebMode);
      server.on("/nightmodedemo", webHandleNightModeDemo);
      server.on("/timeset", webHandleTimeSet);
      server.on("/alarm", webHandleAlarm);
      server.on("/reflection", webHandleReflection);
      server.begin();
      
}





void webHandleSwitchWebMode() {
  Serial.println("Sending webHandleSwitchWebMode");
  if((webMode == 0)||(webMode == 1)) {
    webMode = 2;
    server.send(200, "text/html", "webMode set to 2");
  } else {
    webMode = 1;
    server.send(200, "text/html", "webMode set to 1");
  }
  EEPROM.begin(512);
  delay(10);
  EEPROM.write(186, webMode);
  Serial.println(webMode);
  EEPROM.commit();
  delay(1000);
  EEPROM.end();

  ESP.reset();

  
}

void webHandleConfig() {
  lastInteraction = millis();
  Serial.println("Sending webHandleConfig");
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String s;

  String toSend = webconfig_html;
  //toSend.replace("$css", css_file);
  toSend.replace("$ssids", st);

  server.send(200, "text/html", toSend);
}

void webHandlePassword() {
  Serial.println("Sending webHandlePassword");

  
  String toSend = password_html;
  //toSend.replace("$css", css_file);
  
  server.send(200, "text/html", toSend);

   String qsid;
  if (server.arg("ssid") == "other") {
    qsid = server.arg("other");
  } else {
    qsid = server.arg("ssid");
  }
  cleanASCII(qsid);
  
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

void cleanASCII(String &input) {
  input.replace("%21","!");
  input.replace("%22","\"");
  input.replace("%23","#");
  input.replace("%24","$");
  input.replace("%25","%");
  input.replace("%26","&");
  input.replace("%27","'");
  input.replace("%28","(");
  input.replace("%29",")");
  input.replace("%2A","*");
  input.replace("%2B","+");
  input.replace("%2C",",");
  input.replace("%2D","-");
  input.replace("%2E",".");
  input.replace("%2F","/");
  input.replace("%3A",":");
  input.replace("%3B",";");
  input.replace("%3C","<");
  input.replace("%3D","=");
  input.replace("%3E",">");
  input.replace("%3F","?");
  input.replace("%40","@");
  input.replace("%5B","[");
  input.replace("%5D","]");
  input.replace("%5E","^");
  input.replace("%5F","_");
  input.replace("%60","`");
  input.replace("%7B","{");
  input.replace("%7C","|");
  input.replace("%7D","}");
  input.replace("%7E","~");
  input.replace("%7F","");
  input.replace("+", " ");
   
}

void webHandleTimeZoneSetup() {
  Serial.println("Sending webHandleTimeZoneSetup");
  String toSend = timezonesetup_html;
  //toSend.replace("$css", css_file);
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$latitude", String(latitude));
  toSend.replace("$longitude", String(longitude));

  server.send(200, "text/html", toSend);

  Serial.println("clearing old pass.");
  clearpass();
 

  String qpass;
  qpass = server.arg("pass");
  cleanASCII(qpass);
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
  lastInteraction = millis();
  Serial.println("Sending webHandleConfigSave");
  // /a?ssid=blahhhh&pass=poooo
  String s;
  s = "<p>Settings saved to memory. Clock will now restart and you can find it on your local WiFi network. <p>Please reconnect your phone to your WiFi network first</p>\r\n\r\n";
  server.send(200, "text/html", s);
  EEPROM.begin(512);
  if (server.hasArg("timezone")) {
    String timezonestring = server.arg("timezone");
    timezonevalue = timezonestring.toInt();//atoi(c);
    interpretTimeZone(timezonevalue);
    EEPROM.write(179, timezonevalue);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }

  if (server.hasArg("DST")) {
    DSTtime = 1;
    EEPROM.write(192, 1);
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

void handleCSS() {
  server.send(200, "text/plain", css_file);
//  WiFiClient client = server.client();
//  sendProgmem(client,css_file);
  Serial.println("Sending CSS");
}
void handlecolourjs() {
  server.send(200, "text/plain", colourjs);
//  WiFiClient client = server.client();
//  sendProgmem(client,colourjs);
  Serial.println("Sending colourjs");
}
void handlespectrumjs() {
  server.sendContent(spectrumjs);
//  WiFiClient client = server.client();
//  sendProgmem(client,spectrumjs);
  Serial.println("Sending spectrumjs");
}
void handleclockjs() {
//  server.send(200, "text/plain", clockjs);
//  WiFiClient client = server.client();
//  sendProgmem(client,clockjs);
  Serial.println("Sending clockjs");
}

void handlespectrumCSS() {

//  server.send(200, "text/plain", spectrumCSS);
//  WiFiClient client = server.client();
//  sendProgmem(client,spectrumCSS);
  Serial.println("Sending spectrumCSS");
}

void handleRoot() {
  float alarmHour;
  float alarmMin;
  float alarmSec;
  

  
  EEPROM.begin(512);

  RgbColor tempcolor; 
  HslColor tempcolorHsl; 
  
    

  //toSend.replace("$externallinks", externallinks);
  
  //Check for all the potential incoming arguments
  if(server.hasArg("alarmhour")){
    String alarmHourString = server.arg("alarmhour");  //get value from blend slider
    alarmHour = alarmHourString.toInt();//atoi(c);  //get value from html5 color element
  }
  
  if(server.hasArg("alarmmin")){
    String alarmMinString = server.arg("alarmmin");  //get value from blend slider
    alarmMin = alarmMinString.toInt();//atoi(c);  //get value from html5 color element

    
  }
  
  if(server.hasArg("alarmsec")){
    String alarmSecString = server.arg("alarmsec");  //get value from blend slider
    alarmSec = alarmSecString.toInt();//atoi(c);  //turn value to number
    alarmprogress = 0;
    alarmtick.detach();
    alarmmode = 1;

    alarmtick.attach((alarmHour*3600+alarmMin*60+alarmSec)/(float)pixelCount, alarmadvance);
  }
  
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
  if (server.hasArg("submit")) {
    String memoryarg = server.arg("submit");

    String saveloadmode = memoryarg.substring(5, 11);
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

  if(webMode == 2){
    if (server.hasArg("hourcolorspectrum")) {
      String hourrgbStr = server.arg("hourcolorspectrum");  //get value from html5 color element
      hourrgbStr.replace("%23", "#"); //%23 = # in URI
      getRGB(hourrgbStr, hourcolor);
    }
  
    if (server.hasArg("minutecolorspectrum")) {
      String minutergbStr = server.arg("minutecolorspectrum");  //get value from html5 color element
      minutergbStr.replace("%23", "#"); //%23 = # in URI
      getRGB(minutergbStr, minutecolor);               //convert RGB string to rgb ints
    }
    
  }
  
  if (server.hasArg("blendpoint")) {
    String blendpointstring = server.arg("blendpoint");  //get value from blend slider
    blendpoint = blendpointstring.toInt();//atoi(c);  //get value from html5 color element

  }
    if (server.hasArg("brightness")) {
    String brightnessstring = server.arg("brightness");  //get value from blend slider
    brightness = std::max((int)10, (int)brightnessstring.toInt());//atoi(c);  //get value from html5 color element
    Serial.println(brightness);
    EEPROM.write(191, brightness);
  }

  if (server.hasArg("hourmarks")) {
    String hourmarksstring = server.arg("hourmarks");  //get value from blend slider
    hourmarks = hourmarksstring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(181, hourmarks);
  }
  if (server.hasArg("sleep")) {
    String sleepstring = server.arg("sleep");  //get value input
    sleep = sleepstring.substring(0,2).toInt();//atoi(c);  //get first section of string for hours
    sleepmin = sleepstring.substring(5,7).toInt();//atoi(c);  //get second section of string for minutes
    EEPROM.write(182, sleep);
    EEPROM.write(183, sleepmin);
  }
  if (server.hasArg("wake")) {
    String wakestring = server.arg("wake");  //get value from blend slider
    wake = wakestring.substring(0,2).toInt();//atoi(c);  //get value from html5 color element
    wakemin = wakestring.substring(5,7).toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(189, wake);
    EEPROM.write(190, wakemin);

//update sleep/wake to current
  Serial.println(hour());
  Serial.println(minute());
  Serial.println(sleep);
  Serial.println(sleepmin);
  Serial.println(wake);
  Serial.println(wakemin);
  Serial.println((hour() >= sleep && minute() >= sleepmin));
  Serial.println((hour() <= wake && minute() < wakemin));
  
  
  
  }
  if (server.hasArg("DSThidden")) {
    int oldDSTtime = DSTtime;
    DSTtime = server.hasArg("DST");
    EEPROM.write(192, DSTtime);
    NTPclient.updateTimeZone(timezone+DSTtime);
    adjustTime((DSTtime-oldDSTtime)*3600);
  }
  
  if (server.hasArg("timezone")) {
    int oldtimezone = timezone;
    String timezonestring = server.arg("timezone");
    timezonevalue = timezonestring.toInt();//atoi(c);
    interpretTimeZone(timezonevalue);
    NTPclient.updateTimeZone(timezone+DSTtime);
    //setTime(NTPclient.getNtpTime());
    adjustTime((timezone-oldtimezone)*3600);
    EEPROM.write(179, timezonevalue);
    DSTauto = 0;
    EEPROM.write(185, 0);
  }
  nightCheck();

  

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
    EEPROM.write(185, 1); //tell the system that DST is auto adjusting
    readDSTtime();
    EEPROM.write(179, timezone);


  }


  if (server.hasArg("showsecondshidden")) {
    showseconds = server.hasArg("showseconds");
    EEPROM.write(184, showseconds);
  }
    if (server.hasArg("clockname")) {
    String tempclockname = server.arg("clockname");
    cleanASCII(tempclockname);
    clockname = tempclockname;
    
  
    Serial.println(clockname);
    Serial.println("");
    Serial.println("clearing old clockname.");
    //clear the old clock name out
    for (int i = 195; i < 228; i++) {
      EEPROM.write(i, 0);
    }
    Serial.println("writing eeprom clockname.");
    //addr += EEPROM.put(addr, clockname);
    int clockname_len = clockname.length() + 1; 
    char clocknamechar[clockname_len];
    clockname.toCharArray(clocknamechar, clockname_len);
    if (!mdns.begin(clocknamechar)) {
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    } else {
      Serial.println("mDNS responder started");
    }
   for (int i = 0; i < clockname.length(); ++i){
      EEPROM.write(195+i, clockname[i]);
      Serial.print(clockname[i]);
    }
    Serial.println("");

  }
    //save the current colours in case of crash
    EEPROM.write(100, hourcolor.R);
    EEPROM.write(101, hourcolor.G);
    EEPROM.write(102, hourcolor.B);


    //write the minute color
    EEPROM.write(103, minutecolor.R);
    EEPROM.write(104, minutecolor.G);
    EEPROM.write(105, minutecolor.B);


    //write the blend point
    EEPROM.write(106, blendpoint);



  String toSend = root_html;
  String tempgradient = "";
  String csswgradient = "";
  const String scheme = "scheme";
  for(int i = 1; i < 4; i++){
    //loop makes each of the save/load buttons coloured based on the scheme
    tempgradient = buttongradient_css;
    //load hour color
    tempcolor.R = EEPROM.read(100 + i * 15);
    tempcolor.G = EEPROM.read(101 + i * 15);
    tempcolor.B = EEPROM.read(102 + i * 15);
    //fix darkened colour schemes by manually lightening them. 

    tempgradient.replace("$hourcolor", rgbToText(tempcolor));
    //load minute color
    tempcolor.R = EEPROM.read(103 + i * 15);
    tempcolor.G = EEPROM.read(104 + i * 15);
    tempcolor.B = EEPROM.read(105 + i * 15);

    tempgradient.replace("$minutecolor", rgbToText(tempcolor));

    tempgradient.replace("$scheme", scheme+i);

    csswgradient += tempgradient;
    
    
    
  }
  if(webMode != 2){
    // don't send external links if we're local only
    toSend.replace("$externallinks", externallinks);
      toSend.replace("$csswgradient", csswgradient);
  }

  
  toSend.replace("$minutecolor", rgbToText(minutecolor));
  toSend.replace("$hourcolor", rgbToText(hourcolor));
  toSend.replace("$blendpoint", String(int(blendpoint)));
  toSend.replace("$brightness", String(int(brightness)));
  server.send(200, "text/html", toSend);
  
  Serial.println("Sending handleRoot");
  EEPROM.commit();
  delay(300);

}

void nightCheck() {
    if(hour() > sleep || hour() < wake || ((hour() == sleep && minute() >= sleepmin) || (hour() == wake && minute() < wakemin))){
        nightmode = 1;
        Serial.println("nightmode 1");
    } else {
        nightmode = 0;
        Serial.println("nightmode 0");
    }
}
void handleSettings() {
//  String fontreplace;
//  if(webMode == 1){fontreplace=importfonts;} else {fontreplace="";}
  Serial.println("Sending handleSettings");
  String toSend = settings_html;
  for (int i = 82; i > 0; i--) {
    if (i == timezonevalue) {
      toSend.replace("$timezonevalue" + String(i), "selected");
    } else {
      toSend.replace("$timezonevalue" + String(i), "");
    }
  }
  for (int i = 0; i < 5; i++) {
    if (i == hourmarks) {
      toSend.replace("$hourmarks" + String(i), "selected");
    } else {
      toSend.replace("$hourmarks" + String(i), "");
    }
  }

  if(webMode != 2){
    // don't send external links if we're local only
    toSend.replace("$externallinks", externallinks);
  }
  String ischecked;
  showseconds ? ischecked = "checked" : ischecked = "";
  toSend.replace("$showseconds", ischecked);
  DSTtime ? ischecked = "checked" : ischecked = "";
    Serial.println(timeToText(sleep, sleepmin));
  Serial.println(timeToText(wake, wakemin));
  toSend.replace("$DSTtime", ischecked);
  toSend.replace("$sleep", timeToText(sleep, sleepmin));
  toSend.replace("$wake", timeToText(wake, wakemin));
  toSend.replace("$timezone", String(timezone));
  toSend.replace("$clockname", String(clockname));


  server.send(200, "text/html", toSend);

}

void handleTimezone() {
    String fontreplace;
  if(webMode == 1){fontreplace=importfonts;} else {fontreplace="";}
  String toSend = timezone_html;
  //toSend.replace("$css", css_file);
  //toSend.replace("$fonts", fontreplace);
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
  //toSend.replace("$css", css_file);
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

String timeToText(int hours, int minutes) {
  String out;
    (String(hours, DEC)).length() == 1 ? out += "0" : out += "";
    out += String(hours, DEC);
    out += ":";
    (String(minutes, DEC)).length() == 1 ? out += "0" : out += "";
    out += String(minutes, DEC);
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
  if(alarmmode > 0) {
    if(alarmmode == 1){
      alarmface();  
    }else{
      
    }
    
    
  } else {
    switch (testrun) {
      case 0:
        // no testing
        hour_pos = (hour() % 12) * pixelCount / 12 + minute()*pixelCount / 720;
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

      case 3:
        //set the face to reflection mode
        hour_pos = pixelCount - ((hour() % 12) * pixelCount / 12 + minute() / 6);
        min_pos = pixelCount - minute() * pixelCount / 60;

        
    }
  
    if (nightmode) {
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
        if(testrun == 3) {
          invertLED(pixelCount - second()*pixelCount / 60);
        }
        else {
          invertLED(second()*pixelCount / 60);
        }
      }
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
    c2 = HslColor(hourcolor);
    c1 = HslColor(minutecolor);
  }
  else
  {
    c1 = HslColor(hourcolor);
    c2 = HslColor(minutecolor);
  }
  // the blending is the colour that the hour/minute colour will meet. The greater the blend, the closer to the actual hour/minute colour it gets.
  c2blend = c2blend.LinearBlend(c2, c1, (float)blendpoint / 100);
  c1blend = c1blend.LinearBlend(c1, c2, (float)blendpoint / 100);

  gap = secondhand - firsthand;

  //create the blend between first and 2nd hand
  for (uint16_t i = firsthand; i < secondhand; i++) {
    clock.SetPixelColor(i, HslColor::LinearBlend(c2blend, c2, ((float)i - (float)firsthand) / (float)gap), brightness);
  }
  gap = pixelCount - gap;
  //and the last hand
  for (uint16_t i = secondhand; i < pixelCount + firsthand; i++) {
    clock.SetPixelColor(i % pixelCount, HslColor::LinearBlend(c1blend, c1, ((float)i - (float)secondhand) / (float)gap),brightness); 
  }
  clock.SetPixelColor(hour_pos, hourcolor,brightness); 
  clock.SetPixelColor(min_pos, minutecolor,brightness); 
}

void nightface(uint16_t hour_pos, uint16_t min_pos) {
  for (int i = 0; i < pixelCount; i++) {
    clock.SetPixelColor(i, 0, 0, 0);
  }
  clock.SetPixelColor(hour_pos, hourcolor, std::min(30,brightness)); 
  clock.SetPixelColor(min_pos, minutecolor,std::min(30,brightness)); 

}

void alarmface(){
  //Serial.println("showing alarmface");
  for (int i = 0; i < alarmprogress; i++) {
    clock.SetPixelColor(i, 0, 0, 0);
  }
  for (int i=alarmprogress; i < pixelCount; i++) {
    clock.SetPixelColor(i, 255, 0, 0);
  }
}


void alarmadvance(){
  //Serial.println("advancing alarm");
  alarmprogress++;
  if(alarmprogress==pixelCount) {
    alarmtick.detach();
    alarmtick.attach(0.3, flashface);
    alarmprogress = 0;
    
  }
  updateface();
}

void flashface(){
  alarmmode = 2;
  if(alarmprogress==10){
    alarmtick.detach();
    alarmprogress = 0;
    alarmmode = 0;
  } else {
    if((alarmprogress % 2)==0){
      for (int i=0; i < pixelCount; i++) {
        clock.SetPixelColor(i, 255, 0, 0);
      }
    } else {
      for (int i=0; i < pixelCount; i++) {
        clock.SetPixelColor(i, 0, 0, 0);
      }
    }
  }
  
  alarmprogress++;
  updateface();
}

void invertLED(int i) {
  //This function will set the LED to in inverse of the two LEDs next to it showing as white on the main face
  RgbColor averagecolor;
  averagecolor = RgbColor::LinearBlend(clock.GetPixelColor((i - 1) % pixelCount), clock.GetPixelColor((i + 1) % pixelCount), 0.5);
  averagecolor = RgbColor(255 - averagecolor.R, 255 - averagecolor.G, 255 - averagecolor.B);
  clock.SetPixelColor(i, averagecolor,brightness);
}

void showHourMarks() {
  //shows white at the four quadrants and darkens each hour mark to help the user tell the time
//  RgbColor c;
//  for (int i = 0; i < 12; i++) {
//    c = clock.GetPixelColor(i);
//    c.Darken(255);
//    clock.SetPixelColor(i * pixelCount / 12, c,brightness);
//  }

  for (int i = 0; i < 12; i++) {
    invertLED(i * pixelCount / 12);
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

//void nightModeAnimation() {
//  //darkens the pixels animation to switch to nightmode.
////  int firsthand = std::min(hour_pos, min_pos);
////  int secondhand = std::max(hour_pos, min_pos);
////  int firsthandlen = (120+firsthand-secondhand)%120;
////  int secondhandlen = 120-firsthandlen;
//  
//  
//  
//  RgbColor c;
//  
//  for (uint16_t i = 0; i < 240; i++) {
//    for (uint16_t j = 0; j < std::min(i, (uint16_t)120); i++) {
//    c = clock.GetPixelColor(i);
//    c.Darken(20);
//    clock.SetPixelColor(i, c);
//    
//    }
//    clock.Show();
//    delay(10);
//  }
//}

void logo() {
  //this lights up the clock as the C logo
  //yellow section
  for (int i = 14 / (360 / pixelCount); i < 48 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 100, 100, 0);
  }

  //blank section
  for (int i = 48 / (360 / pixelCount); i < 140 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 0, 0, 0);
  }

  //blue section
  for (int i = 140 / (360 / pixelCount); i < 296 / (360 / pixelCount); i++) {
    clock.SetPixelColor(i, 0, 60, 120);
  }

  //green section
  for (int i = 296 / (360 / pixelCount); i < (360 + 14) / (360 / pixelCount); i++) {
    clock.SetPixelColor(i % pixelCount, 30, 120, 0);
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
  if (partition >= 0 && partition <= 4) { // only 3 locations for saved faces. Don't accidentally overwrite other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color

    EEPROM.write(100 + partition * 15, hourcolor.R);
    EEPROM.write(101 + partition * 15, hourcolor.G);
    EEPROM.write(102 + partition * 15, hourcolor.B);


    //write the minute color
    EEPROM.write(103 + partition * 15, minutecolor.R);
    EEPROM.write(104 + partition * 15, minutecolor.G);
    EEPROM.write(105 + partition * 15, minutecolor.B);


    //write the blend point
    EEPROM.write(106 + partition * 15, blendpoint);

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
  if (partition >= 0 && partition <= 4) { // only 3 locations for saved faces. Don't accidentally read/write other sections of eeprom!
    EEPROM.begin(512);
    delay(10);
    //write the hour color
    hourcolor.R = EEPROM.read(100 + partition * 15);
    hourcolor.G = EEPROM.read(101 + partition * 15);
    hourcolor.B = EEPROM.read(102 + partition * 15);

    //write the minute color
    minutecolor.R = EEPROM.read(103 + partition * 15);
    minutecolor.G = EEPROM.read(104 + partition * 15);
    minutecolor.B = EEPROM.read(105 + partition * 15);

    //write the blend point
    blendpoint = EEPROM.read(106 + partition * 15);
  }
}
//-----------------------------Demo functions (for filming etc)---------------------------------

void webHandleNightModeDemo() {
  nightmode=0;
  setTime(21,59,50,1,1,1);
  sleep = 22;
  sleepmin = 0;
  server.send(200, "text/html", "demo of night mode");
}

void webHandleTimeSet() {
  
    if (server.hasArg("time")) {
    String timestring = server.arg("time");  //get value input
    int timehr = timestring.substring(0,2).toInt();//atoi(c);  //get first section of string for hours
    int timemin = timestring.substring(5,7).toInt();//atoi(c);  //get second section of string for minutes
    setTime(timehr,timemin,0,1,1,1);}

    server.send(200, "text/html", "<form class=form-verticle action=/timeset method=GET> Time Reset /p <input type=time name=time value="+timeToText((int)hour(), (int)minute())+">/p <input type=submit name=submit value='Save Settings'/>");
  
}

void webHandleReflection() {
  if(testrun == 3) {
    testrun = 0;
    server.send(200, "text", "Clock has been set to normal mode.");
    }
    else {    
      testrun = 3;
      server.send(200, "text", "Clock has been set to reflection mode.");
    }
}

void webHandleAlarm() {
    String toSend = alarm_html;
    toSend.replace("$externallinks", externallinks);
    server.send(200, "html", toSend);
  
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

//---------------------------------------SSDP repsponding fucntions-------------------------------------------------------

void ssdpResponder() {
   //WiFiClient client = HTTP.client();
    int clockname_len = clockname.length() + 1; 
    char clocknamechar[clockname_len];
    clockname.toCharArray(clocknamechar, clockname_len);
    String str = "<root><specVersion><major>1</major><minor>0</minor></specVersion><URLBase>http://" + ipString + ":80/</URLBase><device><deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType><friendlyName>" + clocknamechar + "(" + ipString + ")</friendlyName><manufacturer>Omnino Realis</manufacturer><manufacturerURL>http://www.thelightclock.com</manufacturerURL><modelDescription>The Light Clock v1</modelDescription><modelName>The Light Clock v1</modelName><modelNumber>4</modelNumber><modelURL>http://www.thelightclock.com</modelURL><serialNumber>3</serialNumber><UDN>uuid:3</UDN><presentationURL>index.html</presentationURL><iconList><icon><mimetype>image/png</mimetype><height>48</height><width>48</width><depth>24</depth><url>www.thelightclock.com/clockjshosting/logo.png</url></icon><icon><mimetype>image/png</mimetype><height>120</height><width>120</width><depth>24</depth><url>www.thelightclock.com/clockjshosting/logo.png</url></icon></iconList></device></root>";
    server.send(200, "text/plain", str);
    Serial.println("SSDP packet sent");
    
  
}

String StringIPaddress(IPAddress myaddr)
{
  String LocalIP = "";
  for (int i = 0; i < 4; i++)
  {
    LocalIP += String(myaddr[i]);
    if (i < 3) LocalIP += ".";
  }
  return LocalIP;
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

void interpretTimeZone(int timezonename) {
  switch(timezonename){
    case 1: timezone=-12; break;
    case 2: timezone=-11; break;
    case 3: timezone=-10; break;
    case 4: timezone=-9; break;
    case 5: timezone=-8; break;
    case 6: timezone=-8; break;
    case 7: timezone=-7; break;
    case 8: timezone=-7; break;
    case 9: timezone=-7; break;
    case 10: timezone=-6; break;
    case 11: timezone=-6; break;
    case 12: timezone=-6; break;
    case 13: timezone=-6; break;
    case 14: timezone=-5; break;
    case 15: timezone=-5; break;
    case 16: timezone=-5; break;
    case 17: timezone=-4; break;
    case 18: timezone=-4; break;
    case 19: timezone=-4; break;
    case 20: timezone=-4; break;
    case 21: timezone=-3.5; break;
    case 22: timezone=-3; break;
    case 23: timezone=-3; break;
    case 24: timezone=-3; break;
    case 25: timezone=-3; break;
    case 26: timezone=-2; break;
    case 27: timezone=-1; break;
    case 28: timezone=-1; break;
    case 29: timezone=0; break;
    case 30: timezone=0; break;
    case 31: timezone=1; break;
    case 32: timezone=1; break;
    case 33: timezone=1; break;
    case 34: timezone=1; break;
    case 35: timezone=1; break;
    case 36: timezone=2; break;
    case 37: timezone=2; break;
    case 38: timezone=2; break;
    case 39: timezone=2; break;
    case 40: timezone=2; break;
    case 41: timezone=2; break;
    case 42: timezone=2; break;
    case 43: timezone=2; break;
    case 44: timezone=2; break;
    case 45: timezone=3; break;
    case 46: timezone=3; break;
    case 47: timezone=3; break;
    case 48: timezone=3; break;
    case 49: timezone=3.5; break;
    case 50: timezone=4; break;
    case 51: timezone=4; break;
    case 52: timezone=4; break;
    case 53: timezone=4.5; break;
    case 54: timezone=5; break;
    case 55: timezone=5; break;
    case 56: timezone=5.5; break;
    case 57: timezone=5.5; break;
    case 58: timezone=5.75; break;
    case 59: timezone=6; break;
    case 60: timezone=6; break;
    case 61: timezone=6.5; break;
    case 62: timezone=7; break;
    case 63: timezone=7; break;
    case 64: timezone=8; break;
    case 65: timezone=8; break;
    case 66: timezone=8; break;
    case 67: timezone=8; break;
    case 68: timezone=8; break;
    case 69: timezone=9; break;
    case 70: timezone=9; break;
    case 71: timezone=9; break;
    case 72: timezone=9.5; break;
    case 73: timezone=9.5; break;
    case 74: timezone=10; break;
    case 75: timezone=10; break;
    case 76: timezone=10; break;
    case 77: timezone=10; break;
    case 78: timezone=10; break;
    case 79: timezone=11; break;
    case 80: timezone=12; break;
    case 81: timezone=12; break;
    case 82: timezone=13; break;
  }
}

