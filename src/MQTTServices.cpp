#include "MQTTServices.h"
#include "gpio.h"

// Create Queue handles
QueueHandle_t MQTTSensorQueue;
QueueHandle_t MQTTMessageQueue;

void setupMQTTServices()
{
  // Create a Sensor Publish queue
  MQTTSensorQueue = xQueueCreate(20, sizeof(MQTTSensor));
  if (MQTTSensorQueue == NULL) 
  {
    Serial.println("Error creating the sensor queue");
    while (true); // Stop execution
  }else Serial.println("created sensor queue");
  // Create a Message Publish queue
  
  MQTTMessageQueue = xQueueCreate(10, MQTT_MESSAGE_BUFFER_SIZE);
  if (MQTTMessageQueue == NULL) 
  {
    Serial.println("Error creating the message queue");
    while (true); // Stop execution
  }
  else Serial.println("created message queue"); 
}

void MQTTPublishSensor(MQTTSensor payload)
{
  // Will block if the sensor publish Queue is full
  // Places the payload data into the MQTT Sensor Publishing Queue
  if (xQueueSend(MQTTSensorQueue, &payload, 0)) 
  {
    Serial.printf("[Sensor Sender] Sent value: %d\n", payload.value);
  } 
  else 
  {
    Serial.println("[Sensor Sender] Queue full, failed to send");
  }
  vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
}


void MQTTPublishMessage(MQTTMessagePayload payload)
{
  // Will block if the sensor publish Queue is full
  // Places the payload data into the MQTT Sensor Publishing Queue
  if (xQueueSend(MQTTMessageQueue, &payload, 0)) 
  {
    Serial.printf("[Debug Sender] Sent value: %s\n", payload.message);
  } 
  else 
  {
    Serial.println("[Debug Sender] Queue full, failed to send");
  }
  vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
}


    