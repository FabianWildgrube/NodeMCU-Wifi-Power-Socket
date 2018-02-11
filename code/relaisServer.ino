#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "yourSSID";
const char* password = "yourPassword";

const int led = 5;
const int relay = 4;
const int switchInput = 16;

bool relayOn = false;
int lastInputState = 0;

ESP8266WebServer server(80);

void switchRelay(){
  if (relayOn){
    digitalWrite(relay, 0);
    relayOn = false;
  } else {
    digitalWrite(relay, 1);
    relayOn = true;
  }
  Serial.print("Switched relay to: ");
  Serial.println(relayOn);
  //Update status LED
  digitalWrite(led, relayOn);
}

void handleRoot() {
 
  String message = "<html><head><title>Wildgrubes Home Automation</title><meta charset = 'UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'> <script type='text/javascript'>var switchState=false;var switchWidgetState=false;var switchWidget;window.addEventListener('load',function(){switchWidget=document.getElementsByClassName('switchWidget')[0];switchWidget.addEventListener('click',switchDevice);refreshStatus();setInterval(refreshStatus,500);});function refreshStatus(){var request=new XMLHttpRequest();request.open('GET','/status');request.addEventListener('load',function(event){if(request.status>=200&&request.status<300){switchState=(request.responseText==='ON')?true:false;updateSwitchWidget();}else{console.warn(request.statusText,request.responseText);}});request.send();} function switchDevice(){console.log('Switching the device!');var request=new XMLHttpRequest();request.open('GET','/switch');request.addEventListener('load',function(event){if(request.status>=200&&request.status<300){switchState=(request.responseText==='ON')?true:false;updateSwitchWidget();}else{console.warn(request.statusText,request.responseText);}});request.send();} function updateSwitchWidget(){if(switchState!==switchWidgetState){var offColor='linear-gradient(to right, rgba(102,102,102,1) 0%, rgba(48,48,48,1) 100%)';var onColor='linear-gradient(to left, rgba(31,49,166,1) 0%, rgba(32,124,229,1) 100%)';var bgWidth=switchWidget.offsetWidth;var knob=switchWidget.getElementsByTagName('SPAN')[0];var knobWidth=knob.offsetWidth;if(switchState===false){console.log('Switched OFF - color: '+offColor);switchWidget.style.background=offColor;knob.style.left='1';}else{console.log('Switched ON - color: '+onColor);switchWidget.style.background=onColor;var leftOffset=bgWidth-knobWidth-1;knob.style.left=leftOffset;} switchWidgetState=!switchWidgetState;}}</script> <style type='text/css'>*{margin:0;padding:0}body{background:white;font-family:'Trebuchet MS',Helvetica,sans-serif}a{color:#186ff2;text-decoration:none}a:hover{color:#1975ff;cursor:hand;text-decoration:underline}header{background:linear-gradient(to right, rgba(32,124,229,1) 0%, rgba(31,49,166,1) 100%);box-shadow:0px 5px 14px -1px rgba(0,0,0,0.63);position:fixed;height:60px;text-align:center;width:100%}header h3{color:white;font-size:28;padding-top:15}.main{width:90%;padding-top:100px;margin:auto}.switchUnit{margin-top:20px;border:1px solid rgb(150,150,150);border-radius:5px;background:linear-gradient(to right, rgba(222,222,222,1) 0%, rgba(240,240,240,1) 100%);width:45%;padding:15px;color:rgb(50,50,50)}.location{font-size:14;color:rgb(150,150,150)}.switchWidget{position:relative;margin-top:10px;width:50px;height:25px;border-radius:25px;border-color:#232323;border-width:1px;background:linear-gradient(to right, rgba(102,102,102,1) 0%, rgba(48,48,48,1) 100%)}.switchWidget span{position:absolute;left:1;top:1;border-radius:100%;border-width:0;background:linear-gradient(to bottom, #e2e2e2 0%,#dbdbdb 50%,#d1d1d1 51%,#fefefe 100%);width:23px;height:23px}.switchWidget:hover{cursor:pointer}footer{background:linear-gradient(to right, rgba(37,42,46,1) 0%, rgba(1,27,46,1) 100%);width:100%;height:30px;position:fixed;bottom:0;left:0;color:white;text-align:center;padding-top:15px}@media screen and (max-width: 395px){header{height:100px}.switchUnit{width:inherit}.main{width:80%;margin:auto}}@media screen and (max-width: 270px){footer{height:100px}}</style></head><body> <header><h3>Wildgrube's Home Automation</h3> </header><div class='main'><div class='switchUnit'><h2>Laserdrucker</h2> <span class='location'>B&uuml;ro Keller</span><div class='switchWidget'> <span></span></div></div></div><footer><div class='width80'>Developed by <a href='www.fabianwildgrube.de'>Fabian Wildgrube</a>, 2018</div> </footer></body></html>";

  server.send(200, "text/html", message);
}

void handleStatus(){
  Serial.println("Recieved Status Request");
  String message = (relayOn) ? "ON" : "OFF";
  server.send(200, "text/plain", message);
}

void handleWebSwitch(){
  Serial.println("Switching the relay from web");
  switchRelay();
  String message = (relayOn) ? "ON" : "OFF";
  server.send(200, "text/plain", message);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void){
  pinMode(led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(switchInput, INPUT_PULLUP);
  lastInputState = digitalRead(switchInput);
  digitalWrite(led, 0);
  digitalWrite(relay, 1); //switch relay off (turns on on logical Low)
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/switch", handleWebSwitch);

  server.on("/status", handleStatus);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void handlePhysicalInputSwitch(){
  int currentInputState = digitalRead(switchInput);
  if (lastInputState != currentInputState){
    Serial.println("Switching the relay by hand");
    switchRelay();
    lastInputState = currentInputState;
  }
}

void loop(void){
  server.handleClient(); //Listen for Webinterface calls
  handlePhysicalInputSwitch(); //Listen for changes in the physical switch
}
