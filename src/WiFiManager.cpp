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
#include "GCodeControl.h"

#include <Arduino.h>                              		
#include <WiFi.h>                                 		
#include <ESPAsyncWebServer.h>                    		
#include <AsyncTCP.h>                             		
#include <ESPmDNS.h>                              		
#include <ArduinoJson.h>                          		
#include <AsyncJson.h>                            		
#include <WiFiUdp.h>                              		
#include <ArduinoOTA.h>                           		

// Search for parameter in HTTP POST request when in access point mode


const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "hostName";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

namespace {
QueueHandle_t objectRunQueue = nullptr;
QueueHandle_t objectIdentifyQueue = nullptr;
QueueHandle_t sceneRunQueue = nullptr;
constexpr size_t kJsonResponseBufferSize = 256;
constexpr int kGpioCount = 16;
constexpr int kTrackCount = 16;
constexpr int kActionCount = 16;

struct ObjectRunRequest
{
  int index;
  int path;
};

struct SceneRunRequest
{
  int index;
};

void objectRunTask(void*);
void objectIdentifyTask(void*);
void sceneRunTask(void*);

bool queueObjectRun(int index, int path)
{
  if (objectRunQueue == nullptr)
  {
    return false;
  }

  const ObjectRunRequest request = {index, path};
  return xQueueOverwrite(objectRunQueue, &request) == pdTRUE;
}

bool queueObjectIdentify()
{
  if (objectIdentifyQueue == nullptr)
  {
    return false;
  }

  uint8_t trigger = 1;
  return xQueueOverwrite(objectIdentifyQueue, &trigger) == pdTRUE;
}

bool queueSceneRunRequest(int index)
{
  if (sceneRunQueue == nullptr)
  {
    return false;
  }

  const SceneRunRequest request = {index};
  return xQueueOverwrite(sceneRunQueue, &request) == pdTRUE;
}

void objectRunTask(void*)
{
  ObjectRunRequest request = {-1, -1};

  for (;;)
  {
    if (xQueueReceive(objectRunQueue, &request, portMAX_DELAY) == pdTRUE)
    {
      if ((request.index < 0) || (request.index >= kObjectCount))
      {
        Serial.print("Invalid object run index: ");
        Serial.println(request.index);
        continue;
      }

      if (request.path < 0)
      {
        Serial.print("Invalid object run path: ");
        Serial.println(request.path);
        continue;
      }

      Serial.print("run object: ");
      Serial.print(request.index);
      Serial.print(" path: ");
      Serial.println(request.path);

      if (!runPath(request.index, request.path))
      {
        Serial.println("runPath failed");
      }
    }
  }
}

void objectIdentifyTask(void*)
{
  uint8_t trigger = 0;

  for (;;)
  {
    if (xQueueReceive(objectIdentifyQueue, &trigger, portMAX_DELAY) == pdTRUE)
    {
      (void)trigger;
      Serial.println("identify object");
      loadGCodeObject();
    }
  }
}

void sceneRunTask(void*)
{
  SceneRunRequest request = {-1};

  for (;;)
  {
    if (xQueueReceive(sceneRunQueue, &request, portMAX_DELAY) == pdTRUE)
    {
      if (request.index < 0)
      {
        Serial.print("Invalid scene run index: ");
        Serial.println(request.index);
        continue;
      }

      Serial.print("run scene: ");
      Serial.println(request.index);

      if (!runScene(request.index))
      {
        Serial.println("runScene failed");
      }
    }
  }
}
}

bool queueSceneRun(int index)
{
  return queueSceneRunRequest(index);
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
  // Disable WiFi modem power-save: under sustained CPU load (e.g. long blocking Marlin/SD scene
  // waits) the modem can miss its power-save wake window, wedging the TCP send path (errno 11/EAGAIN).
  WiFi.setSleep(false);
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
        // NOTE: if updating SPIFFS this would be the place to unmount it using SPIFFS.end()
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
    if (objectRunQueue == nullptr)
    {
      objectRunQueue = xQueueCreate(1, sizeof(ObjectRunRequest));
      if (objectRunQueue != nullptr)
      {
        xTaskCreatePinnedToCore(objectRunTask,
                                "ObjectRun",
                                4096,
                                nullptr,
                                1,
                                nullptr,
                                1);
      }
      else
      {
        Serial.println("Failed to create objectRunQueue");
      }
    }

    if (objectIdentifyQueue == nullptr)
    {
      objectIdentifyQueue = xQueueCreate(1, sizeof(uint8_t));
      if (objectIdentifyQueue != nullptr)
      {
        xTaskCreatePinnedToCore(objectIdentifyTask,
                                "ObjectIdentify",
                                4096,
                                nullptr,
                                1,
                                nullptr,
                                1);
      }
      else
      {
        Serial.println("Failed to create objectIdentifyQueue");
      }
    }

    if (sceneRunQueue == nullptr)
    {
      sceneRunQueue = xQueueCreate(1, sizeof(SceneRunRequest));
      if (sceneRunQueue != nullptr)
      {
        xTaskCreatePinnedToCore(sceneRunTask,
                                "SceneRun",
                                4096,
                                nullptr,
                                1,
                                nullptr,
                                1);
      }
      else
      {
        Serial.println("Failed to create sceneRunQueue");
      }
    }

    Serial.println("initialising web server");

    // Force every HTTP response to close its connection instead of lingering in
    // keep-alive: ESP32's lwIP has only a handful of TCP PCBs shared with MQTT/OTA/mDNS,
    // and idle keep-alive browser connections were suspected of starving that pool.
    DefaultHeaders::Instance().addHeader("Connection", "close");

    // Route for /home web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve node.html");
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

    // Route for Objects web page
    server.on("/page/objects", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve objects.html");
      // Forcing "Connection: close" here previously left a TCP PCB stuck (lwIP has only a few
      // slots), starving other sockets (e.g. MQTT) and causing repeated write() errno 11 failures.
      request->send(SPIFFS, "/objects.html", "text/html", false);
    });

    // Route for Scene web page
    server.on("/page/scene", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve scene.html");
      request->send(SPIFFS, "/scene.html", "text/html", false);
    });

    // Route for Scenes web page
    server.on("/page/scenes", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve scenes.html");
      request->send(SPIFFS, "/scenes.html", "text/html", false);
    });

    // Route for /favicon
    server.on("/favicon", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Serve favicon.png");
      request->send(SPIFFS, "/Favicon.png", "image/png", false);
    });

    //======== Server REST WebAPI Endpoints ===============================//
    //
    // GET endpoints
    //
    
    server.on("/api/nodeid/value", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/nodeid/value") 
      {
        Serial.println("Received /api/nodeid/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
        JsonDocument doc;
        doc["value"] = node.getNodeIDstring();
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/brokerip/value", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/brokerip/value") 
      {
        //Serial.println("Received /api/brokerip/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
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
//        Serial.println("Received /api/hostname/value GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
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
//        Serial.println("Received /api/node/status GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
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
        //Serial.println("Received /api/mp3Player/status GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
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
//        Serial.println("Received /api/soundTrack/status GET request");
        if (request->hasParam("trackno")) trackNo = strtol(request->getParam("trackno")->value().c_str(),&ptr,10);
        else trackNo = 0;

        if ((trackNo < 0) || (trackNo >= kTrackCount))
        {
          request->send(400, "application/json", "{\"error\":\"invalid trackno\"}");
          return;
        }

//        Serial.print("TrackNo:");
//        Serial.println(trackNo);
//        Serial.print("Track Name:");
//        Serial.println(mp3.track[trackNo].name);

        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
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

        if ((number < 0) || (number >= kActionCount))
        {
          request->send(400, "application/json", "{\"error\":\"invalid action number\"}");
          return;
        }

        JsonDocument doc;
        doc["name"] = action[number].name;
        doc["number"] = number;
        doc["enableLocal"] = action[number].enableLocal;
        doc["enableRemote"] = action[number].enableRemote;
        doc["repeat"] = action[number].getRepeat();
        doc["state"] = action[number].getState();        
        doc["userState"] = action[number].userState; 
        doc["userVar1"] = action[number].userVar1; 
        doc["userVar2"] = action[number].userVar2;

        char responseBuffer[kJsonResponseBufferSize];
        const size_t responseLength = serializeJson(doc, responseBuffer, sizeof(responseBuffer));
        if (responseLength == 0)
        {
          request->send(500, "application/json", "{\"error\":\"serialization failed\"}");
          return;
        }

        request->send(200, "application/json", responseBuffer);
        Serial.print("Sent /api/action/config response for action number: ");
        Serial.println(number);
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

        if ((bit < 0) || (bit >= kGpioCount))
        {
          request->send(400, "application/json", "{\"error\":\"invalid bitno\"}");
          return;
        }
//        Serial.print("Received /api/gpio/config/bit GET request Bit:");
//        Serial.println(bit);
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
        JsonDocument doc;
        doc["bitNo"] = bit;
        doc["value"] = gpio[bit].getValue();
        doc["name"] = gpio[bit].name;
        doc["type"] = gpio[bit].type;
        doc["preset0"] = gpio[bit].preset0;
        doc["preset1"] = gpio[bit].preset1;
        doc["preset2"] = gpio[bit].preset2;
        doc["rate"] = gpio[bit].rate;
//Serial.print("getConfigWEBenpoint  enableRemote: ");
//Serial.println(gpio[bit].enableRemote);
        doc["enableRemote"] = gpio[bit].enableRemote;
        doc["enableLocal"] = gpio[bit].enableLocal;
        doc["publishRate"] = gpio[bit].getPublishRate();
        doc["easingType"] = gpio[bit].getEasingType();
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/object/pose", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      if (request->url() == "/api/object/pose")
      {
        //Serial.print("Received /api/object/pose GET request");
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
        JsonDocument doc;
        doc["poseX"] = currentPose.x;
        doc["poseY"] = currentPose.y;
        doc["poseBrg"] = currentPose.heading;
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/object/status/index", HTTP_GET, [](AsyncWebServerRequest *request) 
    {
      long index;
      if (request->url() == "/api/object/status/index")
      {
        // Check if "index" parameter exists in the url ( ?index=x)
        char* ptr;
        if (request->hasParam("index")) index = strtol(request->getParam("index")->value().c_str(),&ptr,10);
        else index = 0;

        if ((index < 0) || (index >= kObjectCount))
        {
          request->send(400, "application/json", "{\"error\":\"invalid object index\"}");
          return;
        }
        //Serial.print("Received /api/object/status/index GET request index:");
        //Serial.println(index);
        AsyncResponseStream *response = request->beginResponseStream("application/json", kJsonResponseBufferSize);
        JsonDocument doc;
        doc["index"] = index;
        doc["name"] = gcodeObjects[index].name;
        serializeJson(doc,*response);  
        request->send(response);
      }
    });

    server.on("/api/object/status/all", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      // Returns all object names/indices in a single response, avoiding a burst of
      // 16 separate requests (observed to exhaust ESP32 TCP resources and stall
      // the MQTT connection when objects.html loaded).
      AsyncResponseStream *response = request->beginResponseStream("application/json", 1024);
      JsonDocument doc;
      JsonArray objects = doc["objects"].to<JsonArray>();
      for (int i = 0; i < kObjectCount; i++)
      {
        JsonObject obj = objects.add<JsonObject>();
        obj["index"] = i;
        obj["name"] = gcodeObjects[i].name;
      }
      serializeJson(doc, *response);
      request->send(response);
    });


    server.on("/api/scene/status/all", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      // Returns all scene names/indices in a single response, mirroring /api/object/status/all.
      AsyncResponseStream *response = request->beginResponseStream("application/json", 1024);
      JsonDocument doc;
      JsonArray sceneArray = doc["scenes"].to<JsonArray>();
      for (int i = 0; i < kSceneCount; i++)
      {
        JsonObject obj = sceneArray.add<JsonObject>();
        obj["index"] = i;
        obj["name"] = scenes[i].name;
      }
      serializeJson(doc, *response);
      request->send(response);
    });

    server.on("/api/object/run", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst object = json.as<JsonObjectConst>();
      const bool hasIndex = object.containsKey("index");
      const bool hasPath = object.containsKey("path");

      if (!hasIndex || !hasPath)
      {
        Serial.println("Deserialisationerror /api/object/run");
        request->send(400, "application/json", "{\"error\":\"invalid object run payload\"}");
        return;
      }
      const int index = object["index"].as<int>();
      //const int path = object["path"].as<int>();
      char ObjectFile[32];
      snprintf(ObjectFile, sizeof(ObjectFile), "Objects/SoD_%d.GCO", index);
      runSDPath(ObjectFile);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/object/identify", HTTP_POST, [](AsyncWebServerRequest *request) {
      Serial.println("Received /api/object/identify POST request");
      if (!queueObjectIdentify())
      {
        Serial.println("Failed to queue object identify");
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/scene/run", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst object = json.as<JsonObjectConst>();
      const bool hasIndex = object.containsKey("index");

      if (!hasIndex)
      {
        Serial.println("Deserialisationerror /api/scene/run");
        request->send(400, "application/json", "{\"error\":\"invalid scene run payload\"}");
        return;
      }

      const int index = object["index"].as<int>();
      if ((index < 0) || (index >= kSceneCount))
      {
        request->send(400, "application/json", "{\"error\":\"invalid scene index\"}");
        return;
      }

      scenes[index].run();
      request->send(200, "application/json", "OK");
    });

    server.on("/api/node/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
      powerGPIO(false);
      delay(1000);
      ESP.restart();
    });

    server.on("/api/nodeid/value", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("value"))
      {
        Serial.println("/api/nodeid/value - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid nodeid payload\"}");
        return;
      }

      const char* nodeID = doc["value"].as<const char*>();
      if (nodeID != nullptr)
      {
        node.setNodeID(strtol(nodeID, nullptr, 16));
        writeFile(SPIFFS, nodeIDPath, node.getNodeIDstring());
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/brokerip/value", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("value"))
      {
        Serial.println("/api/brokerip/value - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid brokerip payload\"}");
        return;
      }

      const char* buff = doc["value"].as<const char*>();
      if (buff != nullptr)
      {
        writeFile(SPIFFS, brokerIPPath, buff);
        strncpy(node.brokerIP, buff, sizeof(node.brokerIP) - 1);
        node.brokerIP[sizeof(node.brokerIP) - 1] = '\0';
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/gpio/config/bit", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst object = json.as<JsonObjectConst>();
      if (!object.containsKey("bitNo") || !object.containsKey("type"))
      {
        Serial.println("/api/gpio/config/bit - invalid payload");
        request->send(400, "application/json", "{\"error\":\"invalid GPIO config payload\"}");
        return;
      }

      const int bit = object["bitNo"].as<int>();
      if ((bit < 0) || (bit >= kGpioCount))
      {
        request->send(400, "application/json", "{\"error\":\"invalid bitNo\"}");
        return;
      }

      const char* name = object["name"].as<const char*>();
      if (name != nullptr)
      {
        strcpy(gpio[bit].name, name);
      }
      gpio[bit].setType(object["type"].as<int>());
      if (object.containsKey("value"))
      {
        gpio[bit].alwaysWrite(object["value"].as<int>());
      }
      gpio[bit].preset0 = object["preset0"].as<int>();
      gpio[bit].preset1 = object["preset1"].as<int>();
      gpio[bit].preset2 = object["preset2"].as<int>();
      gpio[bit].rate = object["rate"].as<int>();
      gpio[bit].enableRemote = object["enableRemote"].as<bool>();
      gpio[bit].enableLocal = object["enableLocal"].as<bool>();
      gpio[bit].setPublishRate(object["publishRate"].as<int>());
      gpio[bit].setEasingType(object["easingType"].as<int>());
      writeConfigFile(SPIFFS, bit);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/soundtrack/config", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("trackNo") || !doc.containsKey("name"))
      {
        Serial.println("/api/soundtrack/config - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid soundtrack config payload\"}");
        return;
      }

      uint8_t trackNo = doc["trackNo"].as<int>();
      if (trackNo >= kTrackCount)
      {
        request->send(400, "application/json", "{\"error\":\"invalid trackNo\"}");
        return;
      }

      const char* buff = doc["name"].as<const char*>();
      if (buff != nullptr)
      {
        strncpy(mp3.track[trackNo].name, buff, sizeof(mp3.track[trackNo].name) - 1);
        mp3.track[trackNo].name[sizeof(mp3.track[trackNo].name) - 1] = '\0';
      }
      mp3.track[trackNo].duration = doc["duration"].as<int>();
      mp3.track[trackNo].volume = doc["volume"].as<int>();
      mp3.track[trackNo].enableRemote = doc["enableRemote"].as<bool>();
      mp3.track[trackNo].enableLocal = doc["enableLocal"].as<bool>();
      writeMP3TrackConfigFile(SPIFFS, trackNo);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/action/config", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("number") || !doc.containsKey("name"))
      {
        Serial.println("/api/action/config - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid action config payload\"}");
        return;
      }

      uint8_t number = doc["number"].as<int>();
      if (number >= kActionCount)
      {
        request->send(400, "application/json", "{\"error\":\"invalid action number\"}");
        return;
      }

      const char* buff = doc["name"].as<const char*>();
      if (buff != nullptr)
      {
        strncpy(action[number].name, buff, sizeof(action[number].name) - 1);
        action[number].name[sizeof(action[number].name) - 1] = '\0';
      }
      action[number].enableRemote = doc["enableRemote"].as<bool>();
      action[number].enableLocal = doc["enableLocal"].as<bool>();
      writeActionConfigFile(SPIFFS, number);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/gpio/value/bit", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst object = json.as<JsonObjectConst>();
      if (!object.containsKey("bitNo") || !object.containsKey("value"))
      {
        Serial.println("/api/gpio/value/bit - invalid payload");
        request->send(400, "application/json", "{\"error\":\"invalid GPIO value payload\"}");
        return;
      }

      const int bit = object["bitNo"].as<int>();
      if ((bit < 0) || (bit >= kGpioCount))
      {
        request->send(400, "application/json", "{\"error\":\"invalid bitNo\"}");
        return;
      }

      gpio[bit].alwaysWrite(object["value"].as<int>());
      Serial.print("GPIO Bit:");
      Serial.println(bit);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/mp3Player/config", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("manualTrim") || !doc.containsKey("autoTrim"))
      {
        Serial.println("/api/mp3Player/config - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid mp3Player config payload\"}");
        return;
      }

      mp3.manualTrim = doc["manualTrim"].as<int>();
      mp3.autoTrim = doc["autoTrim"].as<int>();
      writeMP3ConfigFile(SPIFFS);
      request->send(200, "application/json", "OK");
    });

    server.on("/api/action/play", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("number"))
      {
        Serial.println("/api/action/play - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid action play payload\"}");
        return;
      }

      int number = doc["number"].as<int>();
      bool loop = doc["loop"].as<bool>();
      if ((number >= 0) && (number < kActionCount))
      {
        action[number].play(CMD_ANY, loop);
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/action/stop", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("number"))
      {
        Serial.println("/api/action/stop - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid action stop payload\"}");
        return;
      }

      int number = doc["number"].as<int>();
      if ((number >= 0) && (number < kActionCount))
      {
        action[number].stop(CMD_ANY);
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/soundtrack/play", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("track"))
      {
        Serial.println("/api/soundtrack/play - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid soundtrack play payload\"}");
        return;
      }

      int track = doc["track"].as<int>();
      bool loop = doc["loop"].as<bool>();
      if ((track >= 0) && (track < kTrackCount))
      {
        mp3.play(CMD_ANY, track, loop);
      }
      request->send(200, "application/json", "OK");
    });

    server.on("/api/soundtrack/stop", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObjectConst doc = json.as<JsonObjectConst>();
      if (!doc.containsKey("track"))
      {
        Serial.println("/api/soundtrack/stop - Deserialisationerror");
        request->send(400, "application/json", "{\"error\":\"invalid soundtrack stop payload\"}");
        return;
      }

      int track = doc["track"].as<int>();
      if ((track >= 0) && (track < kTrackCount))
      {
        mp3.stop(CMD_ANY);
      }
      request->send(200, "application/json", "OK");
    });

    server.serveStatic("/", SPIFFS, "/");
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
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to host (.local): ");
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}


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
