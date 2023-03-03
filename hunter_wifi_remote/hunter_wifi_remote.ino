/*
  This project is a web based remote control for a Hunter X-Core Sprinkler controller
  dedicated for an ESP8266 or ESP32. 
  Tested on a ESP8266 with version v3.0.2 in the Arduino IDE Board Manager
  
  Installation:
  - Copy the following files into one folder.
    hunter_wifi_remote.ino
    hunter.cpp
    hunter.h
  - Open the file hunter_wifi_remote.ino in Arduino IDE.
  - Adapt the SSID and the password in the programm below to your Wifi.
  - Upload the programm to your ESP.
  - Place the ESP within range of your access point.
  - Connect GPIO 16 of the ESP to the REM pin of the X-Core controller.
  - Connect an ESP-pin with 3,3V to the AC pin next to REM on the X-Core controller.
  - Power-up the ESP with a floating power-supply.
  
  Use as follows:
  - Read the IP address shown in the serial monitor during start-up 
    of the ESP.
  - Enter the read IP address in a web browser of a host connected 
    to the same network than the ESP.
  - Push the slider of the zone you wish to activate.

  Options:
  - GPIO 5 can be used to control a pump that should start during activation
    of the zone.
  
  Copyright 2021, Claude Loullingen
  All rights reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  This project is a combination of the ESP8266 Relay Module Web Server 
  project of Rui Santos and Sebastiens OpenSprinkler-Firmware-Hunter project:
  - webserver: https://RandomNerdTutorials.com/esp8266-relay-module-ac-web-server/
  - communication to Hunter X-core: https://github.com/seb821/OpenSprinkler-Firmware-Hunter

*/

// Import required libraries
//#include "ESP8266WiFi.h" //https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/
#include "ESPAsyncWebServer.h" //eventually to be downloaded at https://RandomNerdTutorials.com/esp8266-relay-module-ac-web-server/
#include "hunter.h" //included in this sketch

// Define constants
const bool   PUMP_PIN_DEFAULT = false; // Set to true to set PUMP_PIN as On by default
const int    PUMP_PIN = 5; // GPIO5 = D1

// Replace ... with the SSID and the password of your Wifi
const char* ssid = "micasasucasa";
const char* password = "edu.y.vero";

// Declare global variables
int GlobalZoneID = 0;
int GlobalProgramID = 0;
int GlobalTime = 0;
int GlobalFlag = 0;
bool RAIN_GAUGE_PIN_before = HIGH;
bool RAIN_GAUGE_PIN_now;
int retry = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// HTML-Code for the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>HunterControl</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    h4 {margin-bottom: 0px;}
    p {margin-top: 0px; font-size: 14px;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px; border=1px;}
    .switch {position: relative; display: inline-block; width: 100px; height: 58px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 29px}
    .slider:before {position: absolute; content: ""; height: 44px; width: 44px; left: 7px; bottom: 7px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(42px); -ms-transform: translateX(42px); transform: translateX(42px)}
  </style>
</head>
<body>
  <h3>X-Core Remote Control</h3>
  <h4>Zone #1</h4>
  <p>(arroseurs c&ocirc;t&eacute; Rui, maison N&deg;3, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="1" value="10"><span class="slider"></span></label>
  <!-- When a slider is pushed a GET request is generated which contains the zone# and the state to be set. -->
  <hr>
  <h4>Zone #2</h4>
  <p>(arroseurs fond du terrain, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="2" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #3</h4>
  <p>(arroseurs c&ocirc;t&eacute; Paula, maison N&deg;1, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="3" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #4</h4>
  <p>(tuyeau jardin avant, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="4" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #5</h4>
  <p>(tuyeau fond du terrain, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="5" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #6</h4>
  <p>(tuyeau 1 c&ocirc;t&eacute; maison N&deg;1, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="6" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #7</h4>
  <p>(tuyeau 2 c&ocirc;t&eacute; maison N&deg;1, max. 10 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="7" value="10"><span class="slider"></span></label>
  <hr>
  <h4>Zone #8</h4>
  <p>(remplissage r&eacute;servoir, max. 40 min.)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="8" value="40"><span class="slider"></span></label>
  <hr>
  <h4>Programme #1</h4>
  <p>(lancer le programme d'arrosage 1)</p>
  <label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="9" value="40"><span class="slider"></span></label>
  <script>
    function toggleCheckbox(element) 
    {
      var xhr = new XMLHttpRequest();
      // If an slider is activated, then ...
      if(element.checked)
      { 
        // ... uncheck all other sliders and ...
        for(i=1;i<=9;i++)
        {
          if(i!=element.id)
          {
            document.getElementById(i).checked = false;
          }
        }
        if(element.id<=8)
        {
          // ... send GET request to webserver with zone# and wanted duration.
          xhr.open("GET", "/update?zone="+element.id+"&time="+element.value, true);
        }
        else if(element.id==9)
        {
          // ... send GET request to webserver with program#.
          xhr.open("GET", "/update?program="+element.id, true);          
        }
      }
      else 
      { 
        if(element.id<=8)
        {
          // stop = start for 0 minutes
          xhr.open("GET", "/update?zone="+element.id+"&time=0", true);
        }
      }
      xhr.send();
    }
  </script>
</body>
</html>
)rawliteral";


void setup(void){
  // Serial port for debugging purposes
  Serial.begin(9600);
  
  //define outputs for pump control and bus communication to X-Core controller
  pinMode(PUMP_PIN, OUTPUT); // GPIO5 to switch the pump
  pinMode(HUNTER_PIN, OUTPUT); // Bus Port, see value of HUNTER_PIN in hunter.h

  // Set PUMP_PIN to default value
  if(PUMP_PIN_DEFAULT)
  {
    digitalWrite(PUMP_PIN, HIGH);
  }
  else
  {
    digitalWrite(PUMP_PIN, LOW);
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password); //ssid and password are defined above
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Print ESP8266 Local IP Address to serial monitor
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Publish root web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Handle GET request generated when pushing a slider. GET structure is <ESP_IP>/update?zone=<zone#>&time=<0 to 240 minutes>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputParamValue1;
    String inputParamValue2;
    String inputParamValue3;
    // If both required parameters have been transmitted, then ...
    if (request->hasParam("zone") & request->hasParam("time")) 
    {
      // Extract the values of the parameters
      inputParamValue1 = request->getParam("zone")->value();
      inputParamValue2 = request->getParam("time")->value();
      GlobalZoneID = inputParamValue1.toInt();
      GlobalTime = inputParamValue2.toInt();
      Serial.println("zone = " + inputParamValue1 + ", time = " + inputParamValue2);

      // Set GPIO5 to control the pump and set flag to send a message to the X-Core controller 
      if(GlobalTime>0)
      {
        Serial.println("Set GPIO5 high.");
        digitalWrite(5, HIGH); 
      }
      else if(GlobalTime==0)
      {
        Serial.println("Set GPIO5 low.");
        digitalWrite(5, LOW);
      }
      GlobalFlag = 1; // For an unknown reason, the HunterStart command cannot be used inside of the 'server.on' handler. Instead a flag is set, which will be evaluated within the main loop below.
    }
    else if (request->hasParam("program")) 
    {
      // Extract the values of the parameters
      inputParamValue3 = request->getParam("program")->value();
      GlobalProgramID = inputParamValue3.toInt()-8;
      Serial.println("program = " + (String)GlobalProgramID);

      GlobalFlag = 2; // For an unknown reason, the HunterProgram command cannot be used inside of the 'server.on' handler. Instead a flag is set, which will be evaluated within the main loop below.      
    }
    else 
    {
      Serial.println("No parameters transmitted or parameters incomplete.");
    }
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();

}

// Start or stop zone if according flag is set.
void loop(void) 
{
  // If a zone has been started
  if(GlobalFlag==1)
  {
    Serial.println("GlobalZoneID = " + (String)GlobalZoneID + ", GlobalTime = " + (String)GlobalTime);
    HunterStart(GlobalZoneID,GlobalTime);
    GlobalFlag=0;
  }
  else if(GlobalFlag==2)
  {
    Serial.println("GlobalProgramID = " + (String)GlobalProgramID);
    HunterProgram(GlobalProgramID);
    GlobalFlag=0;    
  }
}
