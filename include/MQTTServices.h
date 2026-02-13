#ifndef MQTTSERVICES_H_
#define MQTTSERVICES_H_

//============ Includes ====================
#include <cstdint>
#include <Arduino.h>

// Queue handles
extern QueueHandle_t MQTTSensorQueue;
extern QueueHandle_t MQTTDebugQueue;
extern QueueHandle_t MQTTOperationQueue;

// Payload structure for passing data into the transmission queue to MQTT (MQTTSensorQueue)
#define BUFFER_SIZE 64

struct MQTTSensor 
{
    public:
        uint8_t bank;    // Sensor Bank
        uint8_t bitNo;   // Hardware Module bit number (not gpio number)
        int value;       // Current Value
};


// Debug class which allows creation logging streams to an MQTT topic
// You can use .print() and .println() calls to log information
// Data is published in complete lines, so multiple .print() calls will be buffered until a .println() is made
// There is an optional .setNodeID() method which allows a unique decimal ID to be set (0-255d)
// You can also use .appendNodeID() to cause the /nn to/*				*** struct moved to arduinoGlue.h * be appended to the topic string, allowing each node to
// publish to a different topic

// This class has a dependancy on the Communications.ino sketch which provides the underlying MQTT interface

//    static const size_t BUFFER_SIZE = 64;
//    static const size_t TOPIC_SIZE = 64;


struct MQTTDebugPayload
{
    public:
        uint8_t bank;    // Sensor Bank
        char message[BUFFER_SIZE+5];
        boolean appendID = false;
};


class debugStream : public Stream 
{
private:

    char buffer[BUFFER_SIZE];
    MQTTDebugPayload payload;
    size_t head = 0; // write position
    size_t tail = 0; // read position

public:
    // Constructor
    debugStream();

    // Required: Return number of bytes available to read
    int available() override;

    // Required: Read one byte (-1 if none)
    int read() override;

    // Required: Peek next byte without removing it
    int peek() override;

    // Required: Write one byte
    size_t write(uint8_t c) override;

    // Write a string or buffer
    size_t write(const uint8_t *data, size_t len) override;

    // Optional: Flush (not needed for memory buffer)
//    void flush() override;

    // request to append nodeID to topic before publishing
    void appendNodeID(bool append);

};


// pre definition of public functions
void MQTTPublishSensor(MQTTSensor payload);
void MQTTPublishDebug(MQTTDebugPayload payload);

// Create an instance of the debug stream
//debugStream debug;
//debugStream operation;

#endif // MQTTSERVICES_H_