#include "MQTTServices.h"
#include "gpio.h"

// Create Queue handles
QueueHandle_t MQTTSensorQueue;
QueueHandle_t MQTTDebugQueue;
QueueHandle_t MQTTOperationQueue;

void setupMQTTServices()
{
  // Create a Sensor Publish queue
  MQTTSensorQueue = xQueueCreate(20, sizeof(MQTTSensor));
  if (MQTTSensorQueue == NULL) 
  {
    Serial.println("Error creating the sensor queue");
    while (true); // Stop execution
  }else Serial.println("created sensor queue");
  // Create a Debug Publish queue
  MQTTDebugQueue = xQueueCreate(10, sizeof(MQTTDebugPayload));
  if (MQTTDebugQueue == NULL) 
  {
    Serial.println("Error creating the debug queue");
    while (true); // Stop execution
  }
  // Create Operation Publish queue
  MQTTOperationQueue = xQueueCreate(10, sizeof(MQTTDebugPayload));
  if (MQTTOperationQueue == NULL) 
  {
    Serial.println("Error creating the operation queue");
    while (true); // Stop execution
  }
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

void MQTTPublishDebug(MQTTDebugPayload payload)
{
  // Will block if the sensor publish Queue is full
  // Places the payload data into the MQTT Sensor Publishing Queue
  if (xQueueSend(MQTTDebugQueue, &payload, 0)) 
  {
    Serial.printf("[Debug Sender] Sent value: %s\n", payload.message);
  } 
  else 
  {
    Serial.println("[Debug Sender] Queue full, failed to send");
  }
  vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
}

// debug class which provides a streams interface for code debugging at sends out
// debug information to the MQTT Communications Module via a queue

    // Constructor - accepts topic string
    debugStream ::debugStream()
    {

    }

    // Required: Return number of bytes available to read
    int debugStream::available() {

        return (head >= tail) ? (head - tail) : (64 - tail + head);
    }

    // Required: Read one byte (-1 if none)
    int debugStream::read() 
    {
        if (available() == 0) return -1;
        char c = buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        return c;
    }

    // Required: Peek next byte without removing it
    int debugStream::peek() 
    {
        if (available() == 0) return -1;
        return buffer[tail];
    }

    // Required: Write one byte
    size_t debugStream::write(uint8_t c) 
    {
        size_t nextHead = (head + 1) % BUFFER_SIZE;
        if (nextHead == tail) {
            // Buffer full — overwrite oldest data
            tail = (tail + 1) % BUFFER_SIZE;
        }
        buffer[head] = c;
        head = nextHead;
        return 1;
    }

    // Optional: Write a string or buffer
    size_t debugStream::write(const uint8_t *data, size_t len) 
    {
        size_t written = 0;
        for (size_t i=0; i < len; i++) 
        {
            written += write(data[i]);
        }

        if(data[len-1]==10)                             // keep buffering until we receive a linefeed char
        {
            // Build the buffer into the payload string
            int i=0;
             while (available()) 
            {
                payload.message[i++] = read();
                payload.message[i]=0;
            }
            MQTTPublishDebug(payload);
        }
        return written;
    }

    // Optional: Flush (not needed for memory buffer)
//    void debugStream::flush() {}

    void debugStream::appendNodeID(bool append)
    {
        // Set true to have the nodeID automatically appended to the topic before publication
        payload.appendID = append;
    }


    