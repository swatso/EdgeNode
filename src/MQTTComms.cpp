// This module handles MQTT data transfers via wifi
// Intended for use with JMRI using turnout and sensor topics
// subscribes to turnout topics 00 through 0F, maintaining bits in the controlbytes C0 and C1
// and drives sensor topics 00 through 0F based on the state of status bytes S0 and S1
// Use this alongside the WIFI_Manager (asynchronous web server)

// Call initMQTT() at startup
//
//  Needs the following:-
//    nodeIDString          -string containing the Wifi Node ID which will be used in the topics
//    brokerIP              -string containing the IP address of the MQTT Broker

// Provides the following functions:-
//  
//    checkMQTTState()      -true = client connected
//    getMQTTUptime()       -connection uptime in minutes

// everything then runs on a ticker
//

#include <string>
#include <Ticker.h>
#include <PubSubClient.h>         // MQTT library
#include <WiFi.h>
#include <ESPAsyncWebServer.h>                    		
#include <AsyncTCP.h>                             		
#include "MQTTComms.h"
#include "MQTTServices.h"
#include "NodeServices.h"
#include "WiFiManager.h"
#include "gpio.h"
#include "sound.h"
#include "action.h"

Ticker runMQTT;

long MQTTConnectionTime;

const char* baseSensorTopic = "iot/io/sensor/nn00";
const char* baseTurnoutTopic = "iot/io/turnout/nn00";
const char* baseSoundTopic = "iot/io/sound/nn00";
const char* soundAutoTrimTopic = "iot/io/sound/autotrim";
const char* baseActionTopic = "iot/io/action/nn00";
char sensorTopic[30];
char turnoutTopic[30];
char reporterTopic[30];
char soundTopic[30];
char actionTopic[30];
char localDebugTopic[30];
char globalDebugTopic[30];
char localOperationsTopic[30];
char globalOperationsTopic[30];
//boolean C0[16];
//boolean S0[16];
char payloadTrue[] = "ACTIVE";
char payloadFalse[] = "INACTIVE";
char payloadCentre[] = "MIDDLE";
WiFiClient espClient;
PubSubClient client(espClient);
TaskHandle_t MQTTSensorService;
TaskHandle_t MQTTMessageService;

void setupMQTTComms() 
{
  initTopics(node.getNodeIDstring());
  for(int i=0; i<16; i++)
  {
    //C0[i] = false;
    //S0[i] = false;
  } 
  client.setBufferSize(512);
  client.setServer(node.brokerIP, 1883);
  client.setCallback(MQTTcallback);
  xTaskCreatePinnedToCore(sensorReceiverTask, "Sensor Task", 2048, NULL, 1, &MQTTSensorService, 1);
  xTaskCreatePinnedToCore(messageReceiverTask, "Message Task", 2048, NULL, 1, &MQTTMessageService, 1);
  connectMQTTClient();
  runMQTT.attach_ms(500, serviceConnection);
}

boolean connectMQTTClient() 
{
  // Attempt to reconnect, return true if successful, otherwise false
  if (!client.connected()) 
  {
    Serial.println("(connectMQTTClient) Not connected");
    // Build clientID based on the NodeID
    String clientId = node.getNodeIDstring();
    clientId += "-";
    clientId += String(micros() & 0xff, 16); // to randomise. sort of
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) 
    {
      Serial.print("Connected to MQTT Broker at: ");
      Serial.println(node.brokerIP);
      MQTTConnectionTime = millis();
      subscribeTopics();
      return(true);     // Connected
    }
    return(false);    // Not yet connected
  } 
  return(true);         // already connected
}


boolean  subscribeTopics()
{
  //Serial.println("<subscribe>");
  // subscribe to the 16 (turnOut) topics for direct GPIO control from JMRI
  std::string subscription;
  byte i;
  for(i=0; i<16; i++)
  {
    // We are subscribing to turnout topics 00 to 0F hex
    turnoutTopic[18]= i + 0x30;
    if(turnoutTopic[18] > 57)turnoutTopic[18]+=7;

    subscription.assign(turnoutTopic,19);
    client.subscribe(subscription.c_str());
    serviceConnection();
    yield();
  }

  // subscribe to the 16 (sound) topics for direct sound control
  for(i=0; i<16; i++)
  {
    // We are subscribing to sound topics 00 to 0F hex
    soundTopic[16]= i + 0x30;
    if(soundTopic[16] > 57)soundTopic[16]+=7;

    subscription.assign(soundTopic,19);
    client.subscribe(subscription.c_str());
    serviceConnection();
    yield();
  }

  // subscribe to the action topic for direct action control
  client.subscribe(soundAutoTrimTopic);

  for(i=0; i<17; i++)
  {
    // We are subscribing to action topics 00 to 0F hex
    actionTopic[17]= i + 0x30;
    if(actionTopic[17] > 57)actionTopic[17]+=7;

    Serial.print("Subscribing to:");
    Serial.println(actionTopic);
    client.subscribe(actionTopic);
    serviceConnection();
    yield();
  }
  return(true);
}


boolean publishMQTT(char* topic, char* message)
{
   if (client.connected())
   {
      client.publish(topic , message);
      return(true);
   }
   return(false);
}


void serviceConnection()
{
  // Service the MQTT client
  if (client.connected()) 
  {
    client.loop();
  }
  else 
  {
    Serial.println("(serviceConnection) MQTT not connected - attempting reconnect");
    connectMQTTClient();
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) 
{
  // Called when one of the subscribed topics is recieved
  // ie a Turnout has changed in value...
  // Event will be defined by topic[17] and topic[18] in the range 00 to 0F hex
  byte event;


  Serial.print("(MQTTcallback) topic:");
  Serial.println(topic);
  
  if (strncmp(topic, turnoutTopic, 14) == 0) 
  {
    // This is a Turnout topic
    event = (topic[18]-0x30);
    if (event > 9)event -= 7;
    Serial.print("(MQTTcallback) Turnout event:");
    Serial.println(event);
    if(gpio[event].type == GPIO_SERVO_ACTUATOR)
    {
        // Move the servo to the preset position based on the payload value (T for preset2, M for preset1, anything else for preset0)
        if ((char)payload[0] == 'T')gpio[event].remoteWrite(gpio[event].preset2);
        else if ((char)payload[0] == 'M')gpio[event].remoteWrite(gpio[event].preset1);
        else gpio[event].remoteWrite(gpio[event].preset0); 
    }
    else if(gpio[event].type == GPIO_SERVO)
    {
      // Set the gpio bit based on the payload value (T for true, anything else for false) 
      gpio[event].remoteWrite(std::stoi((char*)payload));
      Serial.print("(MQTTcallback) Payload as int:");
      Serial.println(std::stoi((char*)payload));
    }
    else
    {
      // Set the gpio bit based on the payload value (T for true, anything else for false) 
      if ((char)payload[0] == 'T')gpio[event].remoteWrite(true);
      else gpio[event].remoteWrite(false);
    }
  }
  if (strncmp(topic, soundAutoTrimTopic, 21) == 0) 
  {
    Serial.print("(MQTTcallback) Sound AutoTrim event");
    // True or Thrown paylaod will apply autoTrim volume adjustment
    if ((char)payload[0] == 'T')mp3.autoTrimEnabled=true;
    else mp3.autoTrimEnabled=false;
    Serial.print("(MQTTcallback) AutoTrim:"); Serial.println(mp3.autoTrimEnabled);
  }
  else if (strncmp(topic, soundTopic, 12) == 0) 
  {
    // This is a Sound topic
    event = (topic[16]-0x30);
    if (event > 9)event -= 7;
//    Serial.print("(MQTTcallback) Sound event:");
//    Serial.println(event);
    if ((char)payload[0] == 'P')mp3.play(CMD_REMOTE,event,false);
    else if ((char)payload[0] == 'L')mp3.play(CMD_REMOTE,event,true);
    else mp3.stop(CMD_REMOTE);
  }
  if (strncmp(topic, actionTopic, 13) == 0) 
  {
    // This is an Action topic
    // Payload is
    // P for play, L for Loop (play with repeat), S for stop
    event = (topic[17]-0x30);
    if (event > 9)event -= 7;
//    Serial.print("(MQTTcallback) Action event:");
//    Serial.println(event);
    if ((char)payload[0] == 'P')action[event].play(CMD_REMOTE,false);
    else if ((char)payload[0] == 'L')action[event].play(CMD_REMOTE,true);
    else if ((char)payload[0] == 'S')action[event].stop(CMD_REMOTE);
  }
//  Serial.println(event);
}

// Receiver task: waits for gpio sensor data from the queue and publishes it
void sensorReceiverTask(void *pvParameters) 
{
  MQTTSensor receivedSensor;
  Serial.println("sensorRecieverTask started");
  while (true) {
    // Wait indefinitely for data
    if (xQueueReceive(MQTTSensorQueue, &receivedSensor, portMAX_DELAY) == pdPASS) 
    {
      switch(gpio[receivedSensor.bitNo].type)
      {
        case GPIO_NONE:
        case GPIO_SERVO:
        case GPIO_AIN:
          char payload[10];
          sprintf(payload, "%d", receivedSensor.value);
          publishMQTT(sensorTopic, payload);
          break;
        case GPIO_DIGIN:
        case GPIO_DIGOUT:
        case GPIO_DIGOUT_PULSE:
        case GPIO_PWM:
          if(receivedSensor.value > 0)publishMQTT(sensorTopic, payloadTrue);
          else publishMQTT(sensorTopic, payloadFalse);
          break;
          case GPIO_SERVO_ACTUATOR:
            // For servo actuators we want to publish a true/false/Centre topic based on whether the servo is at the preset position or not
            if(receivedSensor.value == gpio[receivedSensor.bitNo].preset2)publishMQTT(sensorTopic, payloadTrue);
            else if(receivedSensor.value == gpio[receivedSensor.bitNo].preset1)publishMQTT(sensorTopic, payloadCentre);
            else if(receivedSensor.value == gpio[receivedSensor.bitNo].preset0)publishMQTT(sensorTopic, payloadFalse);
            break;
        default:
          Serial.println("Invalid sensor type in sensorReceiverTask");
          break;
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

// Message Receiver task: waits for data from the message queue and publishes it
void messageReceiverTask(void *pvParameters) 
{
  MQTTMessagePayload receivedMessage;
  while (true) {
    // Wait indefinitely for data
    if (xQueueReceive(MQTTMessageQueue, &receivedMessage, portMAX_DELAY) == pdPASS) 
    {
      Serial.printf("[Message] Received value: %s\n", receivedMessage.message);
      publishMQTT(receivedMessage.topic, receivedMessage.message);
      vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
  }
}

boolean checkMQTTState()
{
  // returns the MQTT client connection state
  return(client.connected());
}

unsigned long getMQTTUptime()
{
  if(checkMQTTState() == true)return((millis()-MQTTConnectionTime)/60000);
  else return(0);
}

void initTopics(char* currentNodeID)
{
  // Build the basic subscription strings by inserting the node ID
  int i;
  for(i=0; i<30 && baseTurnoutTopic[i] != 0; i++)turnoutTopic[i] = baseTurnoutTopic[i];
  turnoutTopic[15]=currentNodeID[0];
  turnoutTopic[16]=currentNodeID[1];  
  for(i=0; i<30 && baseSensorTopic[i] != 0; i++)sensorTopic[i] = baseSensorTopic[i];
  sensorTopic[14]=currentNodeID[0];
  sensorTopic[15]=currentNodeID[1];  
  for(i=0; i<30 && baseSoundTopic[i] != 0; i++)soundTopic[i] = baseSoundTopic[i];
  soundTopic[13]=currentNodeID[0];  
  soundTopic[14]=currentNodeID[1];
  for(i=0; i<30 && baseActionTopic[i] != 0; i++)actionTopic[i] = baseActionTopic[i];
  actionTopic[14]=currentNodeID[0];  
  actionTopic[15]=currentNodeID[1];
  Serial.println(turnoutTopic);
  Serial.println(sensorTopic);
  Serial.println(reporterTopic);
  Serial.println(localDebugTopic);
  Serial.println(globalDebugTopic);
  Serial.println(localOperationsTopic);
  Serial.println(globalOperationsTopic);
  Serial.println(soundTopic);
  Serial.println(actionTopic);

}
