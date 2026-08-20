#include <Arduino.h>



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


#include <WebSocketsServer.h>
#include <math.h>
#include <TimeLib.h>
//#include <TimeAlarms.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>
#include <Ticker.h>
//#include <FastLED.h>

#include "h/settings.h"
#include "h/root.h"
#include "h/timezone.h"
#include "h/timezonesetup.h"
#include "h/assets.h"
#include "h/tzdata.h"
#include "h/webconfig.h"
#include "h/clearromsure.h"
#include "h/password.h"
#include "h/buttongradient.h"
#include "h/externallinks.h"
#include "h/alarm.h"
#include "h/game.h"
#include <ESP8266HTTPUpdateServer.h>

#define clockPin 4                //GPIO pin that the LED strip is on
int pixelCount = 120;            //number of pixels in RGB clock


#define night 0                   // for switching between various clock modes
#define alarm 1
#define normal 2
#define dawnmode 3
#define game 4
int clockmode = normal;

//for switching various night clock modes
#define black 0
#define dots 1
#define dim 2
#define moonphase 3
#define disabled 4
#define moonmode 1
bool dawnbreak;
int sleeptype = dots;

#define gamestartpoints 20 //number of points a player starts with, this will determine how long a game lasts
//#define SECS_PER_HOUR 3600        //number of seconds in an hour

byte mac[6]; // MAC address
String macString;
String ipString;
String netmaskString;
String gatewayString;
String clockname = "thelightclock";

IPAddress dns(8, 8, 8, 8);  //Google dns
const char* ssid = "The Light Clock"; //The ssid when in AP mode
bool mdnsStarted = false;
WebSocketsServer webSocket = WebSocketsServer(81);
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

NeoPixelBus* clockleds = NULL; // NeoPixelBus(pixelCount, clockPin);  //Clock Led on Pin 4
time_t localClockProvider(void);
Ticker NTPsyncclock;
Ticker alarmtick;
int alarmprogress = 0;
Ticker pulseBrightnessTicker;
Ticker gamestartticker;
int pulseBrightnessCounter =0;
Ticker dawntick;//a ticker to establish how far through dawn we are
int dawnprogress = 0;

// SNTP keeps these up to date in the background once configTime() has run.
// They must outlive the call: lwip stores the pointers rather than copying.
const char* const ntpServer1 = "pool.ntp.org";
const char* const ntpServer2 = "time.google.com";



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


RgbColor hourcolor; // starting colour of hour
RgbColor minutecolor; //starting colour of minute
RgbColor alarmcolor; //the color the alarm will be
int brightness = 50; // a variable to dim the over-all brightness of the clock
int maxBrightness = 100;
uint8_t blendpoint = 40; //level of default blending
int randommode; //face changes colour every hour
int hourmarks = 1; //where marks should be made (midday/quadrants/12/brianmode)
int sleep = 22; //when the clock should go to night mode
int sleepmin = 0; //when the clock should go to night mode
int wake = 7; //when clock should wake again
int wakemin = 0; //when clock should wake again
int nightmode = 0;
unsigned long lastInteraction;

uint16_t tzZone = tzDefaultZone; //index into tzZones: the place the clock lives
bool showseconds; //should the seconds hand tick around
int alarmmode = 0;
int gamearray[6];
RgbColor playercolors[6];
int playersremaining = 0;
int playercount = 0;
int nextplayer =0;
int gamebrightness = 100;
int speed = 0;
bool gamestarted = 0;
int loopcounter;

//new_moon = makeTime(0, 0, 0, 7, 0, 1970);

int accelerator = 5;
int prevsecond;
int hourofdeath; //saves the time incase of an unplanned reset
int minuteofdeath; //saves the time incase of an unplanned reset



//-----------------------------------standard arduino setup and loop-----------------------------------------------------------------------
void setup() {
  playercolors[0] = RgbColor(255, 0, 0);
  playercolors[1] = RgbColor(0, 255, 0);
  playercolors[2] = RgbColor(0, 0, 255);
  playercolors[3] = RgbColor(255, 255, 0);
  playercolors[4] = RgbColor(255, 0, 255);
  playercolors[5] = RgbColor(0, 255, 255);//needed to move the colour declaration to setup.

  Serial.begin(115200);
  Serial.println();
  Serial.println("The Light Clock starting. Type 'help' for the serial commands.");

  httpUpdater.setup(&server);
  EEPROM.begin(512);
  //write a magic byte to eeprom 196 to determine if we've ever booted on this device before
  if (EEPROM.read(500) != 196) {
    //if not load default config files to EEPROM
    writeInitalConfig();
  }

  loadConfig();
  delay(10);
  delete clockleds;
  clockleds = new NeoPixelBus(pixelCount, clockPin);
  clockleds->Begin();



  nightCheck();
  updateface();
  clockleds->Show();
  initWiFi();
  lastInteraction = millis();
  delay(1000);
  //initialise the NTP clock sync function
  if (webMode == 1) {
    applyTimeZone(tzZone);
    // Re-reading local time is now just a localtime_r() call -- SNTP refreshes
    // itself in the background -- so it is cheap to do often, which keeps the
    // clock from running an hour behind on the night the zone changes over.
    setSyncInterval(30);
    setSyncProvider(localClockProvider);

    macString = String(WiFi.macAddress());
    ipString = StringIPaddress(WiFi.localIP());
    netmaskString = StringIPaddress(WiFi.subnetMask());
    gatewayString = StringIPaddress(WiFi.gatewayIP());
  }
  //UDP.begin(localPort);
  prevsecond = second();// initalize prev second for main loop

  //update sleep/wake to current
  nightCheck();
  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

}

void loop() {
  //Serial.print("Loop ");
  if (webMode == 0) {
    //initiate web-capture mode
    dnsServer.processNextRequest();
  }
  if (webMode == 0 && millis() - lastInteraction > 300000) {//if the clock is looking for wifi network and hasn't found one after 5 minutes, and no one has tried to set it up, then reboot the clock
    lastInteraction = millis();

    ESP.reset();
  }
  pollSerialConsole();
  webSocket.loop();
  server.handleClient();
  if (mdnsStarted) {
    MDNS.update();
  }
  delay(50);
  if (second() != prevsecond) {
    if (webMode != 0 && second() == 0 && minute() % 10 == 0) { //only record "time of death" if we're in normal running mode.
      EEPROM.begin(512);
      delay(10);
      EEPROM.write(193, hour());
      EEPROM.write(194, minute());
      EEPROM.write(191, brightness);
      EEPROM.commit();
      delay(50); // this section of code will save the "time of death" to the clock so if it unexpectedly resets it should be seemless to the user.
      saveFace(0);
    }
    if (second() == 0) {
      if (hour() == sleep && minute() == sleepmin) {
        clockmode = night;
      }
      if (hour() == wake && minute() == wakemin) {
        clockmode = normal;
      }
      if ((hour() + 25) % 24 == wake && minute() == wakemin) {
        if (dawnbreak) {
          dawnprogress = 0;
          clockmode = dawnmode;
          dawntick.attach(14, dawnadvance);
        }
      }

      //nightCheck();
    }
    //    updateface();
    prevsecond = second();
    //    Serial.print("loopcounter: ");
    //    Serial.println(loopcounter);
    //    loopcounter = 0;
  }
  updateface();
  clockleds->Show();
  loopcounter++;


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
  readTimeZoneConfig();
  randommode = EEPROM.read(180);
  Serial.print("randommode: ");
  Serial.println(randommode);
  hourmarks = EEPROM.read(181);
  Serial.print("hourmarks: ");
  Serial.println(hourmarks);
  sleep = EEPROM.read(182);
  Serial.print("sleep: ");
  Serial.println(sleep);
  sleeptype = EEPROM.read(228);
  Serial.print("sleep: ");
  Serial.println(sleep);
  sleepmin = EEPROM.read(183);
  Serial.print("sleepmin: ");
  Serial.println(sleepmin);
  showseconds = EEPROM.read(184);
  Serial.print("showseconds: ");
  Serial.println(showseconds);
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
  hourofdeath = EEPROM.read(193);
  Serial.print("Hour of Death: ");
  Serial.println(hourofdeath);
  minuteofdeath = EEPROM.read(194);
  Serial.print("minuteofdeath: ");
  Serial.println(minuteofdeath);
  setTime(hourofdeath, minuteofdeath, 30, 0, 0, 0);
  dawnbreak = EEPROM.read(229);
  Serial.print("dawnbreak: ");
  Serial.println(dawnbreak);
  pixelCount = EEPROM.read(230);
  Serial.print("pixelcount: ");
  Serial.println(pixelCount);
  maxBrightness = EEPROM.read(231);
  Serial.print("maxBrightness: ");
  Serial.println(maxBrightness);
}

void writeInitalConfig() {
  Serial.println("can't find settings so writing defaults");
  EEPROM.begin(512);
  delay(10);
  writeTimeZone(tzDefaultZone); //default zone, DST rules and all
  EEPROM.write(180, 0);//default randommode off
  EEPROM.write(181, 0); //default hourmarks to off
  EEPROM.write(182, 22); //default to sleep at 22:00
  EEPROM.write(183, 0);
  EEPROM.write(184, 1); //default to showseconds to yes
  EEPROM.write(186, 0); //default webMode to setup mode off until user sets local wifi
  EEPROM.write(500, 196);//write magic byte to 500 so that system knows its set up.
  EEPROM.write(228, 1);//default sleeptype to 1 (dots)
  EEPROM.write(189, 7);//default wake 7 hours
  EEPROM.write(190, 0); //default to wake at 00 minutes
  EEPROM.write(191, 100); //default to full brightness on USB so as not to crash
  EEPROM.write(193, 10); //default "hour of death" is 10am
  EEPROM.write(220, 1); //default dawnbreak to "on"
  EEPROM.write(230, 120); //default to normal size light clock
  EEPROM.write(231, 255); //default to mains power for max brightness


  for (int i = 195; i < 228; i++) {//zero (instead of null) the values where clockname will be written.
    EEPROM.write(i, 0);
  }
  EEPROM.write(194, 10); //default "minute of death" is 10am
  for (int i = 0; i < clockname.length(); ++i) {
    EEPROM.write(195 + i, clockname[i]);
    Serial.print(clockname[i]);
  }







  EEPROM.commit();
  delay(500);

  //face 1 defaults
  hourcolor = RgbColor(255, 255, 0);
  minutecolor = RgbColor(0, 57, 255);
  blendpoint = 70;
  saveFace(0);
  saveFace(1);
  //face 2 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(0, 0, 255);
  blendpoint = 60;
  saveFace(2);
  //face 3 defaults
  hourcolor = RgbColor(255, 0, 0);
  minutecolor = RgbColor(255, 255, 0);
  blendpoint = 90;
  saveFace(3);

}




void initWiFi() {
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  esid.trim();
  if (webMode == 2) {
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

// Routes for the locally-hosted CSS/JS. Registered in every web mode so the UI
// renders identically with or without an internet connection.
void sendAsset(const WebAsset& asset) {
  if (asset.gzipped) {
    server.sendHeader(F("Content-Encoding"), F("gzip"));
  }
  server.sendHeader(F("Cache-Control"), F("max-age=86400"));
  server.send_P(200, asset.mime, (PGM_P)asset.data, asset.len);
}

void registerAssetRoutes() {
  for (size_t i = 0; i < webAssetCount; i++) {
    WebAsset asset;
    memcpy_P(&asset, &webAssets[i], sizeof(asset));
    server.on(asset.path, [asset]() { sendAsset(asset); });
  }
}

void launchWeb(int webtype) {
  webMode = webtype;
  int clockname_len = clockname.length() + 1;
  char clocknamechar[clockname_len];
  Serial.println("");
  Serial.println("WiFi connected");
  switch (webtype) {
    case 0:
      //set up wifi network to connect to since we are in setup mode.
      webMode == 0;
      Serial.println(WiFi.softAPIP());
      server.on("/", webHandleConfig);
      server.on("/a", webHandleConfigSave);
      server.on("/timezonesetup", webHandleTimeZoneSetup);
      server.on("/tzlist", handleTimezoneList);
      server.on("/passwordinput", webHandlePassword);
      registerAssetRoutes();
      server.on("/switchwebmode", webHandleSwitchWebMode);
      server.on("/generate_204", webHandleConfig);  //Android captive
      server.onNotFound(webHandleConfig);

      break;

    case 1:
      //setup DNS since we are a client in WiFi net

      clockname.toCharArray(clocknamechar, clockname_len);
      mdnsStarted = MDNS.begin(clocknamechar);
      if (mdnsStarted) {
        Serial.println("mDNS responder started");
        // Only legal once begin() has succeeded.
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ws", "tcp", 81);
      } else {
        // Not fatal: the clock is still reachable by IP address.
        Serial.println("Error setting up MDNS responder!");
      }

      Serial.printf("Starting SSDP...\n");
      SSDP.setSchemaURL("description.xml");
      SSDP.setHTTPPort(80);
      SSDP.setName("The Light Clock");
      SSDP.setSerialNumber("4");
      SSDP.setURL("index");
      SSDP.setModelName("The Light Clock v1");
      SSDP.setModelNumber("4");
      SSDP.setModelURL("http://www.thelightclock.com");
      SSDP.setManufacturer("Omnino Realis");
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
  server.on("/tzlist", handleTimezoneList);
  registerAssetRoutes();
  server.on("/switchwebmode", webHandleSwitchWebMode);
  server.on("/nightmodedemo", webHandleNightModeDemo);
  server.on("/timeset", webHandleTimeSet);
  server.on("/alarm", webHandleAlarm);
  server.on("/reflection", webHandleReflection);
  server.on("/dawn", webHandleDawn);
  server.on("/moon", webHandleMoon);
  server.on("/brighttest", brighttest);
  server.on("/lightup", lightup);
  server.on("/game", webHandleGame);
  server.on("/speed",speedup);

  server.begin();

}


void speedup() {
  speed++;
  speed = speed%3;
  server.send(200, "text/html", "Speed Up: " + speed);

}


void webHandleSwitchWebMode() {
  Serial.println("Sending webHandleSwitchWebMode");
  if ((webMode == 0) || (webMode == 1)) {
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

  String toSend = FPSTR(webconfig_html);
  toSend.replace("$ssids", st);

  server.send(200, "text/html", toSend);
}

void webHandlePassword() {
  Serial.println("Sending webHandlePassword");


  String toSend = FPSTR(password_html);

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
  input.replace("%21", "!");
  input.replace("%22", "\"");
  input.replace("%23", "#");
  input.replace("%24", "$");
  input.replace("%25", "%");
  input.replace("%26", "&");
  input.replace("%27", "'");
  input.replace("%28", "(");
  input.replace("%29", ")");
  input.replace("%2A", "*");
  input.replace("%2B", "+");
  input.replace("%2C", ",");
  input.replace("%2D", "-");
  input.replace("%2E", ".");
  input.replace("%2F", "/");
  input.replace("%3A", ":");
  input.replace("%3B", ";");
  input.replace("%3C", "<");
  input.replace("%3D", "=");
  input.replace("%3E", ">");
  input.replace("%3F", "?");
  input.replace("%40", "@");
  input.replace("%5B", "[");
  input.replace("%5D", "]");
  input.replace("%5E", "^");
  input.replace("%5F", "_");
  input.replace("%60", "`");
  input.replace("%7B", "{");
  input.replace("%7C", "|");
  input.replace("%7D", "}");
  input.replace("%7E", "~");
  input.replace("%7F", "");
  input.replace("+", " ");

}

void webHandleTimeZoneSetup() {
  Serial.println("Sending webHandleTimeZoneSetup");
  String toSend = FPSTR(timezonesetup_html);
  toSend.replace("$zoneindex", String(tzZone));

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
  if (server.hasArg("zone")) {
    writeTimeZone(parseZoneArg());
  }
  if (server.hasArg("pixelCount")) {
    String pixelCountString = server.arg("pixelCount");  //get value from blend slider
    pixelCount = pixelCountString.toInt();//atoi(c);  //get value from html5 color element
    ChangeNeoPixels(pixelCount, clockPin);
    EEPROM.write(230, pixelCount);
  }

  if (server.hasArg("powerType")) {
    String powerTypeString = server.arg("powerType");  //get value from blend slider
    int powerType = powerTypeString.toInt();//atoi(c);  //get value from html5 color element
    if (powerType == 1) {
      maxBrightness = 255;
    } else {
      maxBrightness = 100;

    }
    brightness = maxBrightness;
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
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
  float alarmHour;
  float alarmMin;
  float alarmSec;



  EEPROM.begin(512);

  RgbColor tempcolor;
  HslColor tempcolorHsl;

  if (server.hasArg("pixelCount")) {
    String pixelCountString = server.arg("pixelCount");  //get value from blend slider
    pixelCount = pixelCountString.toInt();//atoi(c);  //get value from html5 color element
    ChangeNeoPixels(pixelCount, clockPin);
    EEPROM.write(230, pixelCount);
  }

  if (server.hasArg("powerType")) {
    String powerTypeString = server.arg("powerType");  //get value from blend slider
    int powerType = powerTypeString.toInt();//atoi(c);  //get value from html5 color element
    if (powerType == 1) {
      maxBrightness = 255;
    } else {
      maxBrightness = 100;
      brightness = _min(maxBrightness, brightness);
    }
    EEPROM.write(191, brightness);
    EEPROM.write(231, maxBrightness);
  }





  //Check for all the potential incoming arguments
  if (server.hasArg("alarmhour")) {
    String alarmHourString = server.arg("alarmhour");  //get value from blend slider
    alarmHour = alarmHourString.toInt();//atoi(c);  //get value from html5 color element
  }

  if (server.hasArg("alarmmin")) {
    String alarmMinString = server.arg("alarmmin");  //get value from blend slider
    alarmMin = alarmMinString.toInt();//atoi(c);  //get value from html5 color element
  }

  if (server.hasArg("alarmsec")) {
    String alarmSecString = server.arg("alarmsec");  //get value from blend slider
    alarmSec = alarmSecString.toInt();//atoi(c);  //turn value to number
    alarmprogress = 0;
    alarmtick.detach();
    Serial.println("alarm triggered");
    clockmode = alarm;

    alarmtick.attach((alarmHour * 3600 + alarmMin * 60 + alarmSec) / (float)pixelCount, alarmadvance);
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

  if (server.hasArg("alarmcolor")) {
    String minutergbStr = server.arg("alarmcolor");  //get value from html5 color element
    minutergbStr.replace("%23", "#"); //%23 = # in URI
    getRGB(minutergbStr, alarmcolor);               //convert RGB string to rgb ints
  }
  if (server.hasArg("submit")) {


    String memoryarg = server.arg("submit");

    String saveloadmode = memoryarg.substring(5, 11);

    Serial.print("Submit: ");
    Serial.println(memoryarg);
    if (saveloadmode == "Scheme") {

      String saveload = memoryarg.substring(0, 4);
      Serial.print("Save/Load: ");
      Serial.println(saveload);
      String location = memoryarg.substring(12);
      Serial.print("Location: ");
      Serial.println(location);
      if (saveload == "Save") {
        saveFace(location.toInt());
      } else {
        loadFace(location.toInt());
      }
    }
  }

  if (webMode == 2) {
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
    brightness = _max((int)10, (int)brightnessstring.toInt());//atoi(c);  //get value from html5 color element
    Serial.print("brightness: ");
    Serial.println(brightness);
    EEPROM.write(191, brightness);
  }

  if (server.hasArg("hourmarks")) {
    String hourmarksstring = server.arg("hourmarks");  //get value from blend slider
    hourmarks = hourmarksstring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(181, hourmarks);
  }
  if (server.hasArg("sleeptype")) {
    String sleeptypestring = server.arg("sleeptype");  //get value from blend slider
    sleeptype = sleeptypestring.toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(228, sleeptype);
  }
  if (server.hasArg("sleep")) {
    String sleepstring = server.arg("sleep");  //get value input
    sleep = sleepstring.substring(0, 2).toInt(); //atoi(c);  //get first section of string for hours
    sleepmin = sleepstring.substring(3).toInt();//atoi(c);  //get second section of string for minutes
    EEPROM.write(182, sleep);
    EEPROM.write(183, sleepmin);
  }
  if (server.hasArg("wake")) {
    String wakestring = server.arg("wake");  //get value from blend slider
    wake = wakestring.substring(0, 2).toInt(); //atoi(c);  //get value from html5 color element
    wakemin = wakestring.substring(3).toInt();//atoi(c);  //get value from html5 color element
    EEPROM.write(189, wake);
    EEPROM.write(190, wakemin);

    //update sleep/wake to current
    Serial.println("");
    Serial.print("time: ");
    Serial.println(timeToText(hour(), minute()));
    Serial.print("sleep: ");
    Serial.println(timeToText(sleep, sleepmin));
    Serial.print("wake: ");
    Serial.println(timeToText(wake, wakemin));
    nightCheck();
  }
  if (server.hasArg("zone")) {
    uint16_t chosen = parseZoneArg();
    if (chosen != tzZone) {
      applyTimeZone(chosen);
      writeTimeZone(tzZone);
      // The zone change moves the wall clock, so resync rather than wait
      // out the sync interval showing the old local time.
      time_t moved = localClockProvider();
      if (moved) {
        setTime(moved);
      }
      nightCheck();
    }
  }
  if (server.hasArg("alarmoff")) {
    nightCheck();
    alarmtick.detach();
    Serial.print("alarmoff has triggered");
  }

  if (server.hasArg("showsecondshidden")) {
    showseconds = server.hasArg("showseconds");
    EEPROM.write(184, showseconds);
  }
  if (server.hasArg("dawnbreakhidden")) {
    dawnbreak = server.hasArg("dawnbreak");
    EEPROM.write(229, dawnbreak);
  }

  if (server.hasArg("clockname")) {
    String tempclockname = server.arg("clockname");
    cleanASCII(tempclockname);

    if (tempclockname != clockname) {
      clockname = tempclockname;

      Serial.println(clockname);
      Serial.println("");
      Serial.println("clearing old clockname.");
      //clear the old clock name out
      for (int i = 195; i < 228; i++) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom clockname.");

      int clockname_len = clockname.length() + 1;
      char clocknamechar[clockname_len];
      clockname.toCharArray(clocknamechar, clockname_len);
      mdnsStarted = MDNS.begin(clocknamechar);
      if (mdnsStarted) {
        Serial.println("mDNS responder started");
        // Only legal once begin() has succeeded.
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ws", "tcp", 81);
      } else {
        // Not fatal: the clock is still reachable by IP address.
        Serial.println("Error setting up MDNS responder!");
      }
      for (int i = 0; i < clockname.length(); ++i) {
        EEPROM.write(195 + i, clockname[i]);
        Serial.print(clockname[i]);
      }
      Serial.println("");
    }
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


  String toSend = FPSTR(root_html);
  String tempgradient = "";
  String csswgradient = "";
  const String scheme = "scheme";
  for (int i = 1; i < 4; i++) {
    //loop makes each of the save/load buttons coloured based on the scheme
    tempgradient = FPSTR(buttongradient_css);
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

    tempgradient.replace("$scheme", scheme + i);

    csswgradient += tempgradient;
  }
  toSend.replace("$csswgradient", csswgradient);
    toSend.replace("$externallinks", FPSTR(externallinks));

  if (clockmode == alarm) {
    toSend.replace("$alarm", "<a class='btn btn-default' href=/?alarmoff=1>Cancel Alarm</a>");
  }
  else {
    toSend.replace("$alarm", "<a class='btn btn-default' href=/alarm>Set Alarm</a>");
  }
  toSend.replace("$minutecolor", rgbToText(minutecolor));
  toSend.replace("$hourcolor", rgbToText(hourcolor));
  toSend.replace("$blendpoint", String(int(blendpoint)));
  toSend.replace("$brightness", String(int(brightness)));
  toSend.replace("$maxBrightness", String(int(maxBrightness)));
  server.send(200, "text/html", toSend);

  Serial.println("Sending handleRoot");
  EEPROM.commit();
  delay(300);
}

void nightCheck() {
  if ((hour() == sleep && minute() >= sleepmin) || (hour() == wake && minute() < wakemin)) {
    clockmode = night;
  } else if (sleep < wake && hour() > sleep && hour() < wake) {
    clockmode = night;
  } else if (sleep > wake && (hour() > sleep || hour() < wake)) {
    clockmode = night;
  } else {
    clockmode = normal;
  }
  Serial.print("clockmode ");
  Serial.println(clockmode);
}
void handleSettings() {
  Serial.println("Sending handleSettings");
  String toSend = FPSTR(settings_html);
  for (int i = 0; i < 5; i++) {
    if (i == hourmarks) {
      toSend.replace("$hourmarks" + String(i), "selected");
    } else {
      toSend.replace("$hourmarks" + String(i), "");
    }
  }

  for (int i = 0; i < 5; i++) {
    if (i == sleeptype) {
      toSend.replace("$sleeptype" + String(i), "selected");
    } else {
      toSend.replace("$sleeptype" + String(i), "");
    }
  }

  if (pixelCount == 120 && maxBrightness == 255) { //check if we're in "normal" light clock mode
    toSend.replace("$original", "selected");
    toSend.replace("$mini", "");
    toSend.replace("$customtype", "");
    toSend.replace("$customvisible", "none");
  } else {

    if (pixelCount == 60 && maxBrightness == 100) { //check if we're in mini light clock mode
      toSend.replace("$original", "");
      toSend.replace("$mini", "selected");
      toSend.replace("$customtype", "");
      toSend.replace("$customvisible", "none");
    } else {
      toSend.replace("$original", "");
      toSend.replace("$mini", "");
      toSend.replace("$customtype", "selected");
      toSend.replace("$customvisible", "block");
    }
  }
    toSend.replace("$externallinks", FPSTR(externallinks));
  toSend.replace("$pixelCount", String(pixelCount));
  if (maxBrightness == 100) {
    toSend.replace("$maxbright100", "selected");
    toSend.replace("$maxbright255", "");
  } else {
    toSend.replace("$maxbright100", "");
    toSend.replace("$maxbright255", "selected");
  }
  String ischecked;
  showseconds ? ischecked = "checked" : ischecked = "";
  toSend.replace("$showseconds", ischecked);
  dawnbreak ? ischecked = "checked" : ischecked = "";
  toSend.replace("$dawnbreak", ischecked);
  Serial.print("sleep: ");
  Serial.println(timeToText(sleep, sleepmin));
  Serial.print("wake: ");
  Serial.println(timeToText(wake, wakemin));
  toSend.replace("$sleep", timeToText(sleep, sleepmin));
  toSend.replace("$wake", timeToText(wake, wakemin));
  toSend.replace("$zonesummary", tzSummaryText());
  toSend.replace("$clockname", String(clockname));


  server.send(200, "text/html", toSend);

}

void handleTimezone() {
  String toSend = FPSTR(timezone_html);
  toSend.replace("$zoneindex", String(tzZone));
  toSend.replace("$zonesummary", tzSummaryText());

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
  String toSend = FPSTR(clearromsure_html);
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
  //Serial.println("Updating Face");
  int hour_pos;
  int min_pos;

  switch(speed) {
    case 0:
      hour_pos = ((hour() % 12) * pixelCount / 12 + minute() * pixelCount / 720);
      min_pos = (minute() * pixelCount / 60 + second() * pixelCount / 3600);
    break;

    case 1:
      hour_pos = ((minute() % 12) * pixelCount / 12 + second() * pixelCount / 720);
      min_pos = (second() * pixelCount / 60);
    break;

    case 2:
      hour_pos = ((10 % 12) * pixelCount / 12 + 10 * pixelCount / 720);
      min_pos = (10 * pixelCount / 60 + 0 * pixelCount / 3600);;
    break;
  }


  //Serial.println("Main Switch");
  switch (clockmode) {



    case night:

      switch (sleeptype) {
        case black:
          for (int i = 0; i < pixelCount; i++) {
            clockleds->SetPixelColor(i, 0, 0, 0);
          }

          break;
        case dots:
          for (int i = 0; i < pixelCount; i++) {
            clockleds->SetPixelColor(i, 0, 0, 0);
          }
          clockleds->SetPixelColor(hour_pos, hourcolor, (std::min)(30, brightness));
          clockleds->SetPixelColor(min_pos, minutecolor, (std::min)(30, brightness));

          break;

        case dim:
          face(hour_pos, min_pos, 4);
          break;

        case moonphase:
          moon();
          clockleds->SetPixelColor((hour_pos + 1 + pixelCount) % pixelCount, 0, 0, 0);
          clockleds->SetPixelColor((hour_pos - 1 + pixelCount) % pixelCount, 0, 0, 0);
          clockleds->SetPixelColor((min_pos + 1 + pixelCount) % pixelCount, 0, 0, 0);
          clockleds->SetPixelColor((min_pos - 1 + pixelCount) % pixelCount, 0, 0, 0);
          clockleds->SetPixelColor(hour_pos, hourcolor, (std::min)(30, brightness));
          clockleds->SetPixelColor(min_pos, minutecolor, (std::min)(30, brightness));
          break;

        case disabled:
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



      break;


    case alarm:
      alarmface();
      break;
    case game:
      gameface();
      break;

    case normal:

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
      break;

    case dawnmode:
      dawn(dawnprogress);
      clockleds->SetPixelColor((hour_pos + 1 + pixelCount) % pixelCount, 0, 0, 0);
      clockleds->SetPixelColor((hour_pos - 1 + pixelCount) % pixelCount, 0, 0, 0);
      clockleds->SetPixelColor((min_pos + 1 + pixelCount) % pixelCount, 0, 0, 0);
      clockleds->SetPixelColor((min_pos - 1 + pixelCount) % pixelCount, 0, 0, 0);
      clockleds->SetPixelColor(hour_pos, hourcolor, (std::min)(30, brightness));
      clockleds->SetPixelColor(min_pos, minutecolor, (std::min)(30, brightness));


  }


  //Serial.println("Show LEDS");


}

void face(uint16_t hour_pos, uint16_t min_pos) {

  face(hour_pos, min_pos, brightness);
}

void face(uint16_t hour_pos, uint16_t min_pos, int bright) {
  //this face colours the clock in 2 sections, the c1->c2 divide represents the minute hand and the c2->c1 divide represents the hour hand.
  HslColor c1;
  HslColor c1blend;
  HslColor c2;
  HslColor c2blend;




  int gap;
  int firsthand = (std::min)(hour_pos, min_pos);
  int secondhand = _max(hour_pos, min_pos);
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
  c2blend = c2blend.LinearBlend(c2, c1, (float)blendpoint / 255);
  c1blend = c1blend.LinearBlend(c1, c2, (float)blendpoint / 255);

  gap = secondhand - firsthand;

  //create the blend between first and 2nd hand
  for (uint16_t i = firsthand; i < secondhand; i++) {
    clockleds->SetPixelColor(i, HslColor::LinearBlend(c2blend, c2, ((float)i - (float)firsthand) / (float)gap), bright);
  }
  gap = pixelCount - gap;
  //and the last hand
  for (uint16_t i = secondhand; i < pixelCount + firsthand; i++) {
    clockleds->SetPixelColor(i % pixelCount, HslColor::LinearBlend(c1blend, c1, ((float)i - (float)secondhand) / (float)gap), bright);
  }
  clockleds->SetPixelColor(hour_pos, hourcolor, bright);
  clockleds->SetPixelColor(min_pos, minutecolor, bright);
}

void nightface(uint16_t hour_pos, uint16_t min_pos) {
  for (int i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, 0, 0, 0);
  }
  clockleds->SetPixelColor(hour_pos, hourcolor, (std::min)(30, brightness));
  clockleds->SetPixelColor(min_pos, minutecolor, (std::min)(30, brightness));

}

void alarmface() {
  RgbColor redblack;
  if (alarmprogress == pixelCount) {//flash the face when alarm is finished
    (second() % 2) ? redblack = alarmcolor : redblack = RgbColor(0, 0, 0);
    for (int i = 0; i < pixelCount; i++) {
      clockleds->SetPixelColor(i, redblack);
    }
  }
  else {
    for (int i = 0; i < alarmprogress; i++) {
      clockleds->SetPixelColor(i, 0, 0, 0);
    }
    for (int i = alarmprogress; i < pixelCount; i++) {
      clockleds->SetPixelColor(i, alarmcolor);
    }
  }


}


void alarmadvance() {
  //Serial.println("advancing alarm");

  if (alarmprogress != pixelCount) {
    alarmprogress++;
    updateface();

  } else {
    alarmtick.detach();
  }
  //    alarmtick.attach(0.3, flashface);
  //    alarmprogress = 0;
  //
  //  }
  //
}

//void flashface() {
//  alarmmode = 2;
//  if (alarmprogress == 10) {
//    alarmtick.detach();
//    alarmprogress = 0;
//    clockmode = normal;
//  } else {
//    if ((alarmprogress % 2) == 0) {
//      for (int i = 0; i < pixelCount; i++) {
//        clockleds->SetPixelColor(i, 255, 0, 0);
//      }
//    } else {
//      for (int i = 0; i < pixelCount; i++) {
//        clockleds->SetPixelColor(i, 0, 0, 0);
//      }
//    }
//  }
//
//  alarmprogress++;
//  updateface();
//}

void invertLED(int i) {
  //This function will set the LED to in inverse of the two LEDs next to it showing as white on the main face
  RgbColor averagecolor;
  averagecolor = RgbColor::LinearBlend(clockleds->GetPixelColor((i - 1) % pixelCount), clockleds->GetPixelColor((i + 1) % pixelCount), 0.5);
  averagecolor = RgbColor(255 - averagecolor.R, 255 - averagecolor.G, 255 - averagecolor.B);
  clockleds->SetPixelColor(i, averagecolor, brightness);
}

void showHourMarks() {
  //shows white at the four quadrants and darkens each hour mark to help the user tell the time
  //  RgbColor c;
  //  for (int i = 0; i < 12; i++) {
  //    c = clockleds->GetPixelColor(i);
  //    c.Darken(255);
  //    clockleds->SetPixelColor(i * pixelCount / 12, c,brightness);
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
  int secondhand = _max(hour_pos, min_pos);
  RgbColor c;
  for (uint16_t i = secondhand; i < pixelCount; i++) {
    c = clockleds->GetPixelColor(i);
    c.Darken(240);
    clockleds->SetPixelColor(i, c);
  }
}

//void nightModeAnimation() {
//  //darkens the pixels animation to switch to nightmode.
////  int firsthand = (std::min)(hour_pos, min_pos);
////  int secondhand = (max)(hour_pos, min_pos);
////  int firsthandlen = (120+firsthand-secondhand)%120;
////  int secondhandlen = 120-firsthandlen;
//
//
//
//  RgbColor c;
//
//  for (uint16_t i = 0; i < 240; i++) {
//    for (uint16_t j = 0; j < (std::min)(i, (uint16_t)120); i++) {
//    c = clockleds->GetPixelColor(i);
//    c.Darken(20);
//    clockleds->SetPixelColor(i, c);
//
//    }
//
//    delay(10);
//  }
//}

void logo() {
  //this lights up the clock as the C logo
  //yellow section
  for (int i = 14 / (360 / pixelCount); i < 48 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, 100, 100, 0);
  }

  //blank section
  for (int i = 48 / (360 / pixelCount); i < 140 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, 0, 0, 0);
  }

  //blue section
  for (int i = 140 / (360 / pixelCount); i < 296 / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i, 0, 60, 120);
  }

  //green section
  for (int i = 296 / (360 / pixelCount); i < (360 + 14) / (360 / pixelCount); i++) {
    clockleds->SetPixelColor(i % pixelCount, 30, 120, 0);
  }


}

void pulseBrightness() {
  pulseBrightnessCounter++;
  if(pulseBrightnessCounter == 10){
    pulseBrightnessCounter = 0;
    brightness = brightness+18;
  } else {
    brightness = brightness -2;
  }
  updateface();

}

void sparkles() {
  updateface();
  int darkled[pixelCount];
  memset(darkled, 0 , sizeof(darkled));//initialize all leds to off


    for (int i = 0; i< pixelCount * 0.75; i++){
      int ledToTurnOn = random(pixelCount-i); // choose a random pixel to turn on from the remaining off pixels
      int k = 0;
      while(k <= ledToTurnOn){
        ledToTurnOn += darkled[k]; // skip over the already on LEDs
        k++;
      }
      darkled[ledToTurnOn] = 1;
    }


      for (int j = 0; j < pixelCount; j++) {
        if(darkled[j]==0){
          clockleds->SetPixelColor(j, 0, 0, 0); //blacken the LED if it's dark in the array
        }
      }


}

void dawnadvance() {
  if (dawnprogress == 255) {
    clockmode = normal;
    dawntick.detach();
    dawnprogress = 0;
  }
  else {
    dawnprogress++;
  }
}
void dawn(int i) {//this sub will present a dawning sun with the time highlighted in dots. I should vary from 0 to 255
  RgbColor  c1 = RgbColor(255, 142, 0);
  int bright;
  int green;
  int blue = 0;

  if (i < 142) {
    bright = i * 64 / 142;
  } else if (i >= 142 && i < 204) {
    bright = 64 + (i - 142) * 128 / 62;
  } else {
    bright = 192 + (i - 204) * 64 / 51;
  }



  green = _max(142, i);

  if (i > 204) {
    blue = (5 * i - 1020);
  } else {
    blue = 0;
  }

  for (int j = 0; j < pixelCount; j++) {
    if (j < (i * pixelCount / 280) || j > (pixelCount - (i * pixelCount / 280))) {
      clockleds->SetPixelColor(j, RgbColor(255, green, blue) , bright);
    }
    else {
      clockleds->SetPixelColor(j, 0, 0, 0);
    }
  }

}
void dawntest() {
  RgbColor  c1 = RgbColor(255, 142, 0);
  int bright;
  int green;
  int blue = 0;

  for (int i = 0; i < 255; i++) {

    if (i < 142) {
      bright = i * 64 / 142;
    } else if (i >= 142 && i < 204) {
      bright = 64 + (i - 142) * 128 / 62;
    } else {
      bright = 192 + (i - 204) * 64 / 51;
    }



    green = _max(142, i);

    if (i > 204) {
      blue = (5 * i - 1020);
    } else {
      blue = 0;
    }

    for (int j = 0; j < pixelCount; j++) {
      if (j < (i * pixelCount / 280) || j > (pixelCount - (i * pixelCount / 280))) {
        clockleds->SetPixelColor(j, RgbColor(255, green, blue) , bright);
      }
      else {
        clockleds->SetPixelColor(j, 0, 0, 0);
      }
    }

    clockleds->Show();
    delay(100);
  }
  for (int j = 0; j < pixelCount; j++) {
    clockleds->SetPixelColor(j, 20, 20, 20);

  }

}

void moontest() {
  int lp = 2551443;//moon phase length in seconds(?)
  int new_moon = 518400;//
  int phase = floor(((now() - new_moon) % lp) / (24 * 3600)); //time since new moon div moon phase len
  Serial.print("phase: ");
  Serial.println(phase);

  for (phase = 0; phase < 28; phase++) {
    int fill = pixelCount * (14 - (abs(14 - phase))) / 14 - pixelCount / 6; //how full is the moon based on the phase
    Serial.print("fill: ");
    Serial.println(fill);

    int startPos = pixelCount / 5 + fill / 2 + pixelCount / 2 * (phase > 14); //start on one side, then go back to the other
    for (int i = 0; i < pixelCount; i++) {
      if (i < fill) {
        clockleds->SetPixelColor((i + 2 * pixelCount - startPos) % pixelCount, 64, 64, 64); //fill the LEDs in the zone at moon brightness
      }
      else {
        int bright = _max(64 - (pixelCount - i) * (64 / (pixelCount / 6)), _max(0, (64 - (i - fill) * (64 / (pixelCount / 6))))); //check if these LEDs are on either side of full-bright and make them semi-bright
        Serial.print("bright: ");
        Serial.println(bright);
        clockleds->SetPixelColor((i + 2 * pixelCount - startPos) % pixelCount, bright, bright, bright); //add in start pos and % to offset to one side
      }
    }
    clockleds->Show();
    delay(1000);
  }

}
void moon() {
  int lp = 2551443;//moon phase length in seconds(?)
  int new_moon = 518400;//
  int phase = floor(((now() - new_moon) % lp) / (24 * 3600)); //time since new moon div moon phase len


  int fill = pixelCount * (14 - (abs(14 - phase))) / 14 - pixelCount / 6; //how full is the moon based on the phase

  int startPos = pixelCount / 5 + fill / 2 + pixelCount / 2 * (phase > 14); //start on one side, then go back to the other
  for (int i = 0; i < pixelCount; i++) {
    if (i < fill) {
      clockleds->SetPixelColor((i + 2 * pixelCount - startPos) % pixelCount, 64, 64, 64); //fill the LEDs in the zone at moon brightness
    }
    else {
      int bright = _max(64 - (pixelCount - i) * (64 / (pixelCount / 6)), _max(0, (64 - (i - fill) * (64 / (pixelCount / 6))))); //check if these LEDs are on either side of full-bright and make them semi-bright

      clockleds->SetPixelColor((i + 2 * pixelCount - startPos) % pixelCount, bright, bright, bright); //add in start pos and % to offset to one side
    }
  }


}

void brighttest() {
  for (int i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, i, i, i);
  }

  delay(10000);
}

void lightup() {
  int darkled[pixelCount];
  memset(darkled, 0 , sizeof(darkled));//initialize all leds to off
  server.send(200, "text/html", "<form class=form-verticle action=/lightup method=GET> Skip check /p <input type=number name=skip>/p <input type=submit name=submit value='Save Settings'/>");
  if (server.hasArg("skip")) {

    String skipstring = server.arg("skip");  //get value input
    int skip = skipstring.toInt();
    randomSeed(skip); //seed just incase we find one we particularly like/don't like
    for (int i = 0; i < pixelCount; i++) {

      int ledToTurnOn = random(pixelCount-i); // choose a random pixel to turn on from the remaining off pixels
      int k = 0;
      while(k <= ledToTurnOn){
        ledToTurnOn += darkled[k]; // skip over the already on LEDs
        k++;
      }
      darkled[ledToTurnOn] = 1;

      face(10, 50);
      for (int j = 0; j < pixelCount; j++) {
        if(darkled[j]==0){
          clockleds->SetPixelColor(j, 0, 0, 0); //blacken the LED if it's dark in the array
        }
      }
      clockleds->Show();
      delay(_max((pow(pixelCount - i, 7) / pow(pixelCount, 7)) * 1000, 40));

    }
  }
  delay(5000);

}
//------------------------------Serial console-----------------------------------
// A way in when the web UI is out of reach -- moved house, new router, wrong
// network -- with nothing but a USB cable. Polled once per loop().

void serialHelp() {
  Serial.println();
  Serial.println("  status      what the clock currently thinks");
  Serial.println("  clearwifi   forget the WiFi network, keep colours and settings, reboot to setup");
  Serial.println("  cleareeprom wipe every saved setting and reboot (asks first)");
  Serial.println("  restart     reboot without changing anything");
  Serial.println("  help        this list");
  Serial.println();
}

void serialStatus() {
  char name[48];
  tzZoneName(tzZone, name, sizeof(name));
  Serial.println();
  Serial.print("  name:      ");
  Serial.println(clockname);
  Serial.print("  web mode:  ");
  Serial.println(webMode);  // 0 = setup AP, 1 = joined WiFi, 2 = local only
  Serial.print("  ssid:      ");
  Serial.println(esid.length() ? esid : String("(none saved)"));
  if (webMode == 1) {
    Serial.print("  ip:        ");
    Serial.println(WiFi.localIP());
  }
  Serial.print("  zone:      ");
  Serial.print(name);
  Serial.print("  ");
  Serial.print(tzOffsetText());
  Serial.println(tzIsSummerTime() ? "  (daylight saving on)" : "");
  Serial.print("  time:      ");
  Serial.println(timeToText(hour(), minute()));
  Serial.println();
}

void handleSerialCommand(String command) {
  static bool awaitingWipeConfirmation = false;
  command.trim();
  command.toLowerCase();

  if (awaitingWipeConfirmation) {
    awaitingWipeConfirmation = false;
    if (command == "yes") {
      Serial.println("clearing EEPROM...");
      clearEEPROM();
      Serial.println("cleared. Restarting into setup mode.");
      delay(200);
      ESP.reset();
    } else {
      Serial.println("cancelled, nothing was cleared.");
    }
    return;
  }

  if (command.length() == 0) {
    return;
  }
  if (command == "help" || command == "?") {
    serialHelp();
  } else if (command == "status") {
    serialStatus();
  } else if (command == "clearwifi") {
    // Everything else -- colours, alarm, zone, clock type -- survives, so
    // the clock only has to be pointed at the new network.
    clearssid();
    clearpass();
    EEPROM.begin(512);
    EEPROM.write(186, 0);  // back to setup mode, so it raises its own AP
    EEPROM.commit();
    delay(200);
    Serial.println("WiFi details cleared. Restarting into setup mode -- look for the clock's own network.");
    delay(200);
    ESP.reset();
  } else if (command == "cleareeprom") {
    awaitingWipeConfirmation = true;
    Serial.println("This wipes every saved setting, including your colours. Type 'yes' to confirm, anything else to cancel.");
  } else if (command == "restart") {
    Serial.println("restarting...");
    delay(200);
    ESP.reset();
  } else {
    Serial.print("unknown command: ");
    Serial.println(command);
    serialHelp();
  }
}

// Reads one line at a time without blocking the LED refresh.
void pollSerialConsole() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      String command = line;
      line = "";
      handleSerialCommand(command);
    } else if (line.length() < 40) {
      line += c;
    }
  }
}

//------------------------------EEPROM save/read functions-----------------------

// Byte 234 says which format bytes 232-233 are in. Bumping it re-runs the
// migration below on clocks that were flashed before this layout existed.
#define TZ_CONFIG_VERSION 1

// Stores the zone index. Caller commits.
void writeTimeZone(uint16_t zone) {
  EEPROM.write(232, zone & 0xff);
  EEPROM.write(233, (zone >> 8) & 0xff);
  EEPROM.write(234, TZ_CONFIG_VERSION);
}

// Works out which zone an already-configured clock was keeping, from the
// pre-1.2 settings: byte 179 held either a position in the old 82-entry
// GMT-offset dropdown, or -- if the timezonedb lookup had been switched on
// (byte 185) -- a raw UTC offset in hours. Neither knew about DST on its
// own; both map onto a zone that does.
static uint16_t migrateLegacyTimeZone() {
  uint8_t legacy = EEPROM.read(179);
  bool wasAutoOffset = EEPROM.read(185);

  if (wasAutoOffset) {
    // Written from a float, so offsets west of Greenwich wrapped past 255.
    int offset = (legacy > 128) ? (int)legacy - 256 : (int)legacy;
    if (offset >= -12 && offset <= 14) {
      // Etc/GMT signs run the opposite way to UTC offsets: UTC+10 is Etc/GMT-10.
      String name = F("Etc/GMT");
      name += (offset <= 0) ? '+' : '-';
      name += abs(offset);
      if (offset == 0) {
        name = F("UTC");
      }
      uint16_t found = tzZoneByName(name);
      if (found < tzZoneCount) {
        return found;
      }
    }
    return tzDefaultZone;
  }

  if (legacy >= 1 && legacy <= tzLegacyZoneCount) {
    return pgm_read_word(&tzLegacyZone[legacy - 1]);
  }
  return tzDefaultZone;
}

// Loads the saved zone, migrating a pre-1.2 setting the first time.
void readTimeZoneConfig() {
  if (EEPROM.read(234) == TZ_CONFIG_VERSION) {
    tzZone = EEPROM.read(232) | (EEPROM.read(233) << 8);
    if (tzZone >= tzZoneCount) {
      tzZone = tzDefaultZone;
    }
  } else {
    tzZone = migrateLegacyTimeZone();
    Serial.println("migrating time zone from the pre-1.2 settings");
    writeTimeZone(tzZone);
    EEPROM.commit();
    delay(10);
  }
  char name[48];
  tzZoneName(tzZone, name, sizeof(name));
  Serial.print("time zone: ");
  Serial.println(name);
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
  clockmode = normal;
  setTime(21, 59, 50, 1, 1, 1);
  sleep = 22;
  sleepmin = 0;
  server.send(200, "text/html", "demo of night mode");
}

void webHandleGame() {
  String toSend = FPSTR(game_html);
    toSend.replace("$externallinks", FPSTR(externallinks));
  toSend.replace("$playercolor", rgbToText(playercolors[nextplayer]));
  server.send(200, "html", toSend);
}

void webHandleTimeSet() {

  if (server.hasArg("time")) {
    String timestring = server.arg("time");  //get value input
    int timehr = timestring.substring(0, 2).toInt(); //atoi(c);  //get first section of string for hours
    int timemin = timestring.substring(3, 5).toInt(); //atoi(c);  //get second section of string for minutes


    Serial.print("Time Total: ");
    Serial.println(timestring);
    Serial.print("Time Hour: ");
    Serial.println(timehr);
    Serial.print("Time Minute: ");
    Serial.println(timemin);

    setTime(timehr, timemin, 0, 1, 1, 1);
  }

  server.send(200, "text/html", "<form class=form-verticle action=/timeset method=GET> Time Reset /p <input type=time name=time value=" + timeToText((int)hour(), (int)minute()) + ">/p <input type=submit name=submit value='Save Settings'/>");

}

void webHandleReflection() {
  if (testrun == 3) {
    testrun = 0;
    server.send(200, "text", "Clock has been set to normal mode.");
  }
  else {
    testrun = 3;
    server.send(200, "text", "Clock has been set to reflection mode.");
  }
}

void webHandleDawn() {
  dawntest();
  server.send(200, "text", "test dawn");
}

void webHandleMoon() {
  moontest();
  server.send(200, "text", "test moon");
}

void webHandleAlarm() {
  String toSend = FPSTR(alarm_html);
    toSend.replace("$externallinks", FPSTR(externallinks));
  server.send(200, "html", toSend);

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
//----------------------------------------Time zone functions----------------------------------------------------------------
// The clock works out daylight saving for itself. Each zone in h/tzdata.h
// carries a POSIX TZ rule ("AEST-10AEDT,M10.1.0,M4.1.0/3") that spells out
// when summer time starts and ends, and newlib applies it to every
// localtime_r() call -- so the changeover happens on the right night without
// anyone touching a switch.

// Copies zone `idx`'s IANA name out of PROGMEM. Safe for any idx.
void tzZoneName(uint16_t idx, char* out, size_t len) {
  TzZone zone;
  memcpy_P(&zone, &tzZones[idx < tzZoneCount ? idx : tzDefaultZone], sizeof(zone));
  strncpy_P(out, zone.name, len - 1);
  out[len - 1] = 0;
}

// Copies zone `idx`'s POSIX rule out of PROGMEM. Safe for any idx.
void tzZoneRule(uint16_t idx, char* out, size_t len) {
  TzZone zone;
  memcpy_P(&zone, &tzZones[idx < tzZoneCount ? idx : tzDefaultZone], sizeof(zone));
  strncpy_P(out, zone.posix, len - 1);
  out[len - 1] = 0;
}

// Points SNTP at the pool and hands newlib the chosen zone's rule. Cheap
// enough to call again whenever the user picks a different place.
void applyTimeZone(uint16_t idx) {
  tzZone = (idx < tzZoneCount) ? idx : tzDefaultZone;
  char rule[48];
  char name[48];
  tzZoneRule(tzZone, rule, sizeof(rule));
  tzZoneName(tzZone, name, sizeof(name));
  Serial.print("time zone: ");
  Serial.print(name);
  Serial.print(" -> ");
  Serial.println(rule);
  configTime(rule, ntpServer1, ntpServer2);
}

// UTC as UTC. Zero until SNTP has heard back for the first time.
static time_t utcNow() {
  time_t utc = time(nullptr);
  return (utc < 1000000000UL) ? 0 : utc;  // anything before 2001 is the boot value
}

// Local wall-clock time, as the epoch value TimeLib wants from a sync
// provider. This is where the zone's DST rule actually gets applied.
time_t localClockProvider(void) {
  time_t utc = utcNow();
  if (utc == 0) {
    return 0;  // tells TimeLib to leave the clock alone and ask again later
  }
  struct tm local;
  localtime_r(&utc, &local);
  tmElements_t parts;
  parts.Second = local.tm_sec;
  parts.Minute = local.tm_min;
  parts.Hour = local.tm_hour;
  parts.Day = local.tm_mday;
  parts.Month = local.tm_mon + 1;
  parts.Year = local.tm_year + 1900 - 1970;
  return makeTime(parts);
}

// True while summer time is in force here, right now.
bool tzIsSummerTime() {
  time_t utc = utcNow();
  if (utc == 0) {
    return false;
  }
  struct tm local;
  localtime_r(&utc, &local);
  return local.tm_isdst > 0;
}

// The zone's current offset from UTC, formatted the way people write it:
// "UTC+11", "UTC-3:30", "UTC".
String tzOffsetText() {
  time_t utc = utcNow();
  if (utc == 0) {
    return String(F("not synced yet"));
  }
  long offset = (long)(localClockProvider() - utc);
  char sign = offset < 0 ? '-' : '+';
  if (offset < 0) {
    offset = -offset;
  }
  int hours = offset / 3600;
  int minutes = (offset % 3600) / 60;
  if (hours == 0 && minutes == 0) {
    return String(F("UTC"));
  }
  String text = F("UTC");
  text += sign;
  text += hours;
  if (minutes) {
    text += ':';
    text += (minutes < 10 ? "0" : "");
    text += minutes;
  }
  return text;
}

// Finds a zone by IANA name. Returns tzZoneCount when there is no match,
// which is what the browser sending us a name our TZ.h predates looks like.
uint16_t tzZoneByName(const String& wanted) {
  char name[48];
  for (uint16_t i = 0; i < tzZoneCount; i++) {
    tzZoneName(i, name, sizeof(name));
    if (wanted.equals(name)) {
      return i;
    }
  }
  return tzZoneCount;
}

// One line for the settings page: where the clock thinks it is, what that
// means right now, and -- the whole point of the change -- whether it has
// put itself on summer time.
String tzSummaryText() {
  char name[48];
  tzZoneName(tzZone, name, sizeof(name));
  String text = name;
  text += F(" (");
  text += tzOffsetText();
  if (tzIsSummerTime()) {
    text += F(", daylight saving on");
  }
  text += ')';
  return text;
}

// Reads the ?zone= argument. The picker sends an index into tzZones; a name
// like "Europe/Lisbon" is also accepted so anything holding an IANA name --
// a browser, a script -- can set the zone without knowing our numbering.
uint16_t parseZoneArg() {
  String arg = server.arg("zone");
  arg.trim();
  if (arg.length() == 0) {
    return tzZone;
  }
  if (isDigit(arg[0])) {
    long index = arg.toInt();
    if (index >= 0 && index < tzZoneCount) {
      return (uint16_t)index;
    }
    return tzZone;
  }
  uint16_t found = tzZoneByName(arg);
  return (found < tzZoneCount) ? found : tzZone;
}

// The zone list, newline separated, in index order -- the picker turns line
// N into the option with value N. Streamed a chunk at a time: the whole list
// is around 9 KB and materialising that as a String is more than the heap
// wants to give up in one piece.
void handleTimezoneList() {
  server.sendHeader(F("Cache-Control"), F("max-age=86400"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/plain"), F(""));

  String chunk;
  chunk.reserve(600);
  char name[48];
  for (uint16_t i = 0; i < tzZoneCount; i++) {
    tzZoneName(i, name, sizeof(name));
    chunk += name;
    chunk += '\n';
    if (chunk.length() > 512) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  if (chunk.length()) {
    server.sendContent(chunk);
  }
  server.sendContent(F(""));
}

void ChangeNeoPixels(uint16_t count, uint8_t pin)
{
  if (clockleds)
  {
    delete clockleds;
  }
  clockleds = new NeoPixelBus(count, pin);
  clockleds->Begin();
}
//-----------------------------------------------------------------------------------------------------websocket stuff------------------------------------------------------------------------------------------------------

String wsHead(String input){
  int headend = find_text("|", input);
  return input.substring(0,headend);
}

String wsValue(String input){
  int valuestart = find_text("|", input)+1;
  return input.substring(valuestart);
}

int find_text(String needle, String haystack) {
  int foundpos = -1;
  for (int i = 0; i <= haystack.length() - needle.length(); i++) {
    if (haystack.substring(i,needle.length()+i) == needle) {
      foundpos = i;
    }
  }
  return foundpos;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

 switch (type) {
   case WStype_DISCONNECTED:
     Serial.printf("[%u] Disconnected!\n", num);
     break;
   case WStype_CONNECTED: {
       IPAddress ip = webSocket.remoteIP(num);
       Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

       // send message to client
       webSocket.sendTXT(num, "Connected");
     }
     break;
   case WStype_TEXT:

     String value = wsValue((char*)payload);
     String head = wsHead((char*)payload);

     if(head=="hourcolor"){
        getRGB(value, hourcolor);
     }
     if(head=="minutecolor"){
        getRGB(value, minutecolor);
     }
     if(head=="brightness"){
       brightness = (int)value.toInt();
     }
     if(head=="blendpoint"){
       blendpoint = (uint8_t)value.toInt();
     }
     if(head=="newplayer"){

       gamejoin(num);
     }
     if(head=="gamestart"){
       gamestart();
     }
     if(head=="gameplus"){
       gameplus(num);
     }


     break;
 }

}


//========================================GAME FUNCTIONS======================================================================

void gamestart(){
  Serial.println("start command received");
  gamestartticker.attach_ms(50, gamecountdown);

}

void gamecountdown(){
  Serial.print("Gamebrightness: ");
  Serial.println(gamebrightness);
  gamebrightness = gamebrightness - (maxBrightness/50);
  if (gamebrightness<=0) {
    gamestarted = 1;
    gamestartticker.detach();
    gamebrightness = maxBrightness;
  }
}

void gamejoin(int num){
  if (gamestarted == 0) {
    gamebrightness = maxBrightness;
    nextplayer++;
    gamearray[num] = gamestartpoints;
    clockmode = game;
    playercount = 0;
    for(int i=0; i<6; i++){
      if(gamearray[i]==gamestartpoints){
        playercount++;
      }
    }
  }
}
void gameplus(int playernum){
  if(gamestarted==1){
    if(gamearray[playernum]>0){//if your score is 0 or less then you're eliminated
      for(int i=0; i<playercount; i++){
        if(playernum == i){
          gamearray[i] += (playersremaining-1);//add to the clicking players score a point for each opponant
        } else {
          gamearray[i]--;//take that point off everyone else
          gamearray[i] = _max(gamearray[i],0);
        }
      }
    }

    playersremaining = 0;//check if we have a winner
    int winner = 0;
    for(int i=0; i<playercount; i++){
      if(gamearray[i]>0){
        winner = i;
        playersremaining++;
      }
    }
    if(playersremaining <= 1){
      animatewinner(winner);
    }

    //debug
    int accumulatedscore=gamearray[0];
    int totalpoints=playercount*gamestartpoints;
    for (size_t i = 0; i < playercount; i++) {

      Serial.print("Player ");
      Serial.print(i);
      Serial.print(" score: ");
      Serial.print(gamearray[i]);
      Serial.print(" animate to: ");
      Serial.println((int)((float)accumulatedscore/(float)totalpoints*pixelCount));
      accumulatedscore+=gamearray[i+1];
    }
  }

}

void animatewinner(int winner){
  for (size_t i = 0; i < 6; i++) {
    gamearray[i]=0;
    gamestarted=0;
    nextplayer=0;
  }
  for (size_t i = 0; i < pixelCount; i++) {
    clockleds->SetPixelColor(i, playercolors[winner]);
  }

  delay(1000);
  nightCheck();
}
void gameface(){
int playeranimating = 0;
int accumulatedscore = gamearray[0];
int totalpoints = playercount*gamestartpoints;
  if(gamestarted==0){
    for (size_t i = 0; i < pixelCount; i++) {
      if(i < ((playeranimating + 1) * pixelCount/playercount)){
        clockleds->SetPixelColor(i, playercolors[playeranimating], gamebrightness);
      } else {
        playeranimating++;
      }
    }
  } else {
    for (size_t i = 0; i < pixelCount; i++) {
      if(i < ((float)accumulatedscore/(float)totalpoints*pixelCount)){
        clockleds->SetPixelColor(i, playercolors[playeranimating]);
      } else {
        playeranimating++;
        accumulatedscore += gamearray[playeranimating];
      }
    }
  }

}
