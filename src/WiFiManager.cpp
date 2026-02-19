/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-wi-fi-manager-asyncwebserver/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include "WiFiManager.h"
#include "NodeServices.h"
#include "gpio.h"
#include "FileSystem.h"
#include "MQTTComms.h"
#include "sound.h"
#include "action.h"

//#include <cstring>
#include <Arduino.h>                              		//-- moved to arduinoGlue.h
#include <WiFi.h>                                 		//-- moved to arduinoGlue.h
#include <ESPAsyncWebServer.h>                    		//-- moved to arduinoGlue.h
#include <AsyncTCP.h>                             		//-- moved to arduinoGlue.h
#include <ESPmDNS.h>                              		//-- moved to arduinoGlue.h
#include <ArduinoJson.h>                          		//-- moved to arduinoGlue.h
#include <AsyncJson.h>                            		//-- moved to arduinoGlue.h
#include <WiFiUdp.h>                              		//-- moved to arduinoGlue.h
#include <ArduinoOTA.h>                           		//-- moved to arduinoGlue.h

// Search for parameter in HTTP POST request when in access point mode


const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "hostName";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);



void playAudio(int x, bool y)
{

}
void stopAudio()
{

}
String brokerIP;
//String nodeIDstring;
int sceneNo;
//int nodeID;

//Variables to save values from HTML form
String ssid;
String pass;
//String hostName;

boolean haveConnected;
long wifiConnectionTime;

// Timer variables
unsigned long previousMillis = 0;
const long interval = 15000;  // interval to wait for Wi-Fi connection (milliseconds)

// Initialize WiFi
bool initWiFi() 
{
  haveConnected = false;
  WiFi.setHostname(node.hostName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(node.ssid, node.pass);
  Serial.println(" Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) 
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) 
    {
      Serial.println("Failed to connect.");
      return false;
    }
    yield();
  }
  String localHostIP = WiFiGetIPAddress();
  Serial.println(localHostIP);
  Serial.println("(initOTA)...");
  ArduinoOTA.setHostname(node.hostName);
  ArduinoOTA
    .onStart([]() 
    {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
      {
        type = "filesystem";
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        SPIFFS.end();
      }
      Serial.println("Start updating " + type);
    })
    
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  if(!MDNS.begin(node.hostName)) 
  {
     Serial.println("Error starting mDNS");
  }
  else
  {
    MDNS.addService("_http", "_tcp", 80);
    Serial.print("MDNS started for host:");
    Serial.println(node.hostName);
  }
  haveConnected = true;
  return true;
}

void  checkWiFiConnection()
{
  // Checks the Wifi connection and re-connects if it is down
  // Call it from the main loop every 15 seconds or so...
  if(haveConnected == true)
  {
    // Have connected to WiFi at some point
    if(WiFi.status() != WL_CONNECTED)
    {
      // Wifi connection has dropped out, try and restart it
      wifiConnectionTime = 0;
      Serial.println("(checkWiFiConnection) WiFi dropped out");
      WiFi.disconnect();
      WiFi.reconnect();
      if(WiFi.status() == WL_CONNECTED)wifiConnectionTime = millis();
    }
    else 
    {
//      Serial.println("(checkWiFiConnection) WiFi ok");
    }
  }
  else 
  {
    Serial.print("(checkWiFiConnection) WiFi never connected");
  }
}

void setupWiFi() 
{
  if(initWiFi()) 
  {
    Serial.println("initialising web server");
    server.serveStatic("/", SPIFFS, "/");
    
    // Route for /home web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve node.html");
//      request->send(SPIFFS, "/scenes.html", "text/html", false, processor);
      request->send(SPIFFS, "/node.html", "text/html", false);
    });

    // Route for /node web page
    server.on("/page/node", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve node.html");
      request->send(SPIFFS, "/node.html", "text/html", false);
    });

    // Route for node configuration web page
    server.on("/page/nodeconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve nodeConfig.html");
      request->send(SPIFFS, "/nodeConfig.html", "text/html", false);
    });

    // Route for gpio bit configuration web page
    server.on("/page/gpioconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve gpioConfig.html");
      request->send(SPIFFS, "/gpioConfig.html", "text/html", false);
    });

    // Route for gpio web page
    server.on("/page/gpio", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve gpio.html");
      request->send(SPIFFS, "/gpio.html", "text/html", false);
    });

    // Route for action web page
    server.on("/page/action", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve action.html");
      request->send(SPIFFS, "/action.html", "text/html", false);
    });

    // Route for action config web page
    server.on("/page/actionconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve actionconfig.html");
      request->send(SPIFFS, "/actionConfig.html", "text/html", false);
    });

    // Route for sound web page
    server.on("/page/sound", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve sound.html");
      request->send(SPIFFS, "/sound.html", "text/html", false);
    });

    // Route for soundConfig config web page
    server.on("/page/soundconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve soundConfig.html");
      request->send(SPIFFS, "/soundConfig.html", "text/html", false);
    });

    // Route for /favicon
    server.on("/favicon", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve favicon.png");
      request->send(SPIFFS, "/favicon.png", "image/png", false);
    });

    //======== Server REST WebAPI Endpoints ===============================//
    //
    // GET endpoints
    //
    
    server.on("/api/nodeid/value", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      Serial.println("x");
      if (request->url() == "/api/nodeid/value") 
      {
        Serial.println("Received /api/nodeid/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        Serial.print("nodeIDstring:");
        Serial.println(node.getNodeIDstring());
        doc["value"] = node.getNodeIDstring();
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/brokerip/value", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/brokerip/value") 
      Serial.println("x");
      {
        Serial.println("Received /api/brokerip/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["value"] = node.brokerIP;
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/hostname/value", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/hostname/value") 
      {
        Serial.println("Received /api/hostname/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["value"] =node.hostName;
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/node/status", HTTP_GET, [](AsyncWebServerRequest *request) 
    {

      if (request->url() == "/api/node/status") 
      {
        Serial.println("Received /api/node/status GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["WIFIuptime"] = WiFiGetCommsUptime();
        doc["ipAddress"] = WiFiGetIPAddress();
        doc["rssi"] = WiFiGetRSSI();
        doc["mqttState"] = checkMQTTState();
        doc["mqttUptime"] = getMQTTUptime();
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/mp3Player/status", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/mp3Player/status") 
      {
        Serial.println("Received /api/mp3Player/status GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["currentTrack"] = mp3.currentTrack;
        doc["currentVolume"] = mp3.currentVolume;
        doc["manualTrim"] = mp3.manualTrim;
        doc["autoTrim"] = mp3.autoTrim; 
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/soundTrack/status", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/soundTrack/status") 
      {
        int trackNo;
        char* ptr;
        Serial.println("Received /api/soundTrack/status GET request");
        if (request->hasParam("trackno")) trackNo = strtol(request->getParam("trackno")->value().c_str(),&ptr,10);
        else trackNo = 0;
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["name"] = mp3.track[trackNo].name;
        doc["duration"] = mp3.track[trackNo].duration;
        doc["volume"] = mp3.track[trackNo].volume;
        doc["enableLocal"] = mp3.track[trackNo].enableLocal;
        doc["enableRemote"] = mp3.track[trackNo].enableRemote;
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/action/config", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/action/config") 
      {
        int number;
        char* ptr;
        Serial.println("Received /api/action/config GET request");
        if (request->hasParam("number")) number = strtol(request->getParam("number")->value().c_str(),&ptr,10);
        else number = 0;
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["number"] = number;
        doc["enableLocal"] = action[number].enableLocal;
        doc["enableRemote"] = action[number].enableRemote;
        doc["repeat"] = action[number].getRepeat();
        doc["state"] = action[number].getState();        
        doc["userState"] = action[number].userState; 
        doc["userVar1"] = action[number].userVar1; 
        doc["userVar2"] = action[number].userVar2;

        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/gpio/config/bit", HTTP_GET, [](AsyncWebServerRequest *request) 
    {  
      long bit;
      if (request->url() == "/api/gpio/config/bit") 
      {
        // Check if "BitNo" parameter exists in the url ( ?Bitno=x)
        char* ptr;
        if (request->hasParam("bitno")) bit = strtol(request->getParam("bitno")->value().c_str(),&ptr,10);
        else bit = 0;
        Serial.print("Received /api/gpio/config/bit GET request Bit:");
        Serial.println(bit);
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        JsonDocument doc;
        doc["bitNo"] = bit;
        doc["value"] = gpio[bit].getValue();
        doc["name"] = gpio[bit].name;
        doc["type"] = gpio[bit].type;
        doc["preset0"] = gpio[bit].preset0;
        doc["preset1"] = gpio[bit].preset1;
        doc["preset2"] = gpio[bit].preset2;
        doc["rate"] = gpio[bit].rate;
Serial.print("getConfigWEBenpoint  enableRemote: ");
Serial.println(gpio[bit].enableRemote);
        doc["enableRemote"] = gpio[bit].enableRemote;
        doc["enableLocal"] = gpio[bit].enableLocal;
        doc["publishRate"] = gpio[bit].getPublishRate();
        doc["easingType"] = gpio[bit].getEasingType();
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    // Handle POST requests
    // ********************
    server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) 
    {
      Serial.println("Received api POST request");
      Serial.println(request->url());

      if (request->url() == "/api/node/restart") 
      {
        Serial.println("Restart Requested");
        powerGPIO(false);
        delay(1000);
        ESP.restart();
      }

      if (request->url() == "/api/nodeid/value") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            const char* nodeID = doc["value"].as<const char*>();
            Serial.print("MQTT NodeID:");
            Serial.println(nodeID);    
            node.setNodeID(strtol(nodeID,NULL,16));
            writeFile(SPIFFS, nodeIDPath, node.getNodeIDstring());
            Serial.print("MQTT NodeID:");
            Serial.println(node.getNodeIDstring());
          }
      }

      if (request->url() == "/api/brokerip/value") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            //String bIP = doc["value"];
            const char* buff;
            buff = doc["value"].as<const char*>();
            writeFile(SPIFFS, brokerIPPath, buff); 
            strcpy(node.brokerIP,buff);
            Serial.print("MQTT brokerIP:");
            Serial.println(node.brokerIP);
          }
      }

      if (request->url() == "/api/gpio/config/bit") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            Serial.println("config/bit");
            uint8_t bit = (int) doc["bitNo"];
            const char* buff;
            buff = doc["name"].as<const char*>();
            strcpy(gpio[bit].name,buff);
            gpio[bit].alwaysWrite((int) doc["value"]);
            gpio[bit].setType((int) doc["type"]);
            gpio[bit].preset0 = (int) doc["preset0"];
            gpio[bit].preset1 = (int) doc["preset1"];
            gpio[bit].preset2 = (int) doc["preset2"];
            gpio[bit].rate = (int) doc["rate"];
            gpio[bit].enableRemote = doc["enableRemote"];
            gpio[bit].enableLocal = doc["enableLocal"];
            gpio[bit].setPublishRate ((int) doc["publishRate"]);
            gpio[bit].setEasingType((int) doc["easingType"]);
            writeConfigFile(SPIFFS,bit);
           }
      }

      if (request->url() == "/api/soundtrack/config") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            uint8_t trackNo = (int) doc["trackNo"];
            const char* buff;
            buff = doc["name"].as<const char*>();
            strcpy(mp3.track[trackNo].name,buff);
            mp3.track[trackNo].duration = (int) doc["duration"];
            mp3.track[trackNo].volume = (int) doc["volume"]; 
            mp3.track[trackNo].enableRemote = doc["enableRemote"];
            mp3.track[trackNo].enableLocal = doc["enableLocal"];
            writeMP3TrackConfigFile(SPIFFS,trackNo);
          }
      }

      if (request->url() == "/api/action/config") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            uint8_t number = (int) doc["number"];
            const char* buff;
            buff = doc["name"].as<const char*>();
            action[number].enableRemote = doc["enableRemote"];
            action[number].enableLocal = doc["enableLocal"];
            writeActionConfigFile(SPIFFS,number);
          }
      }

      if (request->url() == "/api/gpio/value/bit") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            uint8_t bit = (int) doc["bitNo"];
            gpio[bit].alwaysWrite((int) doc["value"]);
            Serial.print("GPIO Bit:");
            Serial.println(bit);
Serial.print("Value:");
Serial.println((int) doc["value"]);
           }
      }

      if (request->url() == "/api/mp3Player/config") 
      {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, (const char*)data);
        if(error)
        {
           Serial.println("Deserialisationerror");
        }
        else
        {
          mp3.manualTrim = doc["manualTrim"];
          mp3.autoTrim =doc["autoTrim"];
          writeMP3ConfigFile(SPIFFS);
        }
      }
      
      if (request->url() == "/api/action/play") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            Serial.print("action: ");
            int number = (int) doc["number"];
            bool loop = doc["loop"];
            if((number >= 0)&&(number<16))action[number].play(CMD_ANY,loop);             
            Serial.println(number);
          }
      }

      if (request->url() == "/api/action/stop") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            Serial.print("action: ");
            int number = (int) doc["number"];
            if((number >= 0)&&(number<16))action[number].stop(CMD_ANY);             
            Serial.println(number);
          }
      }

      if (request->url() == "/api/soundtrack/play") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            Serial.print("track: ");
            int track = (int) doc["track"];
            bool loop = doc["loop"];
            if((track >= 0)&&(track<16))mp3.play(CMD_ANY,track,loop);             
            Serial.println(track);
          }
      }

      if (request->url() == "/api/soundtrack/stop") 
      {
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, (const char*)data);
          if(error)
          {
              Serial.println("Deserialisationerror");
          }
          else
          {
            Serial.print("track: ");
            int track = (int) doc["track"];
            mp3.stop(CMD_ANY);
            if((track >= 0)&&(track<16))mp3.stop(CMD_ANY);             
            Serial.println(track);
          }
      }

      Serial.println("200");
      request->send(200,"application/json","OK");
    });      
    server.begin();
  }
  else 
  {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost())
        {
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) 
          {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass.c_str());
          }
        }
      }
///      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to host (.local): " + hostName);
 
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to host (.local): ");
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

/*
void processBitGetValue(uint8_t bit)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  StaticJsonDocument<100> doc;
  doc["value"] = gpio[bit].remoteRead();
  serializeJson(doc,*response);  
  request->send(response);         // return current values of all IO points
}
*/


unsigned long WiFiGetCommsUptime()
{
  if(wifiConnectionTime != 0)return((millis()-wifiConnectionTime)/60000);
  else return(0);
}

String WiFiGetIPAddress()
{
  return(WiFi.localIP().toString());
}

String WiFiGetSSID()
{
  return(WiFi.SSID());
}

int WiFiGetRSSI()
{
  return(WiFi.RSSI());
}
