# NodeMCU Wifi Enabled Power Outlet
In order to switch on a printer (or any other device for that matter) without getting up from my computer I built a power outlet, that can be switched on and off through a simple web interface. 

<img src="/images/ScreenShot_Webinterface.png" width="250"> <img src="/images/IMG_0148.JPG" width="500">

# Basic idea
The basic idea is to setup a minimal http-Server on a NodeMCU that hosts a website, which can pull the current status of the power outlet and switch it through AJAX-calls. For redundancy and ease of use we add a status LED and a physical switch to control the outlet without an internet connection as well.
The NodeMCU will listen for http-requests and switch the relay on calls from the web, as well as if the physical switch is flipped.

# Parts you need:
* 1 [NodeMCU](http://www.ebay.de/itm/NodeMCU-V3-Lua-WIFI-IOT-Node-Entwicklung-ESP8266-micro-USB-32bit-Arduino-E06-/172109878470)
* 1 [Relay](https://www.ebay.de/itm/5V-1-Channel-Optocouplers-Relay-Shield-for-Arduino-Optokoppler-Relais-CP0401D/281661623000?hash=item4194574ad8:g:2KoAAOSwFLBaayRF)
* 1 [PnP Transistor](http://www.ebay.de/itm/50x-BC547B-Transistor-NPN-45V-100mA-TO92-von-CDIL/290341478860?hash=item4399b361cc:g:XiEAAOSwd4tT6v7u)
* 2 Resistors (330 Ohm, 220 Ohm)
* 1 [LED](https://www.ebay.de/itm/2-st-LED-5mm-kaltweis-5800-7000mcd-30-3-5V-20mA-Front-gewolbt/112641693157)
* 1 [physical switch](https://www.ebay.de/itm/2x-MINI-WIPPSCHALTER-WIPPENSCHALTER-RUND-SCHWARZ-15MM-1-POLIG-230V-6A-KFZ-12V-DC/263110599869)
* [Jumper cables](https://www.ebay.de/itm/40-x-10cm-20cm-o-30cm-Jumper-Kabel-Dupont-Cable-Breadboard-Wire-f-Arduino/252355489428)
* [5V Power outlet](http://www.ebay.de/itm/Universal-Netzteil-3V-4-5V-5V-6V-7-5V-9V-12V-300mA-3-6W-mit-USB-Adapter-goobay/331649352035)
* Power cable
* [Chandelier clamps](http://www.ebay.de/itm/Lusterklemmen-a-12-Lusterklemme-Verbindungsklemme-Klemme-Listerklemme-Lampe/253095147132) (for safe connections with the mains power cable)

# Knowledge Requirements
I will be using parts and constructs which would far exceed the scope of this tutorial to explain. If you are not familiar with some of these things, here is a list of links that helped me a lot:
* [Relays](https://www.youtube.com/watch?v=b6ZagKRnRdM)
* [Transistors](https://www.youtube.com/watch?v=UdAnUc7nXYs)
* [NodeMCU](http://www.nodemcu.com/index_en.html)
* [Wifi setup for the NodeMCU](http://henrysbench.capnfatz.com/henrys-bench/arduino-projects-tips-and-more/connect-nodemcu-esp-12e-to-wifi-router-using-arduino-ide/)
* [Arduino IDE](https://www.arduino.cc/en/main/software)
* [USB to UART Bridge Driver](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers) (if you are using a mac this driver is needed in order for the Arduino IDE to be able to connect to microcontrollers through USB)
* [AJAX-Calls](https://www.w3schools.com/js/js_ajax_intro.asp)

# The Circuit
>:exclamation: In this project we will be working with 230V mains power, which is very dangerous if not handled properly. Do not try to imitate this at home if you have not worked with mains power before!
<img src="/images/IMG_0149.JPG">

Since we have to cut open the mains wire anyway, we split off our 5V power supply in parallel. From that we power the NodeMCU, and the relay.
See the fritzing diagram below for more detail on the circuit (The resistor values depend on the LED and transistor models you use, I used 330 Ohms on the LED and 220 Ohm on the Transistor):

![Fritzing Diagram](/diagram/wifiRelaisFritzing_bb.png)

We split off hot and neutral from mains to power the 230Vac - 5Vdc converter, which powers the nodeMCU. The relay takes 5V and ground as well. 

The third input pin of the relay is connected to pin D2 through a transistor. The transistor is needed to amplify the logic high voltage of 3.3V to around 5V because the relay won't switch consistently with just 3.3V. (This is somewhat odd, since it did switch just fine without the transistor, however only for a couple times. After about 5 times switching back and forth the relay just stopped working. As soon as I connected the input pin to 5V/ground it switched just fine. So I added the transistor to pull the 3.3V up to 5V).
>:information_source: Notice that the relay I used switches from NC to NO if low/ground is supplied to the input pin. This means the transistor needs to supply 0V/Ground. If we switch the transistor it connects the input pin to ground thus switching the relay.

On pin D0 we connect a simple switch, which is then connected to ground. We will use this, to switch the relay by hand. Since we want the webinterface to show the correct status at all times, we run the switch as an input to the node, which manages all the switching of the relay.

Finally we add a small status LED on pin D1, so on/off state of the relay is visible on the housing as well.

# The Code
First let's write the script that will run on the nodeMCU. You could program the node in the Lua scripting language, however, I prefer C-based arduino code, which works just as well. I wrote the code in the [arduino ide](https://www.arduino.cc/en/main/software) and flashed it onto the node directly from the ide. The complete script can be found in [here](/code/relaisServer.ino).

Import libraries for accessing WiFi networks:
```C
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
```

Create a couple of constants for wifi credentials, GPIO pins and two flags to store the relay's current state and the state of the input switch
```C
const char* ssid = "yourSSID";
const char* password = "yourPassword";

const int led = 16;
const int relay = 4;
const int switchInput = 5;

bool relayOn = false;
int lastInputState = 0;
```
>:information_source: Note that the nodeMCU pin numbers for D0, D1, and D2 are NOT 0, 1 and 2. Find a full list of the correct numbers for the pins [here](https://github.com/esp8266/Arduino/issues/584)

Now lets create the webserver. This is pretty simple thanks to the libraries we imported earlier. The parameter for the `server()` constructor specifies the port on which the server should listen for clients, which we set to 80. This is the standard port for HTTP-Servers, making it convenient to access the web interface by typing the IP-address of the node into the browser.

```C
ESP8266WebServer server(80);
```

Now lets create the heart of this script - a function that switches the relay. Although this particular relay switches from NC to NO on low (0V/Ground) we set the relay pin to 1 to turn it on and vice versa. Why? Because we have a transistor in between the relay and the NodeMCU and the transistor switches on high voltage supplied to the base! (But we still need to connect the transistor to ground so that switching it means making the relay's input pin electrically common with ground and thus switching the relay)
Finally we update the status LED so it is always consistent with the relay's state.

```C
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
```

## Server
The main motivation behind this project is my laziness. I don't want to get up to switch on my printer but rather do it from the comfort of my favorite browser. So far we created a webserver on the node but now we have to think about what we want to show to the user and how we will process everything. 
Basically we want a switch widget, that shows the current status (on/off) of our relays and switches the relay on/off when the user clicks it. Since we have a physical switch that can change the relay's state as well, we also want some way to keep the web interface in sync with the nodeMCU. To achieve all of this we are going to create a couple of different handles on our server, to which we will send AJAX calls from the web interface. That way we can simply user a small javascript function in the webinterface, to regularly check for the relays status and send a switching command to the nodeMCU if the user pressed the switch. 

We will do the wiring up of the server and the different handles later, lets first create a couple of functions that deal with the different scenarios:

If the webinterface is called up we send back our html (which we will create later):

```C
void handleRoot() {
 
  String message = "insert a minified version of our webinterface here later";

  server.send(200, "text/html", message);
}
```

If a client checks for the status we send back if its on or off in plain text
```C
void handleStatus(){
  Serial.println("Recieved Status Request");
  String message = (relayOn) ? "ON" : "OFF";
  server.send(200, "text/plain", message);
}
```

If the client wants to switch the relais we switch the relay and send back the new state
```C
void handleWebSwitch(){
  Serial.println("Switching the relay from web");
  switchRelay();
  String message = (relayOn) ? "ON" : "OFF";
  server.send(200, "text/plain", message);
}
```

This is optional, however, in case some user starts trying different urls/paths we will let them know that there's nothing else by sending a 404 Error (who doesn't love them ;)
```C
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
```

Now all of our handles are ready, so we will wire up our server. We do this in the `setup()` function. This function will be called automatically as the very first thing within our script, as soon as the NodeMCU has power.
```C
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
```
Note that we turn on serial monitoring, which enables us to print stuff to the serial monitor of the arduino ide once the code runs on the NodeMCU.
We set the pins to INPUT/OUTPUT as we need them and then we connect to the wifi.
Once we have wifi we set up the different handles on the server by defining a path and the function which is to be run, when the server receives a request for that path. We will need these paths later for our AJAX calls in the web interface.
Finally we start the server with `server.begin();`.

## Physical Switch
We have the groundwork for switching via wifi, now we write a small function that will handle the physical switch:
```C
void handlePhysicalInputSwitch(){
  int currentInputState = digitalRead(switchInput);
  if (lastInputState != currentInputState){
    Serial.println("Switching the relay by hand");
    switchRelay();
    lastInputState = currentInputState;
  }
}
```

Finally we write up the `loop()` function. This function is executed automatically after `setup()` has finished and is called again and again until the NodeMCU is disconnected from its power source.
We listen for clients on the webserver and check if the physical switch was flipped. That's basically all the script has to do until the end of eternity which means we're finished with the "Backend".
```C
void loop(void){
  server.handleClient(); //Listen for Webinterface calls
  handlePhysicalInputSwitch(); //Listen for changes in the physical switch
}
```

However, do not flash the script onto the NodeMCU just yet, because we still need to add our webinterface to the `handleRoot()`function!

# The Web Interface
The basic web interface is pretty straightforward, just create a simple html file with some markup that resembles some sort of a switch. You can look at [my html](/code/wifiRelay.html) for inspiration if you want to, or just go completely crazy ;) 

The interesting stuff happens in the javascript. Let's first set up a couple of global variables that hold the status of the relay and the status of our widget on the web interface as well as a variable for the widget itself so we can access it from all functions later on:
```JavaScript
var switchState = false;
var switchWidgetState = false;

var switchWidget;
```

Remember we need the interface to always be in sync with the actual state of the relay. To do that let's write up a function that requests the `/status` handle on the NodeMCU and processes the answer:
```JavaScript
function refreshStatus(){
	var request = new XMLHttpRequest();
	request.open('GET','/status');
	request.addEventListener('load', function(event) {
	   if (request.status >= 200 && request.status < 300) {
	      switchState = (request.responseText === 'ON') ? true : false;
	      updateSwitchWidget();
	   } else {
	      console.warn(request.statusText, request.responseText);
	   }
	});
	request.send();	
}
```
If you are not familiar with AJAX calls using the `XMLHttpRequest()` take a look at the [w3schools tutorials](https://www.w3schools.com/js/js_ajax_intro.asp).

Notice how we just call the `updateSwitchWidget()` function after we stored the current state of the relay in our global variable. This for one simplifies the code inside the XMLHttpRequest and we also need to update the Switch Widgets appearance when the user clicks the widget later on. So it's smart to put the functionality of updatign the widget into a separate function right away:
```JavaScript
function updateSwitchWidget(){
	if (switchState !== switchWidgetState) {
		if (switchState === false){
			switchWidget.style.background = 'red'; //dummy colors, you can do something fun like an animation here as well
		} else {
			switchWidget.style.background = 'green';
		}
		switchWidgetState = !switchWidgetState;
	}
}
```
We just compare the state of the switchWidget with the actual relay information and if the two are out of sync we change the appaerance of the widget.

All right, now we can keep the widget in sync with the actual relay. To always be in sync we need to call the `refreshStatus()` function repeatedly. To do this setup a listener on the window for the `load` event. This will ensure, that the entire DOM is loaded meaning we can actually access our widget.

```JavaScript
window.addEventListener('load', function(){
	switchWidget = document.getElementsByClassName('switchWidget')[0];
	switchWidget.addEventListener('click', switchDevice);

	refreshStatus();
	setInterval(refreshStatus, 500);
});
```

Now you can see that I also added an eventListener for the `click` event, that calls a function called `switchDevice` let's write that function, which sends a message to the node MCU to switch the relay:
```JavaScript
function switchDevice(){
	console.log('Switching the device!');
	var request = new XMLHttpRequest();
	request.open('GET','/switch');
	request.addEventListener('load', function(event) {
	   if (request.status >= 200 && request.status < 300) {
	      switchState = (request.responseText === 'ON') ? true : false;
	      updateSwitchWidget();
	   } else {
	      console.warn(request.statusText, request.responseText);
	   }
	});
	request.send();
}
```
Notice how we use the NodeMCUs `/switch` handle as the destination of our request and also store the new state of the relay before updating our widget graphically.

# Bringing it all together
Now we need to add our Webinterface html file to the NodeMCU Script in the `handleRoot()` function, otherwise nothing will be displayed when we request the default file from the NodeMCU through our browser. 

Since the NodeMCU doesn't have unlimited memory and I don't want to clutter up my C-like script with html and javascript markup too much I minified the html file. This means deleting all unnecessary white space and linebreaks. Since this makes the code very annoying to read or edit, make sure this is the last step in the process. 

To minify the html I used a [handy online minifier tool](http://minifycode.com/html-minifier/). Once we have the minified html file we copy and paste it in the line `String message = "insert a minified version of our webinterface here later";` instead of the placeholder text we put there earlier.

>:exclamation: Make sure that in your html and javascript you are not using `"` for enclosing strings, but rather `'`. this is because in the script our html code is just a string and having `"` inside a string will "break" it since they are the string deliminators. However having `'` inside a string will work just fine.

Now all we need to do is connect the NodeMCU to our computer via USB and flash the script onto it using the Arduino IDE. Once the script is on the Node check the IDEs Serial Monitor to see the printouts of Wifi connection status and IP adress we added in `setup()`. Once you have the IP-adress try out your new wifi switchable power outlet by typing the IP-adress in the adress field of your browser :)


# Final notes
Now that's basically it, we can switch our relay from the web (Yay!). I put my components in a (rather big) box I had lying around, to hide the rather lazy soldering work of mine ;)

If you have access to the router of your network you might want to setup a permanent IP-address for the NodeMCU. That way people that will use the WebInterface can save the IP in a bookmark or something similar without having to worry about it changing everytime the DHCP server decides to renew the Nodes lease.

Even fancier would be to add an entry to your local DNS server so that people could just type e.g. "awesomepoweroutlet.fritz.box" into their browser. If you're in a home network that uses a Fritzbox as its router [this setup guide](https://blog.lobraun.de/2015/05/03/static-ips-and-dns-names-for-devices-in-your-home-network/) will help you set this up.

While I doubt, that the time I save by not having to get up to turn on my printer anymore in any way justifies the time spent building this it was fun to hack together and I hope the tutorial helps some others trying to do something similar. If there are any questions feel free to ask :)
