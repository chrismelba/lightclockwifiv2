#include <Time.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>
#include <ntp.h>
#include <Ticker.h>

String clientName ="TheLightClock"; //The MQTT ID -> MAC adress will be added to make it kind of unique
String ssid = "The Light Clock"; //The ssid when in AP mode
#define pixelCount 120            //number of pixels in RGB clock
MDNSResponder mdns;
ESP8266WebServer server(80);
NeoPixelBus clock = NeoPixelBus(pixelCount, 4);  //Clock Led on Pin 4
time_t getNTPtime(void);
NTP NTPclient;
Ticker NTPsyncclock;


String FQDN ="WiFiSwitch.local"; //The DNS hostname - Does not work yet?

const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec
int webtypeGlob; //if we are in AP or SOFT_AP mode
const int debug = 0; //Set to one to get more log to serial

unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list


//To be read from EEPROM Config
String esid = "";
String epass = "";

RgbColor minutecolor = RgbColor(255, 0, 0); //starting colour of minute
RgbColor hourcolor = RgbColor(255, 255, 0); // starting colour of hour
float blendpoint = 0.4; //level of default blending
int prevsecond;


IPAddress apIP(192, 168, 1, 1);        //FOR AP mode
IPAddress netMsk(255,255,255,0);         //FOR AP mode

const char* html = "<html><head><style></style></head><body><form action='/' method='GET'>"
                    "<input type='color' name='hourcolor' value='$hourcolor'/><input type='color' name='minutecolor' value='$minutecolor'/><input type='submit' name='submit' value='Update RGB clock'/>"
                    "<input type='range' name='blendpoint' value='$blendpoint'></form></body></html>";


void setup() {
  Serial.begin(115200);
  clock.Begin();   
  clock.Show();

  
  loadConfig();

  initWiFi();

  //initialise the NTP clock sync function
  NTPclient.begin("time.nist.gov", PST);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);
  
  clock.attach(0.5, toggleColon);

  prevsecond =second();
}


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
          server.on("/", handle_root);  
          server.on("/cleareeprom", webHandleClearRom);
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


void loop() {
  int hour_pos;
  int min_pos;
  server.handleClient();
  hour_pos = (minute() % 12) * pixelCount / 12 + second()/6;
  min_pos = second() * pixelCount / 60;
  delay(50);

  if(second()!=prevsecond) {
    epiphanyface(hour_pos, min_pos);
    clock.Show();
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

