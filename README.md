# NodeMCU Wifi Enabled Power Outlet
Small IOT Project creating a wifi enabled Power outlet.

<img src="/images/ScreenShot_Webinterface.png" width="250">

In order to switch on a printer (or any other device for that matter) without getting up from my computer I built a power outlet, that can be switched on and off through a simple web interface. 

>:exclamation: In this project we will be working with 230V mains power, which is very dangerous if not handled properly. Do not try to imitate this at home if you have not worked with mains power before!

# What you need:
* 1 [NodeMCU](http://www.ebay.de/itm/NodeMCU-V3-Lua-WIFI-IOT-Node-Entwicklung-ESP8266-micro-USB-32bit-Arduino-E06-/172109878470)
* 1 [Relais](https://www.ebay.de/itm/5V-1-Channel-Optocouplers-Relay-Shield-for-Arduino-Optokoppler-Relais-CP0401D/281661623000?hash=item4194574ad8:g:2KoAAOSwFLBaayRF)
* 1 [PnP Transistor](http://www.ebay.de/itm/50x-BC547B-Transistor-NPN-45V-100mA-TO92-von-CDIL/290341478860?hash=item4399b361cc:g:XiEAAOSwd4tT6v7u)
* 2 Resistors (330 Ohm, 220 Ohm)
* 1 [LED](https://www.ebay.de/itm/2-st-LED-5mm-kaltweis-5800-7000mcd-30-3-5V-20mA-Front-gewolbt/112641693157)
* 1 [physical switch](https://www.ebay.de/itm/2x-MINI-WIPPSCHALTER-WIPPENSCHALTER-RUND-SCHWARZ-15MM-1-POLIG-230V-6A-KFZ-12V-DC/263110599869)
* [Jumper cables](https://www.ebay.de/itm/40-x-10cm-20cm-o-30cm-Jumper-Kabel-Dupont-Cable-Breadboard-Wire-f-Arduino/252355489428)
* [5V Power outlet](http://www.ebay.de/itm/Universal-Netzteil-3V-4-5V-5V-6V-7-5V-9V-12V-300mA-3-6W-mit-USB-Adapter-goobay/331649352035)
* Power cable
* [Chandelier clamps](http://www.ebay.de/itm/Lusterklemmen-a-12-Lusterklemme-Verbindungsklemme-Klemme-Listerklemme-Lampe/253095147132)

# Basic idea
The basic idea is to setup a minimal http-Server on the NodeMCU that hosts a website, which can pull the current status of the power line and switch it through AJAX-calls. For redundancy and ease of use we add a status LED and a physical switch to control the box without an internet connection as well.
The NodeMCU Script will therefore listen for http-requests and switch the relay (through the transistor) on calls fro the web, as well as if the physical switch if flipped.

# The curcuit
Since we have to cut open the mains wire anyway, we split off our 5V power supply in parallel. From that we power the NodeMCU, and the relais.
See the fritzing diagram below for more detail on the circuit (The resistor values might change for your setup depending on the transistor and LED models you use):

![Fritzing Diagram](/diagram/wifiRelaisFritzing_bb.png)

We split off hot and neutral from mains to power the 230Vac - 5Vdc converter, which powers the nodeMCU. The relais takes 5V and ground as well. The third input pin of the relais is connected to pin D2 through a transistor. The transistor is needed to amplify the logic high voltage of 3.3V to around 5V because the relay won't switch consistently with just 3.3V. (This is somewhat odd, since it did switch just fine without the transistor, however only for a couple times. After about 5 times switching back and forth the relays just stopped working. As soon as I connected the input pin to 5V/ground it switched just fine. Once I added the transistor in, everything worked fine even when using the Nodes output pins)

On pin D0 we connect a simple switch, which is then connected to ground. We will use this, to switch the relais by hand. Since we want the webinterface to show the correct status at all times, we run the switch as an input to the node, which manages all the switching of the relay.

Finally we add a small status LED on pin D1, so on/off state of the relay is visible on the housing as well.

# The code
First let's write the script that will run on the nodeMCU. You could program the node in Lua scripting language, however I prefer C-based arduino code, which works just as well. I wrote the code in the [arduino ide](https://www.arduino.cc/en/main/software) and flashed it onto the node directly from the ide. The complete script can be found in /code/relaisServer.ino

Import libraries for accessing WiFi networks:
```C
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
```

Create a couple of constants for wifi credentials, GPIO pins and two flags to store the relay's currentstate and the state of the input switch
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

Now lets create the small webserver. This is pretty simple thanks to the libraries we imported earlier. The parameter for the server() constructor specifies on which port the server should listen for clients, which we set to 80. This is the standard port for HTTP-Servers making it convenient to access the web interface by typing the IP-address of the node into the browser.

```C
ESP8266WebServer server(80);
```
