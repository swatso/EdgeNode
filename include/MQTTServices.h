#ifndef MQTTSERVICES_H_
#define MQTTSERVICES_H_

//============ Includes ====================
#include <cstdint>
#include <Arduino.h>
#include "MQTTComms.h"

// Queue handles
extern QueueHandle_t MQTTSensorQueue;
extern QueueHandle_t MQTTMessageQueue;

#define MQTT_MESSAGE_BUFFER_SIZE 100

// Payload structure for passing data into the transmission queue to MQTT (MQTTSensorQueue)
struct MQTTSensor 
{
    public:
        uint8_t bitNo;   // Hardware Module bit number (not gpio number)
        int value;       // Current Value
};

// Payload structure for passing data into the transmission queue to MQTT (MQTTMessageQueue)
struct MQTTMessagePayload
{
    public:
        char topic[30];   // MQTT topic
        char message[MQTT_MESSAGE_BUFFER_SIZE]; // MQTT message
};

void setupMQTTServices();                                   
void MQTTPublishSensor(MQTTSensor payload);                 
void MQTTPublishMessage(MQTTMessagePayload payload);

#endif // MQTTSERVICES_H_