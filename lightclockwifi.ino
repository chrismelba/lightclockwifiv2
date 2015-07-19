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


#define clockPin 4                //GPIO pin that the LED strip is on
#define pixelCount 120            //number of pixels in RGB clock



IPAddress dns(8, 8, 8, 8);  //Google dns  
String clientName ="TheLightClock"; //The MQTT ID -> MAC adress will be added to make it kind of unique
String ssid = "The Light Clock"; //The ssid when in AP mode
MDNSResponder mdns;
ESP8266WebServer server(80);

NeoPixelBus clock = NeoPixelBus(pixelCount, clockPin);  //Clock Led on Pin 4
time_t getNTPtime(void);
NTP NTPclient;
Ticker NTPsyncclock;
WiFiClient DSTclient;

const char* DSTTimeServer = "api.timezonedb.com";

float latitude = -36;
float longitude = 146;

String FQDN ="WiFiSwitch.local"; //The DNS hostname - Does not work yet?

const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec
int webtypeGlob; //if we are in AP or SOFT_AP mode
const int debug = 0; //Set to one to get more log to serial
bool updateTime = true;
unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
int testrun;

//To be read from EEPROM Config
String esid = "";
String epass = "";

RgbColor minutecolor = RgbColor(255, 255, 0); //starting colour of minute
RgbColor hourcolor = RgbColor(0, 0, 255); // starting colour of hour
float blendpoint = 0.4; //level of default blending
int hourmarks = 1;
int sleep = 23;
int wake = 7;
int timezone = 10; //Australian Eastern Standard Time
bool showseconds =0;



int prevsecond;


IPAddress apIP(192, 168, 1, 1);        //FOR AP mode
IPAddress netMsk(255,255,255,0);         //FOR AP mode



//-----------------------------------standard arduino setup and loop-----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  clock.Begin();   
  logo();
  clock.Show();

  
  loadConfig();

  initWiFi();
  delay(1000);
  //initialise the NTP clock sync function
  NTPclient.begin("2.au.pool.ntp.org", timezone);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);
  
  prevsecond =second();
  //readDSTtime(); //function doesn't work over HTTPS so can't access API currently
}

void loop() {

  server.handleClient();
  delay(50);
  if(second()!=prevsecond) {
    updateface();
    prevsecond = second();
  }
}

//----------------------------------------DST adjusting functions------------------------------------------------------------------
void connectToDSTServer() {
  String GETString;
  // attempt to connect, and wait a millisecond:

  Serial.println("Connecting to DST server");
  DSTclient.connect("api.geonames.org", 80);
  
  if (DSTclient.connect("api.geonames.org", 80)) {
    // make HTTP GET request to twitter:
    GETString += "GET /?lat=";
    GETString += latitude;
    GETString += "&lng=";
    GETString += longitude;
    GETString += "&key=N9XTPTVFZJFN\r\n";
    Serial.println(GETString + "HTTP/1.1\r\n" + "Host: api.thetimezonedb.com \r\n"+"Connection: close\r\n\r\n");


    DSTclient.print("/timezone?lat=47.01&lng=10.2&username=thelightclock\r\n");// + "Host: api.thetimezonedb.com \r\n"+"Connection: close\r\n\r\n");
    DSTclient.print("HTTP/1.1\r\n");
    DSTclient.print("Accept: */*\r\n"); 
    DSTclient.print("Accept-Encoding: identity\r\n");
    DSTclient.print("Host: api.geonames.org\r\n");
    //DSTclient.print("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n");
    DSTclient.print("Connection: close\r\n\r\n");

    int i=0;
    while((!DSTclient.available()) && (i<1000)){
      delay(10);
      i++;
    }
  }
}

void readDSTtime() {
  String currentLine = "";
  bool readingUTCOffset = false;
  String UTCOffset;
  connectToDSTServer();
  Serial.print("DST.connected: ");
  Serial.println(DSTclient.connected());
  Serial.print("DST.available: ");
  Serial.println(DSTclient.available());
  
  while(DSTclient.connected()) {
    if(DSTclient.available()) {
      
      // read incoming bytes:
      char inChar = DSTclient.read();
      // add incoming byte to end of line:
      currentLine += inChar; 
      
      // if you get a newline, clear the line:
      if (inChar == '\n') {
        
      Serial.println(currentLine);
        currentLine = "";
      } 
      // if the current line ends with <text>, it will
      // be followed by the tweet:
      if ( currentLine.endsWith("<gmtOffset>")) {
        // tweet is beginning. Clear the tweet string:
        
        Serial.println(currentLine);
        readingUTCOffset = true; 
        UTCOffset = "";
      }
      // if you're currently reading the bytes of a tweet,
      // add them to the tweet String:
      if (readingUTCOffset) {
        if (inChar != '<') {
          UTCOffset += inChar;
        } 
        else {
          // if you got a "<" character,
          // you've reached the end of the tweet:
          readingUTCOffset = false;
          Serial.print("UTC Offset in seconds: ");
          Serial.println(UTCOffset);   
          // close the connection to the server:
          DSTclient.stop(); 
        }
      }
    }
  }
}   




//--------------------set up local wifi sections-----------------------------------------------------
void loadConfig(){
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

}

void initWiFi(){
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  esid.trim();
  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.disconnect();
      delay(100);
      WiFi.mode(WIFI_STA);
      Serial.print("Connecting to WiFi ");
      Serial.println(esid);
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          launchWeb(0);
          return;
      }
  }
  setupAP();   
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(500);
    Serial.print(".");    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0){
    Serial.println("no networks found");
    st ="<b>No networks found:</b>";
  } else {
    Serial.print(n);
    Serial.println(" Networks found");
    st = "<ul>";
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" (OPEN)":"*");
      
      // Print to web SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" (OPEN)":"*";
      st += "</li>";
      delay(10);
     }
    st += "</ul>";
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
  WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  Serial.print("Access point started with name ");
  Serial.println(ssid);
  launchWeb(1);
}

//------------------------------------------------------Web handle sections-------------------------------------------------------------------
void launchWeb(int webtype) {
    Serial.println("");
    Serial.println("WiFi connected");    
    //Start the web server or MQTT
     
        if (webtype==1) {           
          webtypeGlob == 1;
          Serial.println(WiFi.softAPIP());
          server.on("/", webHandleConfig);
          server.on("/a", webHandleConfigSave);          
        } else {
          //setup DNS since we are a client in WiFi net
          if (!mdns.begin((char*) FQDN.c_str(), WiFi.localIP())) {
            Serial.println("Error setting up MDNS responder!");
            while(1) { 
              delay(1000);
            }
          } else {
            Serial.println("mDNS responder started");
          }          
          Serial.println(WiFi.localIP());
          server.on("/", handleRoot);  
          server.on("/cleareeprom", webHandleClearRom);
          server.on("/settings", handleSettings);
        }
        //server.onNotFound(webHandleRoot);
        server.begin();
        Serial.println("Web server started");   
        webtypeGlob=webtype; //Store global to use in loop()
}

void webHandleConfig(){
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String s;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += "-";
  clientName += macToStr(mac);
  
  s = "Configuration of " + clientName + " at ";
  s += ipStr;
  s += "<p>";
  s += st;//list of local wifi networks found earlier
  s += "<form method='get' action='a'>";
  s += "<label>SSID: </label><input name='ssid' length=32><label> Pass: </label><input name='pass' type='password' length=64></br>";
  s += "<input type='submit'></form></p>";
  s += "\r\n\r\n";
  Serial.println("Sending 200");  
  server.send(200, "text/html", s); 
}


void webHandleConfigSave(){
  // /a?ssid=blahhhh&pass=poooo
  String s;
  s = "<p>Settings saved to eeprom and reset to boot into new settings</p>\r\n\r\n";
  server.send(200, "text/html", s); 
  Serial.println("clearing EEPROM.");
  clearEEPROM();
  String qsid; 
  qsid = server.arg("ssid");
  qsid.replace("%2F","/");
  qsid.replace("+"," ");
  Serial.println(qsid);
  Serial.println("");

  String qpass;
  qpass = server.arg("pass");
  qpass.replace("%2F","/");
  Serial.println(qpass);
  Serial.println("");
  
  //int addr=0;
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
    
  Serial.println("writing eeprom pass."); 
  //addr += EEPROM.put(addr, qpass);
  for (int i = 0; i < qpass.length(); ++i)
    {
      EEPROM.write(32+i, qpass[i]);
      Serial.print(qpass[i]); 
    }  
  Serial.println("");
    
  
  EEPROM.commit();
  delay(1000);
  EEPROM.end();
  Serial.println("Settings written, restarting!"); 
  ESP.reset();
}

void handleNotFound() {
  Serial.print("\t\t\t\t URI Not Found: ");
  Serial.println(server.uri());
  server.send ( 200,"text/plain","URI Not Found" );
}

void handleRoot() {
  String toSend = root_html;
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
    String blendpointstring = server.arg("blendpoint");  //get value from blend slider
    int blendpointint = blendpointstring.toInt();//atoi(c);  //get value from html5 color element
    blendpoint = (float)blendpointint/100;
  }
  
  toSend.replace("$minutecolor",rgbToText(minutecolor));
  toSend.replace("$hourcolor",rgbToText(hourcolor));
  toSend.replace("$blendpoint",String(int(blendpoint*100)));
  server.send(200, "text/html", toSend);
  delay(100);
}


void handleSettings() {
  server.send(200, "text/html", settings_html);
  
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

//------------------------------------------------animating functions-----------------------------------------------------------

void updateface() {

    int hour_pos;
    int min_pos;
    switch (testrun) {
    case 0:
        // no testing
        hour_pos = (hour() % 12) * pixelCount / 12 + minute()/6;
        min_pos = minute() * pixelCount / 60;

      break;
    case 1:
      //set the face to tick ever second rather than every minute 
      hour_pos = (minute() % 12) * pixelCount / 12 + second()/6;
      min_pos = second() * pixelCount / 60;

      break;
    case 2: 
      //set the face to the classic 10 past 10 for photos
      hour_pos = 10*pixelCount/12;
      min_pos = 10* pixelCount /60;
  }

    if(hour()>=sleep||hour()<wake){
      nightface(hour_pos, min_pos);
    }else{
      face(hour_pos, min_pos);
    }
    
    if(showseconds){
      invertLED(second()*pixelCount/60);
    }

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
    }
    showQuadrants();
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
      clock.SetPixelColor(i,HslColor::LinearBlend(c2blend, c2, ((float)i-(float)firsthand)/(float)gap));      
    }
    gap = 120 - gap;
    //and the last hand
    for(uint16_t i=secondhand; i<pixelCount+firsthand; i++){
      clock.SetPixelColor(i%120,HslColor::LinearBlend(c1blend, c1, ((float)i-(float)secondhand)/(float)gap));// [i%120]=HslColor::LinearBlend(c1blend, c1, ((float)i-(float)secondhand)/(float)gap);
    }
    clock.SetPixelColor(hour_pos,hourcolor);
    clock.SetPixelColor(min_pos,minutecolor);
}

void nightface(uint16_t hour_pos, uint16_t min_pos) {
  clock.SetPixelColor(hour_pos,hourcolor);
  clock.SetPixelColor(min_pos,minutecolor);
  
}

void invertLED(int i){
  //This function will set the LED to in inverse of the two LEDs next to it. Hopefully showing as white on the main face
  RgbColor averagecolor;
  averagecolor = RgbColor::LinearBlend(clock.GetPixelColor((i-1)%pixelCount), clock.GetPixelColor((i+1)%pixelCount),0.5);
  averagecolor = RgbColor(255-averagecolor.R, 255-averagecolor.G, 255-averagecolor.B);
  clock.SetPixelColor(i, averagecolor);
}

void showHourMarks(){
  for(int i=0; i<12; i++){
    invertLED(i*pixelCount/12);
  }
}

void showQuadrants(){
  for(int i=0; i<4; i++){
    invertLED(i*pixelCount/4);
  }
}

void showMidday(){

    invertLED(0);
  
}

void logo(){
  //this lights up the clock as the C logo
    //yellow section
    for (int i = 14/(360/pixelCount); i < 48/(360/pixelCount); i++){
       clock.SetPixelColor(i, 255, 255, 0);
    }

    //blank section
    for (int i = 48/(360/pixelCount); i < 127/(360/pixelCount); i++){
       clock.SetPixelColor(i, 0, 0, 0);
    }

    //blue section
    for (int i = 127/(360/pixelCount); i < 296/(360/pixelCount); i++){
       clock.SetPixelColor(i, 0, 120, 255);
    }

    //green section
    for (int i = 296/(360/pixelCount); i < (360+14)/(360/pixelCount); i++){
       clock.SetPixelColor(i%pixelCount, 60, 255, 0);
    }
    
    clock.Show();
}

//-------------------------------- Help functions ---------------------------

void webHandleClearRom(){
  String s;
  s = "<p>Clearing the EEPROM and reset to configure new wifi<p>";
  s += "</html>\r\n\r\n";
  Serial.println("Sending 200"); 
  server.send(200, "text/html", s); 
  Serial.println("clearing eeprom");
  clearEEPROM();
  delay(10);
  Serial.println("Done, restarting!");
  ESP.reset();
}


void clearEEPROM(){
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++){
    EEPROM.write(i, 0);    
  }
  delay(200);
  EEPROM.commit(); 
  EEPROM.end(); 
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

//------------------------------NTP Functions---------------------------------


time_t getNTPtime(void)
{
  return NTPclient.getNtpTime();
}

